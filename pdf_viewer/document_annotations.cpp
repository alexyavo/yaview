// The PDF storage of a document's annotations.
//
// Bookmarks, highlights and portals are PDF annotations in the file, so other PDF readers show them too:
//   - a highlight is a Highlight annotation, an underline (uppercase type) an Underline annotation and a
//     strikethrough (type '_') a StrikeOut annotation; its color is the type's color and its note is the
//     annotation's contents,
//   - a bookmark is a Text annotation (a note icon at its position), a freetext bookmark a FreeText annotation
//     and a box bookmark a Square annotation,
//   - a portal is a Link annotation with a GoTo (same file) or GoToR (another file) action to its destination,
//     marked with a /Yaview /Portal entry to tell it from the document's own links.
// Marks have no PDF equivalent: they are entries << /D destination /Title description >> in a /Marks
// dictionary of a private /Yaview dictionary in the document catalog, keyed by the mark's letter.
//
// Annotations of these types that other programs made are read the same way: every Highlight, Text, FreeText
// and Square annotation in the file is a yaview highlight or bookmark, drawn by yaview rather than by MuPDF
// (see PdfRenderer). Changes are appended to the file with an incremental save, which keeps the original
// bytes of the file untouched.
//
// Positions in the file are in PDF user space (the page's coordinate system, usually with the origin at the
// bottom left); everywhere else yaview uses page coordinates (MuPDF's page space, origin at the top left) or
// absolute document coordinates. The page transform of MuPDF converts between the first two.

#include "document.h"
#include "utils.h"

#include <cctype>
#include <functional>

#include <qdatetime.h>
#include <qdir.h>
#include <qfileinfo.h>

extern float HIGHLIGHT_COLORS[26 * 3];
extern float BOOKMARK_RECT_SIZE;
extern bool EXACT_HIGHLIGHT_SELECT;

namespace {

    // the key of the private dictionary in the document catalog and of the portal marker on Link annotations
    const char* YAVIEW_KEY = "Yaview";
    const char* MARKS_KEY = "Marks";

    int64_t current_time_secs() {
        return QDateTime::currentSecsSinceEpoch();
    }

    int64_t annotation_time_secs(const std::string& time) {
        QDateTime datetime = QDateTime::fromString(QString::fromStdString(time), "yyyy-MM-dd HH:mm:ss");
        return datetime.isValid() ? datetime.toSecsSinceEpoch() : current_time_secs();
    }

    int64_t read_date(fz_context* ctx, pdf_obj* dict, pdf_obj* key) {
        pdf_obj* date = pdf_dict_get(ctx, dict, key);
        return date ? pdf_to_date(ctx, date) : -1;
    }

    // the matrix from PDF user space to page coordinates
    fz_matrix page_transform(fz_context* ctx, pdf_document* pdf, int page_number) {
        fz_matrix ctm = fz_identity;
        pdf_page_obj_transform(ctx, pdf_lookup_page_obj(ctx, pdf, page_number), nullptr, &ctm);
        return ctm;
    }

    // an annotation's /C (or /IC) color as RGB; false when it has none
    bool read_color(fz_context* ctx, pdf_obj* annot, pdf_obj* key, float out_color[3]) {
        pdf_obj* color = pdf_dict_get(ctx, annot, key);
        int n = pdf_array_len(ctx, color);
        if (n == 3) {
            for (int i = 0; i < 3; i++) out_color[i] = pdf_array_get_real(ctx, color, i);
            return true;
        }
        if (n == 1) {
            out_color[0] = out_color[1] = out_color[2] = pdf_array_get_real(ctx, color, 0);
            return true;
        }
        if (n == 4) {
            float k = pdf_array_get_real(ctx, color, 3);
            for (int i = 0; i < 3; i++) out_color[i] = (1 - pdf_array_get_real(ctx, color, i)) * (1 - k);
            return true;
        }
        return false;
    }

    // The file name in a GoToR action's file specification, which is either a string or a Filespec dictionary.
    std::string read_file_spec(fz_context* ctx, pdf_obj* file_spec) {
        if (pdf_is_dict(ctx, file_spec)) {
            const char* unicode_name = pdf_dict_get_text_string_opt(ctx, file_spec, PDF_NAME(UF));
            if (unicode_name) return unicode_name;
            return pdf_dict_get_text_string(ctx, file_spec, PDF_NAME(F));
        }
        return pdf_is_string(ctx, file_spec) ? pdf_to_text_string(ctx, file_spec) : "";
    }

    // the destination [page /XYZ left top zoom] of a GoTo action, a GoToR action (with a page index instead
    // of a page object) or a mark
    struct Destination {
        int page = 0;
        std::optional<float> x;
        std::optional<float> y;
        std::optional<float> zoom_level;
    };

    std::optional<Destination> read_destination(fz_context* ctx, pdf_document* pdf, pdf_obj* dest, bool remote) {
        if (pdf_is_dict(ctx, dest)) {
            dest = pdf_dict_get(ctx, dest, PDF_NAME(D));
        }
        if (!pdf_is_array(ctx, dest) || pdf_array_len(ctx, dest) < 2) {
            return {};
        }
        Destination res;
        pdf_obj* page = pdf_array_get(ctx, dest, 0);
        res.page = remote ? pdf_to_int(ctx, page) : pdf_lookup_page_number(ctx, pdf, page);
        if (res.page < 0) {
            return {};
        }
        if (pdf_name_eq(ctx, pdf_array_get(ctx, dest, 1), PDF_NAME(XYZ))) {
            pdf_obj* x = pdf_array_get(ctx, dest, 2);
            pdf_obj* y = pdf_array_get(ctx, dest, 3);
            pdf_obj* zoom = pdf_array_get(ctx, dest, 4);
            if (pdf_is_number(ctx, x)) res.x = pdf_to_real(ctx, x);
            if (pdf_is_number(ctx, y)) res.y = pdf_to_real(ctx, y);
            if (pdf_is_number(ctx, zoom) && pdf_to_real(ctx, zoom) > 0) res.zoom_level = pdf_to_real(ctx, zoom);
        }
        return res;
    }

    // the Marks dictionary of the private Yaview dictionary in the catalog, created when `create` is set
    pdf_obj* marks_dictionary(fz_context* ctx, pdf_document* pdf, bool create) {
        pdf_obj* catalog = pdf_dict_get(ctx, pdf_trailer(ctx, pdf), PDF_NAME(Root));
        pdf_obj* yaview = pdf_dict_gets(ctx, catalog, YAVIEW_KEY);
        if (!pdf_is_dict(ctx, yaview)) {
            if (!create) return nullptr;
            // an indirect object, so that a mark rewrites only this small dictionary and not the catalog
            yaview = pdf_add_new_dict(ctx, pdf, 1);
            pdf_dict_puts_drop(ctx, catalog, YAVIEW_KEY, yaview);
        }
        pdf_obj* marks = pdf_dict_gets(ctx, yaview, MARKS_KEY);
        if (!pdf_is_dict(ctx, marks)) {
            if (!create) return nullptr;
            marks = pdf_dict_puts_dict(ctx, yaview, MARKS_KEY, 8);
        }
        return marks;
    }

    const char* yaview_error_message(fz_context* ctx) {
        return fz_caught_message(ctx);
    }
}

pdf_document* Document::pdf() {
    return doc ? pdf_specifics(context, doc) : nullptr;
}

bool Document::can_annotate() {
    return pdf() != nullptr;
}

DocumentPos Document::pdf_space_to_page_pos(DocumentPos pos) {
    pdf_document* pdf_doc = pdf();
    if (!pdf_doc) return pos;
    fz_point point = fz_make_point(pos.x, pos.y);
    fz_try(context) {
        point = fz_transform_point(point, page_transform(context, pdf_doc, pos.page));
    }
    fz_catch(context) {
    }
    return DocumentPos{ pos.page, point.x, point.y };
}

DocumentPos Document::page_pos_to_pdf_space(DocumentPos pos) {
    pdf_document* pdf_doc = pdf();
    if (!pdf_doc) return pos;
    fz_point point = fz_make_point(pos.x, pos.y);
    fz_try(context) {
        point = fz_transform_point(point, fz_invert_matrix(page_transform(context, pdf_doc, pos.page)));
    }
    fz_catch(context) {
    }
    return DocumentPos{ pos.page, point.x, point.y };
}

// Appends the changes to the file. An incremental save writes only the changed objects, after the
// original contents, which stay as they were. MuPDF expects one incremental save per loaded document:
// every save truncates the file to its original size and writes all of the session's changes again,
// but the /Prev of the written trailer would point at the previous save's xref, which is gone. Resetting
// startxref makes every save's section continue the file's own last xref instead.
bool Document::save_annotations() {
    pdf_document* pdf_doc = pdf();
    if (!pdf_doc || !pdf_has_unsaved_changes(context, pdf_doc)) {
        return pdf_doc != nullptr;
    }
    if (original_startxref < 0) {
        original_startxref = pdf_doc->startxref;
    }
    pdf_doc->startxref = original_startxref;
    pdf_write_options options = pdf_default_write_options;
    options.do_incremental = 1;
    std::string path = utf8_encode(file_name);
    bool saved = true;
    fz_var(saved);
    fz_try(context) {
        pdf_save_document(context, pdf_doc, path.c_str(), &options);
    }
    fz_catch(context) {
        saved = false;
    }
    if (!saved) {
        show_error_message(L"Could not save the annotations to " + file_name + L": " + utf8_decode(yaview_error_message(context)));
    }
    // the save changed the file's modification time; it is not a change of the document to reload it for
    last_update_time = QDateTime::currentDateTime();
    return saved;
}

fz_page* Document::load_pdf_page(int page_number, pdf_page** out_pdf_page) {
    fz_page* page = nullptr;
    fz_try(context) {
        page = fz_load_page(context, doc, page_number);
    }
    fz_catch(context) {
        page = nullptr;
    }
    *out_pdf_page = page ? pdf_page_from_fz_page(context, page) : nullptr;
    if (page && !*out_pdf_page) {
        fz_drop_page(context, page);
        page = nullptr;
    }
    return page;
}

// Reads the annotations of a page into yaview's structures. page_width and page_y_offset place the page in
// absolute document coordinates (the page dimensions of the document are not loaded yet when this runs).
void Document::read_page_annotations(fz_context* ctx,
    fz_page* page,
    int page_number,
    float page_width,
    float page_y_offset,
    std::vector<BookMark>& out_bookmarks,
    std::vector<Highlight>& out_highlights,
    std::vector<Portal>& out_portals) {

    pdf_page* pdf_page = pdf_page_from_fz_page(ctx, page);
    if (!pdf_page) return;
    pdf_document* pdf_doc = pdf_page->doc;

    fz_matrix ctm;
    pdf_page_transform(ctx, pdf_page, nullptr, &ctm);

    auto to_absolute = [&](fz_rect rect) {
        rect = fz_transform_rect(rect, ctm);
        return AbsoluteRect(
            AbsoluteDocumentPos{ rect.x0 - page_width / 2, rect.y0 + page_y_offset },
            AbsoluteDocumentPos{ rect.x1 - page_width / 2, rect.y1 + page_y_offset });
    };

    pdf_obj* annots = pdf_dict_get(ctx, pdf_page->obj, PDF_NAME(Annots));
    int num_annots = pdf_array_len(ctx, annots);

    for (int i = 0; i < num_annots; i++) {
        pdf_obj* annot = pdf_array_get(ctx, annots, i);
        int object_number = pdf_to_num(ctx, annot);
        // a direct annotation object can not be found again to change it; they are practically nonexistent
        if (!pdf_is_dict(ctx, annot) || object_number == 0) continue;

        fz_try(ctx) {
            pdf_obj* subtype = pdf_dict_get(ctx, annot, PDF_NAME(Subtype));
            int64_t creation_date = read_date(ctx, annot, PDF_NAME(CreationDate));
            int64_t modification_date = read_date(ctx, annot, PDF_NAME(M));
            const char* contents = pdf_dict_get_text_string(ctx, annot, PDF_NAME(Contents));
            float color[3] = { 0, 0, 0 };

            bool is_highlight = pdf_name_eq(ctx, subtype, PDF_NAME(Highlight));
            bool is_underline = pdf_name_eq(ctx, subtype, PDF_NAME(Underline));
            bool is_strikeout = pdf_name_eq(ctx, subtype, PDF_NAME(StrikeOut));

            if (is_highlight || is_underline || is_strikeout) {
                pdf_obj* quad_points = pdf_dict_get(ctx, annot, PDF_NAME(QuadPoints));
                int num_quads = pdf_array_len(ctx, quad_points) / 8;
                if (num_quads > 0) {
                    Highlight highlight;
                    highlight.pdf_object = object_number;
                    for (int q = 0; q < num_quads; q++) {
                        highlight.highlight_rects.push_back(to_absolute(fz_rect_from_quad(pdf_to_quad(ctx, quad_points, q * 8))));
                    }
                    const AbsoluteRect& first = highlight.highlight_rects.front();
                    const AbsoluteRect& last = highlight.highlight_rects.back();
                    highlight.selection_begin = { first.x0, (first.y0 + first.y1) / 2 };
                    highlight.selection_end = { last.x1, (last.y0 + last.y1) / 2 };
                    highlight.text_annot = utf8_decode(contents);
                    highlight.type = read_color(ctx, annot, PDF_NAME(C), color) ? get_highlight_color_type(color) : 'a';
                    if (is_underline) highlight.type = highlight.type - 'a' + 'A';
                    if (is_strikeout) highlight.type = '_';
                    highlight.set_times(creation_date, modification_date);
                    out_highlights.push_back(highlight);
                }
            }
            else if (pdf_name_eq(ctx, subtype, PDF_NAME(Text))) {
                AbsoluteRect rect = to_absolute(pdf_dict_get_rect(ctx, annot, PDF_NAME(Rect)));
                BookMark bookmark;
                bookmark.pdf_object = object_number;
                bookmark.description = utf8_decode(contents);
                bookmark.begin_x = (rect.x0 + rect.x1) / 2;
                bookmark.begin_y = (rect.y0 + rect.y1) / 2;
                bookmark.set_times(creation_date, modification_date);
                out_bookmarks.push_back(bookmark);
            }
            else if (pdf_name_eq(ctx, subtype, PDF_NAME(FreeText))) {
                AbsoluteRect rect = to_absolute(pdf_dict_get_rect(ctx, annot, PDF_NAME(Rect)));
                BookMark bookmark;
                bookmark.pdf_object = object_number;
                bookmark.description = utf8_decode(contents);
                bookmark.begin_x = rect.x0;
                bookmark.begin_y = rect.y0;
                bookmark.end_x = rect.x1;
                bookmark.end_y = rect.y1;
                const char* font = nullptr;
                int num_components = 0;
                float text_color[4] = { 0, 0, 0, 0 };
                pdf_parse_default_appearance(ctx, pdf_to_str_buf(ctx, pdf_dict_get_inheritable(ctx, annot, PDF_NAME(DA))), &font, &bookmark.font_size, &num_components, text_color);
                if (num_components == 3) {
                    std::copy(text_color, text_color + 3, bookmark.color);
                }
                else if (num_components == 1) {
                    bookmark.color[0] = bookmark.color[1] = bookmark.color[2] = text_color[0];
                }
                if (bookmark.font_size <= 0) bookmark.font_size = -1;
                bookmark.set_times(creation_date, modification_date);
                out_bookmarks.push_back(bookmark);
            }
            else if (pdf_name_eq(ctx, subtype, PDF_NAME(Square))) {
                AbsoluteRect rect = to_absolute(pdf_dict_get_rect(ctx, annot, PDF_NAME(Rect)));
                BookMark bookmark;
                bookmark.pdf_object = object_number;
                bookmark.description = utf8_decode(contents);
                // a box bookmark's description starts with '#' and its type; give one to boxes made elsewhere
                if (bookmark.description.empty() || bookmark.description[0] != '#') {
                    char type = read_color(ctx, annot, PDF_NAME(C), color) ? get_highlight_color_type(color) : 'a';
                    std::wstring prefix = L"#";
                    prefix.push_back(type);
                    bookmark.description = bookmark.description.empty() ? prefix : prefix + L" " + bookmark.description;
                }
                bookmark.begin_x = rect.x0;
                bookmark.begin_y = rect.y0;
                bookmark.end_x = rect.x1;
                bookmark.end_y = rect.y1;
                bookmark.set_times(creation_date, modification_date);
                out_bookmarks.push_back(bookmark);
            }
            else if (pdf_name_eq(ctx, subtype, PDF_NAME(Link)) && pdf_dict_gets(ctx, annot, YAVIEW_KEY)) {
                pdf_obj* action = pdf_dict_get(ctx, annot, PDF_NAME(A));
                pdf_obj* action_type = pdf_dict_get(ctx, action, PDF_NAME(S));
                bool remote = pdf_name_eq(ctx, action_type, PDF_NAME(GoToR));
                if (remote || pdf_name_eq(ctx, action_type, PDF_NAME(GoTo))) {
                    pdf_obj* dest = pdf_dict_get(ctx, action, PDF_NAME(D));
                    if (!remote && !pdf_is_array(ctx, dest)) {
                        // a named destination
                        dest = pdf_lookup_dest(ctx, pdf_doc, dest);
                    }
                    std::optional<Destination> destination = read_destination(ctx, pdf_doc, dest, remote);
                    if (destination) {
                        AbsoluteRect rect = to_absolute(pdf_dict_get_rect(ctx, annot, PDF_NAME(Rect)));
                        Portal portal;
                        portal.pdf_object = object_number;
                        portal.src_pos = { (rect.x0 + rect.x1) / 2, (rect.y0 + rect.y1) / 2 };
                        portal.dst.document_path = remote ? portal_destination_path(read_file_spec(ctx, pdf_dict_get(ctx, action, PDF_NAME(F)))) : file_name;
                        portal.dst.pos = { destination->page, destination->x.value_or(0), destination->y.value_or(0) };
                        portal.dst.zoom_level = destination->zoom_level.value_or(-1);
                        portal.set_times(creation_date, modification_date);
                        out_portals.push_back(portal);
                    }
                }
            }
        }
        fz_catch(ctx) {
            // a damaged annotation; the others are still read
        }
    }
}

void Document::read_marks(fz_context* ctx, pdf_document* pdf_doc, const std::vector<float>& widths, const std::vector<float>& y_offsets, std::vector<Mark>& out_marks) {
    fz_try(ctx) {
        pdf_obj* marks_dict = marks_dictionary(ctx, pdf_doc, false);
        int num_marks = pdf_dict_len(ctx, marks_dict);
        for (int i = 0; i < num_marks; i++) {
            const char* name = pdf_to_name(ctx, pdf_dict_get_key(ctx, marks_dict, i));
            std::optional<Destination> destination = read_destination(ctx, pdf_doc, pdf_dict_get_val(ctx, marks_dict, i), false);
            if (strlen(name) != 1 || !destination || !destination->y || destination->page >= widths.size()) continue;

            fz_point point = fz_transform_point(fz_make_point(destination->x.value_or(0), destination->y.value()), page_transform(ctx, pdf_doc, destination->page));
            Mark mark;
            mark.symbol = name[0];
            pdf_obj* entry = pdf_dict_get_val(ctx, marks_dict, i);
            if (pdf_is_dict(ctx, entry)) {
                mark.description = utf8_decode(pdf_dict_get_text_string(ctx, entry, PDF_NAME(Title)));
            }
            mark.y_offset = point.y + y_offsets[destination->page];
            if (destination->x) {
                mark.x_offset = point.x - widths[destination->page] / 2;
            }
            mark.zoom_level = destination->zoom_level;
            out_marks.push_back(mark);
        }
    }
    fz_catch(ctx) {
    }
}

// A destination array for `pos`, which is in PDF user space. A remote destination (in another file)
// refers to its page by index, a local one by the page object.
pdf_obj* Document::new_destination(DocumentPos pos, std::optional<float> x, std::optional<float> zoom_level, bool remote) {
    pdf_document* pdf_doc = pdf();
    pdf_obj* dest = pdf_new_array(context, pdf_doc, 5);
    if (remote) {
        pdf_array_push_int(context, dest, pos.page);
    }
    else {
        pdf_array_push(context, dest, pdf_lookup_page_obj(context, pdf_doc, pos.page));
    }
    pdf_array_push(context, dest, PDF_NAME(XYZ));
    if (x) pdf_array_push_real(context, dest, x.value()); else pdf_array_push(context, dest, PDF_NULL);
    pdf_array_push_real(context, dest, pos.y);
    if (zoom_level && zoom_level.value() > 0) pdf_array_push_real(context, dest, zoom_level.value()); else pdf_array_push(context, dest, PDF_NULL);
    return dest;
}

// Creates an annotation of `type` on `page_number`, filled in by `fill`, and saves the file. Returns the
// annotation's object number, or 0 when it could not be created.
int Document::write_annotation(int page_number, enum pdf_annot_type type, const Annotation& annotation, std::function<void(pdf_annot*)> fill) {
    pdf_page* pdf_page = nullptr;
    fz_page* page = load_pdf_page(page_number, &pdf_page);
    if (!page) return 0;

    int object_number = 0;
    pdf_annot* annot = nullptr;
    fz_var(object_number);
    fz_var(annot);
    fz_try(context) {
        annot = pdf_create_annot(context, pdf_page, type);
        fill(annot);
        pdf_set_annot_creation_date(context, annot, annotation_time_secs(annotation.creation_time));
        pdf_set_annot_modification_date(context, annot, annotation_time_secs(annotation.modification_time));
        // the appearance stream makes the annotation visible in readers that don't draw annotations themselves
        pdf_update_annot(context, annot);
        object_number = pdf_to_num(context, pdf_annot_obj(context, annot));
    }
    fz_always(context) {
        pdf_drop_annot(context, annot);
        fz_drop_page(context, page);
    }
    fz_catch(context) {
        show_error_message(L"Could not add the annotation to " + file_name + L": " + utf8_decode(yaview_error_message(context)));
        object_number = 0;
    }
    if (object_number) {
        save_annotations();
    }
    return object_number;
}

// Changes the annotation with object number `pdf_object` on `page_number` through `change` and saves the file.
bool Document::update_annotation(int page_number, int pdf_object, std::function<void(pdf_annot*)> change) {
    pdf_page* pdf_page = nullptr;
    fz_page* page = load_pdf_page(page_number, &pdf_page);
    if (!page) return false;

    bool found = false;
    fz_var(found);
    fz_try(context) {
        for (pdf_annot* annot = pdf_first_annot(context, pdf_page); annot; annot = pdf_next_annot(context, annot)) {
            if (pdf_to_num(context, pdf_annot_obj(context, annot)) == pdf_object) {
                change(annot);
                pdf_set_annot_modification_date(context, annot, current_time_secs());
                pdf_update_annot(context, annot);
                found = true;
                break;
            }
        }
    }
    fz_always(context) {
        fz_drop_page(context, page);
    }
    fz_catch(context) {
        show_error_message(L"Could not change the annotation in " + file_name + L": " + utf8_decode(yaview_error_message(context)));
        found = false;
    }
    return found && save_annotations();
}

// Deletes the annotation with object number `pdf_object` from `page_number` and saves the file. Link
// annotations (portals) are not in MuPDF's annotation list and are removed from the page's /Annots array.
bool Document::delete_annotation(int page_number, int pdf_object) {
    pdf_page* pdf_page = nullptr;
    fz_page* page = load_pdf_page(page_number, &pdf_page);
    if (!page) return false;

    bool found = false;
    fz_var(found);
    fz_try(context) {
        for (pdf_annot* annot = pdf_first_annot(context, pdf_page); annot; annot = pdf_next_annot(context, annot)) {
            if (pdf_to_num(context, pdf_annot_obj(context, annot)) == pdf_object) {
                pdf_delete_annot(context, pdf_page, annot);
                found = true;
                break;
            }
        }
        if (!found) {
            pdf_obj* annots = pdf_dict_get(context, pdf_page->obj, PDF_NAME(Annots));
            int num_annots = pdf_array_len(context, annots);
            for (int i = 0; i < num_annots; i++) {
                if (pdf_to_num(context, pdf_array_get(context, annots, i)) == pdf_object) {
                    pdf_array_delete(context, annots, i);
                    found = true;
                    break;
                }
            }
        }
    }
    fz_always(context) {
        fz_drop_page(context, page);
    }
    fz_catch(context) {
        show_error_message(L"Could not delete the annotation from " + file_name + L": " + utf8_decode(yaview_error_message(context)));
        found = false;
    }
    return found && save_annotations();
}

int Document::write_highlight(const Highlight& highlight) {
    DocumentPos begin = absolute_to_page_pos(highlight.selection_begin);
    std::vector<fz_quad> quads;
    for (const AbsoluteRect& rect : highlight.highlight_rects) {
        quads.push_back(fz_quad_from_rect(rect.to_document(this).rect));
    }
    enum pdf_annot_type type = PDF_ANNOT_HIGHLIGHT;
    if (highlight.type == '_') type = PDF_ANNOT_STRIKE_OUT;
    else if (std::isupper(highlight.type)) type = PDF_ANNOT_UNDERLINE;
    std::string note = utf8_encode(highlight.text_annot);

    return write_annotation(begin.page, type, highlight, [&](pdf_annot* annot) {
        pdf_set_annot_quad_points(context, annot, quads.size(), quads.data());
        pdf_set_annot_color(context, annot, 3, get_highlight_type_color(highlight.type));
        if (note.size() > 0) {
            pdf_set_annot_contents(context, annot, note.c_str());
        }
        });
}

int Document::write_bookmark(const BookMark& bookmark) {
    DocumentPos begin = absolute_to_page_pos(bookmark.begin_pos());
    std::string description = utf8_encode(bookmark.description);
    PagelessDocumentRect rect;
    enum pdf_annot_type type;

    if (bookmark.is_marked()) {
        type = PDF_ANNOT_TEXT;
        rect = fz_make_rect(begin.x - BOOKMARK_RECT_SIZE, begin.y - BOOKMARK_RECT_SIZE, begin.x + BOOKMARK_RECT_SIZE, begin.y + BOOKMARK_RECT_SIZE);
    }
    else {
        type = bookmark.is_box() ? PDF_ANNOT_SQUARE : PDF_ANNOT_FREE_TEXT;
        rect = bookmark.rect().to_document(this).rect;
    }

    return write_annotation(begin.page, type, bookmark, [&](pdf_annot* annot) {
        pdf_set_annot_rect(context, annot, rect);
        pdf_set_annot_contents(context, annot, description.c_str());
        if (type == PDF_ANNOT_SQUARE) {
            std::optional<char> box_type = bookmark.get_type();
            pdf_set_annot_color(context, annot, 3, get_highlight_type_color(box_type.value_or('a')));
            pdf_set_annot_border_width(context, annot, 1);
        }
        else if (type == PDF_ANNOT_FREE_TEXT) {
            pdf_set_annot_default_appearance(context, annot, "Helv", bookmark.font_size, 3, bookmark.color);
        }
        });
}

int Document::write_portal(const Portal& portal) {
    DocumentPos src = absolute_to_page_pos(portal.src_pos);
    pdf_page* pdf_page = nullptr;
    fz_page* page = load_pdf_page(src.page, &pdf_page);
    if (!page) return 0;

    int object_number = 0;
    pdf_obj* link = nullptr;
    fz_var(object_number);
    fz_var(link);
    fz_try(context) {
        fz_matrix ctm;
        pdf_page_transform(context, pdf_page, nullptr, &ctm);
        fz_rect rect = fz_make_rect(src.x - BOOKMARK_RECT_SIZE, src.y - BOOKMARK_RECT_SIZE, src.x + BOOKMARK_RECT_SIZE, src.y + BOOKMARK_RECT_SIZE);
        rect = fz_transform_rect(rect, fz_invert_matrix(ctm));

        link = pdf_add_new_dict(context, pdf_page->doc, 8);
        pdf_dict_put(context, link, PDF_NAME(Type), PDF_NAME(Annot));
        pdf_dict_put(context, link, PDF_NAME(Subtype), PDF_NAME(Link));
        pdf_dict_put_rect(context, link, PDF_NAME(Rect), rect);
        pdf_dict_puts_drop(context, link, YAVIEW_KEY, pdf_new_name(context, "Portal"));
        pdf_dict_put_date(context, link, PDF_NAME(CreationDate), annotation_time_secs(portal.creation_time));
        pdf_dict_put_date(context, link, PDF_NAME(M), annotation_time_secs(portal.modification_time));
        // no border: yaview draws the portal icon, other readers show a plain link
        pdf_obj* border = pdf_dict_put_array(context, link, PDF_NAME(Border), 3);
        for (int i = 0; i < 3; i++) pdf_array_push_int(context, border, 0);

        write_portal_action(link, portal);

        pdf_obj* annots = pdf_dict_get(context, pdf_page->obj, PDF_NAME(Annots));
        if (!pdf_is_array(context, annots)) {
            annots = pdf_dict_put_array(context, pdf_page->obj, PDF_NAME(Annots), 1);
        }
        pdf_array_push(context, annots, link);
        object_number = pdf_to_num(context, link);
    }
    fz_always(context) {
        pdf_drop_obj(context, link);
        fz_drop_page(context, page);
    }
    fz_catch(context) {
        show_error_message(L"Could not add the portal to " + file_name + L": " + utf8_decode(yaview_error_message(context)));
        object_number = 0;
    }
    if (object_number) {
        save_annotations();
    }
    return object_number;
}

// The action of a portal's link annotation: GoTo its destination in this file, or GoToR in another file.
void Document::write_portal_action(pdf_obj* link, const Portal& portal) {
    bool remote = portal.dst.document_path != file_name;
    std::string file_spec = remote ? portal_destination_file_spec(portal.dst.document_path) : "";
    pdf_obj* action = pdf_dict_put_dict(context, link, PDF_NAME(A), 3);
    pdf_dict_put(context, action, PDF_NAME(S), remote ? PDF_NAME(GoToR) : PDF_NAME(GoTo));
    pdf_dict_put_drop(context, action, PDF_NAME(D), new_destination(portal.dst.pos, portal.dst.pos.x, portal.dst.zoom_level, remote));
    if (remote) {
        // a file specification: the path with ASCII only in /F and as it is in /UF
        pdf_obj* filespec = pdf_dict_put_dict(context, action, PDF_NAME(F), 3);
        std::string ascii_spec = file_spec;
        for (char& c : ascii_spec) {
            if (c < 32 || c > 126) c = '_';
        }
        pdf_dict_put(context, filespec, PDF_NAME(Type), PDF_NAME(Filespec));
        pdf_dict_put_text_string(context, filespec, PDF_NAME(F), ascii_spec.c_str());
        pdf_dict_put_text_string(context, filespec, PDF_NAME(UF), file_spec.c_str());
    }
}

// Changes a portal's link annotation (found by its object number, no page needed) through `change`.
bool Document::update_portal_link(int pdf_object, std::function<void(pdf_obj*)> change) {
    pdf_document* pdf_doc = pdf();
    if (!pdf_doc) return false;
    bool changed = true;
    pdf_obj* link = nullptr;
    fz_var(changed);
    fz_var(link);
    fz_try(context) {
        link = pdf_load_object(context, pdf_doc, pdf_object);
        change(link);
        pdf_dict_put_date(context, link, PDF_NAME(M), current_time_secs());
    }
    fz_always(context) {
        pdf_drop_obj(context, link);
    }
    fz_catch(context) {
        show_error_message(L"Could not change the portal in " + file_name + L": " + utf8_decode(yaview_error_message(context)));
        changed = false;
    }
    return changed && save_annotations();
}

bool Document::write_mark(const Mark& mark) {
    pdf_document* pdf_doc = pdf();
    if (!pdf_doc) return false;
    DocumentPos pos = page_pos_to_pdf_space(absolute_to_page_pos({ mark.x_offset.value_or(0), mark.y_offset }));
    std::string name(1, mark.symbol);
    std::string description = utf8_encode(mark.description);
    bool written = true;
    fz_var(written);
    fz_try(context) {
        pdf_obj* marks_dict = marks_dictionary(context, pdf_doc, true);
        pdf_obj* entry = pdf_dict_puts_dict(context, marks_dict, name.c_str(), 2);
        pdf_dict_put_drop(context, entry, PDF_NAME(D), new_destination(pos, mark.x_offset ? std::optional<float>(pos.x) : std::nullopt, mark.zoom_level, false));
        if (description.size() > 0) {
            pdf_dict_put_text_string(context, entry, PDF_NAME(Title), description.c_str());
        }
    }
    fz_catch(context) {
        show_error_message(L"Could not save the mark to " + file_name + L": " + utf8_decode(yaview_error_message(context)));
        written = false;
    }
    return written && save_annotations();
}

bool Document::erase_marks(std::optional<char> symbol) {
    pdf_document* pdf_doc = pdf();
    if (!pdf_doc) return false;
    bool erased = true;
    fz_var(erased);
    fz_try(context) {
        pdf_obj* marks_dict = marks_dictionary(context, pdf_doc, false);
        if (marks_dict) {
            if (symbol) {
                std::string name(1, symbol.value());
                pdf_dict_dels(context, marks_dict, name.c_str());
            }
            else {
                pdf_dict_dels(context, pdf_dict_gets(context, pdf_dict_get(context, pdf_trailer(context, pdf_doc), PDF_NAME(Root)), YAVIEW_KEY), MARKS_KEY);
            }
        }
    }
    fz_catch(context) {
        show_error_message(L"Could not remove the mark from " + file_name + L": " + utf8_decode(yaview_error_message(context)));
        erased = false;
    }
    return erased && save_annotations();
}

// The path of a portal's destination file, from the file specification in its action: relative
// specifications are relative to this document's folder.
std::wstring Document::portal_destination_path(const std::string& file_spec) {
    if (file_spec.empty()) return file_name;
    QDir folder = QFileInfo(QString::fromStdWString(file_name)).absoluteDir();
    return QDir::cleanPath(folder.absoluteFilePath(QString::fromStdString(file_spec))).toStdWString();
}

std::string Document::portal_destination_file_spec(const std::wstring& document_path) {
    QDir folder = QFileInfo(QString::fromStdWString(file_name)).absoluteDir();
    return folder.relativeFilePath(QString::fromStdWString(document_path)).toStdString();
}

// Reads a document's bookmarks and highlights without keeping it open (for the lists across all
// documents). An open document's annotations are already in memory.
bool Document::read_annotations_from_file(std::vector<BookMark>& out_bookmarks, std::vector<Highlight>& out_highlights, std::vector<int>* out_highlight_pages) {
    if (doc && are_highlights_loaded) {
        load_highlight_texts();
        out_bookmarks = bookmarks;
        out_highlights = highlights;
        if (out_highlight_pages) {
            for (const Highlight& highlight : highlights) {
                out_highlight_pages->push_back(absolute_to_page_pos(highlight.selection_begin).page);
            }
        }
        return true;
    }
    if (!QFileInfo(QString::fromStdWString(file_name)).exists()) {
        return false;
    }
    bool read = true;
    fz_var(read);
    fz_document* file_doc = nullptr;
    fz_var(file_doc);
    fz_try(context) {
        file_doc = open_document_with_file_name(context, file_name, db_manager);
        int n = fz_count_pages(context, file_doc);
        float y_offset = 0;
        std::vector<float> widths;
        std::vector<float> y_offsets;
        std::vector<Portal> ignored_portals;
        for (int i = 0; i < n; i++) {
            fz_page* page = fz_load_page(context, file_doc, i);
            fz_rect bounds = fz_bound_page(context, page);
            size_t num_highlights = out_highlights.size();
            read_page_annotations(context, page, i, bounds.x1 - bounds.x0, y_offset, out_bookmarks, out_highlights, ignored_portals);
            if (out_highlight_pages) {
                out_highlight_pages->resize(out_highlights.size(), i);
            }
            widths.push_back(bounds.x1 - bounds.x0);
            y_offsets.push_back(y_offset);
            y_offset += bounds.y1 - bounds.y0;
            fz_drop_page(context, page);
        }
        // the highlighted texts come from the pages' text; the page dimensions place the highlights on them
        if (!doc) {
            page_widths = widths;
            accum_page_heights = y_offsets;
            for (Highlight& highlight : out_highlights) {
                std::deque<AbsoluteRect> selected_characters;
                get_text_selection(context, highlight.selection_begin, highlight.selection_end, !EXACT_HIGHLIGHT_SELECT, selected_characters, highlight.description, file_doc);
            }
            page_widths.clear();
            accum_page_heights.clear();
        }
    }
    fz_always(context) {
        fz_drop_document(context, file_doc);
    }
    fz_catch(context) {
        read = false;
    }
    return read;
}
