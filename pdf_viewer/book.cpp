#include "book.h"
#include "utils.h"
#include "document.h"

extern float BOOKMARK_RECT_SIZE;

bool operator==(const DocumentViewState& lhs, const DocumentViewState& rhs)
{
    return (lhs.book_state.offset_x == rhs.book_state.offset_x) &&
        (lhs.book_state.offset_y == rhs.book_state.offset_y) &&
        (lhs.book_state.zoom_level == rhs.book_state.zoom_level) &&
        (lhs.document_path == rhs.document_path);
}

bool operator==(const CachedPageData& lhs, const CachedPageData& rhs) {
    if (lhs.doc != rhs.doc) return false;
    if (lhs.page != rhs.page) return false;
    if (lhs.zoom_level != rhs.zoom_level) return false;
    return true;
}

QJsonObject Mark::to_json() const
{
    QJsonObject res;
    res["y_offset"] = y_offset;
    res["symbol"] = symbol;
    res["description"] = QString::fromStdWString(description);
    if (x_offset) {
        res["x_offset"] = x_offset.value();
    }
    if (zoom_level) {
        res["zoom_level"] = zoom_level.value();
    }
    return res;
}

void Annotation::add_metadata_to_json(QJsonObject& obj) const {
    obj["creation_time"] = QString::fromStdString(creation_time);
    obj["modification_time"] = QString::fromStdString(modification_time);
    obj["pdf_object"] = pdf_object;
}

QDateTime Annotation::get_creation_datetime() const {
    return QDateTime::fromString(QString::fromStdString(creation_time), "yyyy-MM-dd HH:mm:ss");
}

QDateTime Annotation::get_modification_datetime() const {
    return QDateTime::fromString(QString::fromStdString(modification_time), "yyyy-MM-dd HH:mm:ss");
}

void Annotation::update_creation_time() {
    creation_time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();
    update_modification_time();
}

void Annotation::update_modification_time() {
    modification_time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();
}

void Annotation::set_times(int64_t creation_secs, int64_t modification_secs) {
    // the PDF dates are optional; a missing modification date is the creation date and vice versa
    if (creation_secs < 0) creation_secs = modification_secs;
    if (modification_secs < 0) modification_secs = creation_secs;
    creation_time = creation_secs < 0 ? "" : QDateTime::fromSecsSinceEpoch(creation_secs).toString("yyyy-MM-dd HH:mm:ss").toStdString();
    modification_time = modification_secs < 0 ? "" : QDateTime::fromSecsSinceEpoch(modification_secs).toString("yyyy-MM-dd HH:mm:ss").toStdString();
}

QJsonObject BookMark::to_json() const
{
    QJsonObject res;
    res["y_offset"] = get_y_offset();
    res["description"] = QString::fromStdWString(description);
    res["begin_x"] = begin_x;
    res["begin_y"] = begin_y;
    res["end_x"] = end_x;
    res["end_y"] = end_y;

    if (is_freetext()) {
        res["color_red"] = color[0];
        res["color_green"] = color[1];
        res["color_blue"] = color[2];
        res["font_size"] = font_size;
    }

    add_metadata_to_json(res);

    return res;

}

bool BookMark::is_freetext() const {
    return (begin_y > -1) && (end_y > -1);
}

std::optional<char> BookMark::get_type() const{
    if (is_box()) {
        if (description.size() > 1) {
            return description[1];
        }
    }
    return {};
}

bool BookMark::is_box() const {
    if (description.size() > 0) {
        return description[0] == '#';
    }
    return false;
}

bool BookMark::is_marked() const {
    return (begin_y > -1) && (end_y == -1);
}

QJsonObject Highlight::to_json() const
{
    QJsonObject res;
    res["selection_begin_x"] = selection_begin.x;
    res["selection_begin_y"] = selection_begin.y;
    res["selection_end_x"] = selection_end.x;
    res["selection_end_y"] = selection_end.y;
    res["description"] = QString::fromStdWString(description);
    res["text_annot"] = QString::fromStdWString(text_annot);
    res["type"] = type;
    add_metadata_to_json(res);
    return res;
}

QJsonObject Portal::to_json() const
{
    QJsonObject res;
    res["src_offset_x"] = src_pos.x;
    res["src_offset_y"] = src_pos.y;
    res["dst_document_path"] = QString::fromStdWString(dst.document_path);
    res["dst_page"] = dst.pos.page;
    res["dst_x"] = dst.pos.x;
    res["dst_y"] = dst.pos.y;
    res["dst_zoom_level"] = dst.zoom_level;
    add_metadata_to_json(res);
    return res;
}

OpenedBookState PortalViewState::book_state(Document* destination_document) const {
    OpenedBookState res;
    AbsoluteDocumentPos abspos = destination_document->document_to_absolute_pos(pos);
    res.offset_x = abspos.x;
    res.offset_y = abspos.y;
    res.zoom_level = zoom_level;
    return res;
}

bool operator==(const Mark& lhs, const Mark& rhs)
{
    return (lhs.symbol == rhs.symbol) && (lhs.y_offset == rhs.y_offset);
}

bool operator==(const BookMark& lhs, const BookMark& rhs)
{
    return  (lhs.begin_y == rhs.begin_y) && (lhs.description == rhs.description);
}

bool operator==(const fz_point& lhs, const fz_point& rhs) {
    return (lhs.y == rhs.y) && (lhs.x == rhs.x);
}

bool operator==(const Highlight& lhs, const Highlight& rhs)
{
    return  (lhs.selection_begin.x == rhs.selection_begin.x) && (lhs.selection_end.x == rhs.selection_end.x) &&
        (lhs.selection_begin.y == rhs.selection_begin.y) && (lhs.selection_end.y == rhs.selection_end.y);
}

bool operator==(const Portal& lhs, const Portal& rhs)
{
    return  (lhs.src_pos.y == rhs.src_pos.y) && (lhs.dst.document_path == rhs.dst.document_path);
}

AbsoluteRect BookMark::get_rectangle() const{
    if (end_y > -1) {

        return AbsoluteRect(
            AbsoluteDocumentPos{ begin_x, begin_y },
            AbsoluteDocumentPos{ end_x, end_y }
        );
    }
    else {
        return AbsoluteRect(
            AbsoluteDocumentPos{ begin_x - BOOKMARK_RECT_SIZE, begin_y - BOOKMARK_RECT_SIZE },
            AbsoluteDocumentPos{ begin_x + BOOKMARK_RECT_SIZE, begin_y + BOOKMARK_RECT_SIZE }
        );
    }
}

AbsoluteRect Portal::get_rectangle() const{

    return AbsoluteRect(
        AbsoluteDocumentPos{ src_pos.x - BOOKMARK_RECT_SIZE, src_pos.y - BOOKMARK_RECT_SIZE},
        AbsoluteDocumentPos{ src_pos.x + BOOKMARK_RECT_SIZE, src_pos.y + BOOKMARK_RECT_SIZE}
    );
}

float BookMark::get_y_offset() const{
    return begin_y;
}

AbsoluteDocumentPos BookMark::begin_pos() const {
    return AbsoluteDocumentPos{ begin_x, begin_y };
}

AbsoluteDocumentPos BookMark::end_pos() const {
    return AbsoluteDocumentPos{ end_x, end_y };
}

AbsoluteRect BookMark::rect() const {
    return AbsoluteRect(begin_pos(), end_pos());
}

void SearchResult::fill(Document* doc) {
    if (rects.size() == 0) {
        doc->fill_search_result(this);
    }
}

std::string reference_type_string(ReferenceType rt) {
    if (rt == ReferenceType::None) return "none";
    if (rt == ReferenceType::Equation) return "equation";
    if (rt == ReferenceType::Reference) return "reference";
    if (rt == ReferenceType::Abbreviation) return "abbreviation";
    if (rt == ReferenceType::Generic) return "generic";
    if (rt == ReferenceType::Link) return "link";
    return "";
}
