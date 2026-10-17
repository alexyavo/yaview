<p align="center">
  <img src="resources/yaview-icon.png" alt="yaview icon" width="128">
</p>

<h1 align="center">yaview — a PDF Reader</h1>

<p align="center">
  <a href="https://github.com/alexyavo/yaview/actions/workflows/memory-tests.yml"><img alt="Memory leak tests" src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/alexyavo/yaview/badges/memory-tests.json"></a>
</p>

yaview is a fork of [sioyek](https://github.com/ahrm/sioyek).

lightweight, supports vim navigation.

### Basics

- <kbd>o</kbd> open a file, starting in the current document's folder.
- <kbd>Ctrl</kbd>+<kbd>o</kbd> open a file, starting in `default_open_file_path` (the home folder if unset).
- <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>o</kbd> open a file with the system's file dialog (starts in
  `default_open_file_path` if set).
- <kbd>Shift</kbd>+<kbd>o</kbd> open a previously opened document (in the list, <kbd>Cmd</kbd>+<kbd>Backspace</kbd> removes
  the highlighted one).
- `:clear_document_history` clear the list of previously opened documents, except the ones open in tabs. yaview also
  forgets where you were in the removed documents.
- <kbd>h</kbd> <kbd>j</kbd> <kbd>k</kbd> <kbd>l</kbd> navigation.
- <kbd>Ctrl</kbd>+<kbd>d</kbd> page down by half a screen (also: <kbd>Space</kbd>, <kbd>PageDown</kbd>)
- <kbd>Ctrl</kbd>+<kbd>u</kbd> page up by half a screen (also: <kbd>Shift</kbd>+<kbd>Space</kbd>, <kbd>PageUp</kbd>)
- <kbd>F10</kbd> fit the document to screen ignoring white page margins.
- <kbd>F9</kbd> fit the document to screen (including white margins).
- <kbd>F8</kbd> toggle dark mode.
- <kbd>gg</kbd> go to the first page.
- `[NUM]`<kbd>gg</kbd> go to page `[NUM]`
- <kbd>G</kbd> go to the last page.
- <kbd>t</kbd> open table of contents (if exists)
- <kbd>Backspace</kbd> go back in history (also: <kbd>Ctrl</kbd>+<kbd>←</kbd>, the mouse's back button, a trackpad
  swipe on macOS)
- <kbd>Shift</kbd>+<kbd>Backspace</kbd> go forward in history (also: <kbd>Ctrl</kbd>+<kbd>→</kbd>, the mouse's forward
  button, a trackpad swipe on macOS)
- <kbd>ss</kbd> search selected text in configured search engine.

  The default is Google, opened in the system's default browser. To use another engine, put `search_url_s` followed
  by the engine's search URL in `yaview.config` (see [Configuration](#configuration)); the selected text is
  appended to the URL. For example:

  ```
  search_url_s https://kagi.com/search?q=
  ```

- <kbd>:</kbd> for commands
- <kbd>/</kbd> to search. <kbd>n</kbd> / <kbd>Shift</kbd>+<kbd>n</kbd> to go to the next/previous match.

### Marks

- <kbd>m</kbd> + letter set a mark at the current location (e.g. <kbd>ma</kbd>). Lowercase marks belong to the
  document, uppercase marks are global (going to one opens its document). Marks are kept across restarts.
- <kbd>&#96;</kbd> + letter go to a mark (e.g. <kbd>&#96;</kbd><kbd>a</kbd>).
- `:show_marks` list the marks of all documents.
- `:clear_document_marks` delete the marks of the current document (including the uppercase ones set in it).
- `:clear_all_marks` delete the marks of all documents.
- Right click a line to put a ruler (visual mark) under it.
- <kbd>j</kbd> / <kbd>k</kbd> move the ruler to the next/previous line (also: <kbd>↓</kbd> / <kbd>↑</kbd>).
- <kbd>&#96;</kbd><kbd>&#96;</kbd> go back to the ruler's last location (also: <kbd>&#96;</kbd> then right click).
- <kbd>F7</kbd> toggle ruler scrolling, where the mouse wheel moves the ruler.

### Bookmarks

- <kbd>b</kbd> add a bookmark at the current location: type its text and press <kbd>Enter</kbd>. Selected text is
  filled in as the bookmark text.
- <kbd>gb</kbd> search the bookmarks of the current document.
- <kbd>gB</kbd> search the bookmarks of all documents.
- <kbd>db</kbd> delete the closest bookmark.

### Highlights

- <kbd>h</kbd> + lowercase letter highlight the selected text (e.g. <kbd>ha</kbd>). The letter is the highlight's type;
  each type has its own color (`highlight_color_<letter>` in `yaview.config`).
- <kbd>gh</kbd> search the highlights of the current document.
- <kbd>gH</kbd> search the highlights of all documents.
- <kbd>gnh</kbd> / <kbd>gNh</kbd> go to the next/previous highlight.
- <kbd>dh</kbd> delete the clicked highlight.


### Portals

- <kbd>p</kbd> start a portal at the current location; go to the destination (in any document) and press <kbd>p</kbd>
  again. Clicking a link after the first <kbd>p</kbd> uses the link's destination.
- <kbd>F12</kbd> toggle the helper window, which shows the destination of the closest portal. In it, the mouse wheel
  and dragging adjust the destination; <kbd>Cmd</kbd>/<kbd>Ctrl</kbd> + wheel and the zoom keys (<kbd>+</kbd> / <kbd>-</kbd>,
  <kbd>Cmd</kbd>+<kbd>=</kbd> / <kbd>Cmd</kbd>+<kbd>-</kbd>) zoom it.
- <kbd>Tab</kbd> go to the closest portal's destination (also: <kbd>gp</kbd>).
- <kbd>Shift</kbd>+<kbd>Tab</kbd> edit the closest portal: go to its destination, adjust it, and go back
  (<kbd>Backspace</kbd>) to save it (also: <kbd>P</kbd>).
- <kbd>dp</kbd> delete the closest portal.

### Configuration

- All configuration goes in one file, `~/.config/yaview/yaview.config`; yaview creates it on first start. `:config`
  opens it, and changes apply when you save it.
- One setting per line: `<name> <value>`. Colors are three or four numbers from 0 to 1 (`0.1 0.2 0.3`) or a hex value
  (`#1a1410`); on/off settings take `1` or `0`. Lines starting with `#` are comments.
- Key bindings: `keymap <command> <keys>`, with keys written as in the defaults: `T`, `gg`, `<C-d>` (Ctrl+D), `<D-w>`
  (Cmd+W), `<S-<tab>>` (Shift+Tab). A key that already has a default binding gets the new command.
- For example:

  ```
  ui_theme dark
  search_url_s https://kagi.com/search?q=
  keymap goto_toc T
  ```
- The defaults are built into yaview: `:prefs` shows the default settings, `:keys` the default key bindings.
- yaview saves window sizes and positions itself, in `~/Library/Application Support/yaview/auto.config`; those settings
  are marked "saved by yaview" below.

#### Appearance and colors

| Setting | Default | Meaning |
|---|---|---|
| `background_color` | `0.97 0.97 0.97` | Window background around the pages in the normal (light) color mode. |
| `dark_mode_background_color` | `0.0 0.0 0.0` | Window background around the pages in dark mode. |
| `custom_color_mode_empty_background_color` | `0 0 0` | Window background around the pages in custom color mode. |
| `custom_background_color` | `0.180 0.204 0.251` | Page background in custom color mode (`toggle_custom_color`). |
| `custom_text_color` | `0.847 0.871 0.914` | Page text color in custom color mode. |
| `custom_color_contrast` | `0.5` | Factor in the custom color transform that lightens red and blue content (range 0–1). |
| `dark_mode_contrast` | `0.8` | Contrast of dark mode; lower values dim the white text and lighten the black page (range 0–1). |
| `default_dark_mode` | `0` | Deprecated: start in dark mode, ignored when `use_system_theme` is 1; put `toggle_dark_mode` in `startup_commands` instead. |
| `use_system_theme` | `0` | Switch the document between light and dark mode with the system appearance. |
| `use_custom_color_as_dark_system_theme` | `0` | With `use_system_theme`, use custom color mode instead of dark mode while the system is dark. |
| `preserve_image_colors_in_dark_mode` | `0` | Keep images in their original colors in dark and custom color modes. |
| `inverted_preserved_image_colors` | `0` | In custom color mode, draw preserved images in dark-mode colors instead of their originals. |
| `adjust_annotation_colors_for_dark_mode` | `1` | Convert highlight, selection, search and bookmark colors to suit dark and custom color modes. |
| `fastread_opacity` | `0.5` | Opacity of the dimmed part of words in fast-read mode (`toggle_fastread`) (range 0–1). |

#### Popups and UI

| Setting | Default | Meaning |
|---|---|---|
| `ui_theme` | `auto` | Color theme of popups and the tab bar: `auto` (follows the system appearance), `dark`, `light` or `custom`. |
| `ui_dark_background_color` | `#1a1410` | Popup and tab bar background in the dark theme. |
| `ui_dark_text_color` | `#ebdbb2` | Popup and tab bar text in the dark theme. |
| `ui_dark_selected_background_color` | `#fe8019` | Background of the selected popup row in the dark theme. |
| `ui_dark_selected_text_color` | `#1a1410` | Text of the selected popup row in the dark theme. |
| `ui_dark_border_color` | `#d65d0e` | Popup border and selected tab outline in the dark theme. |
| `ui_light_background_color` | `#fbf1c7` | Popup and tab bar background in the light theme. |
| `ui_light_text_color` | `#3c3836` | Popup and tab bar text in the light theme. |
| `ui_light_selected_background_color` | `#af3a03` | Background of the selected popup row in the light theme. |
| `ui_light_selected_text_color` | `#fbf1c7` | Text of the selected popup row in the light theme. |
| `ui_light_border_color` | `#d65d0e` | Popup border and selected tab outline in the light theme. |
| `ui_background_color` | `0 0 0` | Popup background with `ui_theme custom`. |
| `ui_text_color` | `1 1 1` | Popup text with `ui_theme custom`. |
| `ui_selected_background_color` | `1 1 1` | Selected row background and border color with `ui_theme custom`. |
| `ui_selected_text_color` | `0 0 0` | Selected row text with `ui_theme custom`. |
| `ui_font` | `Monaspace Neon` | Font of popups and the tab bar; the bundled JetBrains Mono is used when it is not installed. |
| `font_size` | `18` | Font size in pixels of popup menus (range 1–100). |
| `menu_screen_width_ratio` | `0.6` | Popup width as a fraction of the window width (range 0–1). |
| `menu_screen_height_ratio` | `0.6` | Popup height as a fraction of the window height (range 0–1). |
| `menu_screen_center_x` | `0.5` | Horizontal position of the popup's center as a fraction of the window width, clamped so the popup stays inside (range 0–1). |
| `menu_screen_center_y` | `0.5` | Vertical position of the popup's center as a fraction of the window height, clamped so the popup stays inside (range 0–1). |
| `multiline_menus` | `1` | Wrap long popup entries over several lines instead of truncating them. |
| `fuzzy_searching` | `0` | Filter popup lists by fuzzy matching instead of substring matching. |
| `item_list_prefix` | `>` | Prefix shown before each entry of the bookmark, portal and mark lists. |
| `show_most_recent_commands_first` | `1` | List recently used commands first in the command palette. |
| `show_command_hints` | `0` | After the first key of a multi-key binding, show the bindings that can follow. |
| `show_doc_path` | `0` | Show full paths instead of file names in the recent documents list (`open_prev_doc`). |
| `documentation_font_size` | `16` | Font size in pixels of the command documentation window (range 1–100). |
| `disable_cursor_blinking` | `0` | Stop the text cursor from blinking in input fields. |
| `flat_toc` | `0` | Show the table of contents as a flat list instead of a tree (faster for very large ones). |
| `collapsed_toc` | `0` | Open the table of contents collapsed, with only the current entry expanded. |
| `page_labels_in_table_of_contents` | `0` | Show page labels instead of page numbers in the table of contents. |
| `toc_jump_align_top` | `0` | Put the target of a table of contents jump at the top of the window instead of the middle. |
| `create_table_of_contents_if_not_exists` | `1` | Build a table of contents from the text when the document has none. |
| `max_created_toc_size` | `5000` | Maximum number of entries in a generated table of contents (range 1–100000). |

#### Status bar

| Setting | Default | Meaning |
|---|---|---|
| `status_bar_format` | `[ %{current_page} / %{num_pages} ]%{chapter_name}%{search_results}%{search_progress}%{link_status}%{waiting_for_symbol}%{indexing}%{preview_index}%{synctex}%{drag}%{presentation}%{visual_scroll}%{locked_scroll}%{highlight}%{freehand_drawing}%{closest_bookmark}%{close_portal}%{rect_select}%{custom_message}%{download}` | Template of the left part of the status bar; each `%{...}` placeholder is replaced by its value. |
| `right_status_bar_format` |  | Template of the right-aligned part of the status bar, with the same placeholders. |
| `status_bar_color` | `0 0 0` | Background of the status bar, the text input bar and the command hints. |
| `status_bar_text_color` | `1 1 1` | Text color of the status bar, the text input bar and the command hints. |
| `status_bar_font_size` | `-1` | Font size in pixels of the status bar and the text input bar; `-1` uses the system size (range 1–100). |
| `status_font` |  | Font of the status bar and the text input bar; empty uses the bundled JetBrains Mono. |
| `show_statusbar_only_when_hovered` | `0` | Hide the status bar unless the mouse pointer is over it. |
| `show_closest_bookmark_in_statusbar` | `0` | Fill `%{closest_bookmark}` with the text of the nearest bookmark. |
| `show_close_portal_in_statusbar` | `0` | Fill `%{close_portal}` with `[ PORTAL ]` when a portal is nearby. |

#### Tabs and title bar (macOS)

| Setting | Default | Meaning |
|---|---|---|
| `tab_bar_font_size` | `14` | Font size in pixels of the tab bar; the bar's height follows it (range 1–100). |
| `macos_tabs_in_titlebar` | `1` | macOS: put the tab bar in the title bar next to the window buttons and always show it; with 0 it sits below the title bar and appears with two or more tabs. |
| `macos_hide_titlebar` | `0` | macOS: hide the window title bar; this also turns off `macos_tabs_in_titlebar`. |
| `macos_titlebar_color` | `-1 -1 -1` | macOS: title bar color; the default `-1 -1 -1` leaves the system color. |

#### Navigation and scrolling

| Setting | Default | Meaning |
|---|---|---|
| `vertical_move_amount` | `1.0` | Distance in inches of `move_down`/`move_up` and of each mouse wheel step (range 0.1–10). |
| `horizontal_move_amount` | `1.0` | Distance in inches of `move_left`/`move_right` and of each horizontal wheel step (range 0.1–10). |
| `move_screen_ratio` | `0.5` | Fraction of the window height that `screen_down`/`screen_up` scroll (range 0–1). |
| `move_screen_percentage` | `0.5` | Same setting as `move_screen_ratio`. |
| `touchpad_sensitivity` | `1.0` | Multiplier for wheel and touchpad scrolling distance (range 0–10). |
| `free_touchpad_movement` | `0` | Let touchpad scrolling move diagonally; otherwise each gesture follows its dominant axis. |
| `inverted_horizontal_scrolling` | `0` | Reverse the direction of horizontal wheel and touchpad scrolling. |
| `ignore_scroll_events` | `0` | Ignore all mouse wheel and touchpad scrolling. |
| `scroll_past_document_ends` | `1` | Allow scrolling until the first or last page edge reaches the middle of the window; 0 stops at the window edge. |
| `horizontal_scroll_past_page_ends` | `0` | Allow horizontal scrolling up to one page width beyond the sides of the page. |
| `allow_horizontal_drag_when_document_is_small` | `0` | Allow dragging the document sideways when the page is narrower than the window. |
| `smooth_scroll_speed` | `3.0` | Speed added per scroll step in smooth scroll mode (`toggle_smooth_scroll_mode`) (range 0–20). |
| `smooth_scroll_drag` | `3000` | Deceleration of the movement in smooth scroll mode (range 10–10000). |
| `smooth_move_max_velocity` | `1500` | Speed of the `move_*_smooth` commands and of edge scrolling while selecting text (range 0–100000). |
| `gg_uses_labels` | `0` | Make `<n>gg` go to the page labeled n instead of page number n. |

#### Ruler (visual mark)

| Setting | Default | Meaning |
|---|---|---|
| `ruler_display_mode` | `underline` | How the ruler marks the current line: `underline`, `box`, or `slit` (masks the rest of the page). |
| `ruler_mode` | `1` | When the ruler masks the page (`slit`), also mask above and beside the line, not only below it. |
| `ruler_color` | `0.5 0.5 0.5` | Color of the ruler's underline or box. |
| `ruler_marker_color` | `1 0 0` | Color of the marker showing where a line wider than the window continues after horizontal scrolling. |
| `ruler_pixel_width` | `2` | Thickness in pixels of the ruler's underline, box and marker (range 1–100). |
| `ruler_padding` | `1.0` | Extra vertical padding around the ruler's line (range 0–10). |
| `ruler_x_padding` | `5.0` | Extra horizontal padding around the ruler's line (range 0–10). |
| `visual_mark_color` | `0.0 0.0 0.0 0.1` | RGBA color of the mask the ruler draws in `slit` mode. |
| `vertical_line_color` | `0 0 0 0.1` | Same setting as `visual_mark_color`. |
| `visual_mark_next_page_fraction` | `0.75` | Where the ruler's line ends up when the view scrolls to follow it, from 0 (window center) to 1 (window top) (range -2–2). |
| `visual_mark_next_page_threshold` | `0.25` | How close the ruler may get to the window edge, as a fraction of half the window height, before the view scrolls to follow it (range -2–2). |
| `autocenter_visual_scroll` | `0` | Center the view on the ruler after every ruler move. |
| `force_custom_line_algorithm` | `0` | Find ruler lines with yaview's image-based algorithm even when the document has text lines. |

#### Zoom, fit and page layout

| Setting | Default | Meaning |
|---|---|---|
| `zoom_inc_factor` | `1.2` | Zoom factor of `zoom_in`/`zoom_out` (range 1–2). |
| `scroll_zoom_inc_factor` | `1.2` | Zoom factor per Ctrl+wheel (or Cmd+wheel) step (range 1–2). |
| `wheel_zoom_on_cursor` | `0` | Zoom with the wheel around the mouse pointer instead of the window's center. |
| `fit_to_page_width_ratio` | `0.75` | Fraction of the window width the page fills after `fit_to_page_width_ratio` (range 0.25–1). |
| `page_space_x` | `10` | Horizontal gap between the two pages in two-page mode (range 0–100). |
| `page_space_y` | `10` | Vertical gap between pages in two-page mode or with `real_page_separation` or `uniform_page_widths` (range 0–100). |
| `real_page_separation` | `0` | Lay the pages out with a `page_space_y` gap between them. |
| `uniform_page_widths` | `0` | Scale every page to the document's median page width. |
| `recto_verso_adjustment` | `0` | Shift the page pairing in two-page mode by one page. |
| `page_separator_width` | `0` | Thickness of a line drawn between pages; 0 draws none (range 0–10). |
| `page_separator_color` | `0.9 0.9 0.9` | Color of the line drawn between pages. |

#### Mouse and clicks

| Setting | Default | Meaning |
|---|---|---|
| `shift_click_command` | `overview_under_cursor` | Command run on Shift+click. |
| `control_click_command` | `smart_jump_under_cursor` | Command run on Ctrl+click; Cmd+click runs it too. |
| `alt_click_command` |  | Command run on Alt (Option)+click. |
| `right_click_command` |  | Command run on right-click, replacing the default (placing the ruler on the clicked line). |
| `shift_right_click_command` |  | Command run on Shift+right-click. |
| `control_right_click_command` |  | Command run on Ctrl+right-click; Cmd+right-click runs it too. |
| `alt_right_click_command` |  | Command run on Alt (Option)+right-click. |
| `middle_click_command` |  | Command run on middle-click; empty jumps to the reference under the pointer or searches the paper name under it. |
| `hold_middle_click_command` | `download_paper_under_cursor` | Command run when the middle button is held down for more than 200 ms. |
| `highlight_middle_click` | `0` | Middle-clicking while text is selected adds a highlight instead of running the middle-click action. |
| `right_click_scroll_up_command` |  | Command run on wheel up while the right button is held. |
| `right_click_scroll_down_command` |  | Command run on wheel down while the right button is held. |
| `left_click_scroll_up_command` |  | Command run on wheel up while the left button is held. |
| `left_click_scroll_down_command` |  | Command run on wheel down while the left button is held. |
| `right_click_context_menu` | `0` | Show a context menu on right-click over selected text, a highlight, a bookmark or the overview window. |
| `context_menu_items_for_selected_text` | `copy\|add_highlight(a)\|add_highlight(b)\|add_highlight(c)` | Context menu entries for selected text, as commands separated by `\|`. |
| `context_menu_items_for_highlights` | `delete_highlight\|edit_selected_annotation\|add_highlight(a)\|add_highlight(b)\|add_highlight(c)` | Context menu entries for a highlight. |
| `context_menu_items_for_bookmarks` | `delete_visible_annotation\|edit_selected_annotation\|move_selected_bookmark` | Context menu entries for a bookmark. |
| `context_menu_items_for_overview` |  | Context menu entries for the overview window; empty shows no menu there. |

#### Text selection and copying

| Setting | Default | Meaning |
|---|---|---|
| `text_highlight_color` | `1.0 1.0 0.0` | Color of selected text. |
| `invert_selected_text` | `0` | Show selected text with inverted colors instead of a colored box. |
| `single_click_selects_words` | `0` | Make click-and-drag select whole words and double-click-and-drag select characters; the default is the reverse. |
| `text_selection_minimum_distance` | `5` | Distance the pointer must move with the button down before a text selection starts (range 0–100). |
| `add_newlines_when_copying_text` | `0` | Keep line breaks in text copied with `copy`. |
| `always_copy_selected_text` | `0` | Copy mouse-selected text to the clipboard; otherwise it goes to the X11 primary selection, which only Linux has. |
| `fill_textbar_with_selected_text` | `1` | Prefill text prompts, such as search, with the selected text. |

#### Keyboard selection and tags

| Setting | Default | Meaning |
|---|---|---|
| `keyboard_select_background_color` | `0.9 0.75 0 1` | RGBA background of the tags shown on words (`keyboard_select`) and links. |
| `keyboard_select_text_color` | `0 0 0.5 1` | RGBA text color of those tags. |
| `keyboard_selected_tag_background_color` | `0 0 0 1` | RGBA background of tags that have been typed in full. |
| `keyboard_selected_tag_text_color` | `1 1 1 1` | RGBA text color of tags that have been typed in full. |
| `keyboard_select_font_size` | `20` | Font size in pixels of the tags (range 1–100). |
| `tag_font_face` |  | Font of the tags; empty uses the bundled JetBrains Mono. |
| `numeric_tags` | `0` | Tag words with numbers instead of letters. |
| `alphabetic_link_tags` | `0` | Tag links with letters instead of numbers. |
| `hide_overlapping_link_labels` | `1` | Hide the tag of a link that sits on top of another link. |
| `keyboard_select_inclusive` | `0` | Include the word under the second tag in a `keyboard_select` selection; by default the selection stops before it. |
| `keyboard_point_selection` | `0` | Pick the points and rectangles that commands ask for with keyboard tags instead of the mouse. |

#### Search

| Setting | Default | Meaning |
|---|---|---|
| `search_highlight_color` | `0.0 1.0 0.0` | Color of the current search match. |
| `unselected_search_highlight_color` | `0 0.5 0.5` | Color of the other visible search matches. |
| `should_highlight_unselected_search` | `1` | Highlight every visible search match, not only the current one. |
| `super_fast_search` | `1` | Search with yaview's own text index instead of MuPDF's search; `incremental_search` needs it. |
| `incremental_search` | `0` | Search while typing, once the `super_fast_search` index is ready. |
| `case_sensitive_search` | `0` | Make searches case-sensitive. |
| `smartcase_search` | `0` | Search case-sensitively only when the query has an uppercase letter; overrides `case_sensitive_search`. |
| `search_url_<letter>` | `s`, `g`: `https://www.google.com/search?q=`; `c`: `https://scholar.google.com/scholar?q=`; other letters empty | URL that `external_search` followed by the letter opens, with the selected text (or the detected paper name) appended or put in place of `%{search_pattern}`. |
| `middle_click_search_engine` | `s` | Letter of the `search_url_<letter>` used when middle-clicking a paper name. |
| `shift_middle_click_search_engine` | `l` | Letter of the `search_url_<letter>` used when Shift+middle-clicking a paper name; `search_url_l` is empty by default. |

#### Highlights, bookmarks and drawings

| Setting | Default | Meaning |
|---|---|---|
| `highlight_color_<letter>` | 26 colors from `0.94 0.64 1.00` (`a`) to `1.00 0.31 0.02` (`z`), listed in `prefs.config` | Color of highlight type `<letter>` (the uppercase type draws an underline in the same color) and of freehand drawings of that type. |
| `strike_line_width` | `1.0` | Line thickness of underline and strikethrough highlights (range 0–10). |
| `exact_highlight_select` | `0` | Highlight exactly the selected characters instead of expanding to whole words. |
| `sort_highlights_by_location` | `1` | Sort the highlight list by position in the document instead of creation time. |
| `sort_bookmarks_by_location` | `1` | Sort the bookmark and portal lists by position in the document instead of creation time. |
| `freetext_bookmark_color` | `0 0 0` | Text color of new freetext bookmarks. |
| `freetext_bookmark_font_size` | `8` | Default font size of freetext bookmarks; `increase_freetext_font_size`/`decrease_freetext_font_size` change it (range 0–100). |
| `render_freetext_borders` | `0` | Draw a border around freetext bookmarks. |
| `lighten_colors_when_embedding_annotations` | `1` | Lighten highlight colors when `embed_annotations` writes them into a PDF. |
| `simplify_freehand_drawings` | `1` | Drop redundant points from a freehand stroke when it ends. |
| `annotations_directory` |  | Folder for each document's side files (`<file>.yaview.drawings`, `.scratchpad`, `.annotations`); empty puts them next to the document. |
| `shared_database_path` |  | Path of the `shared.db` database; a synced folder shares it between computers; empty uses the data folder. |

#### Portals, overview and links

| Setting | Default | Meaning |
|---|---|---|
| `hover_overview` | `0` | Show an overview of a link's target while the pointer is over the link. |
| `overview_size` | `0.8 0.4` | Width and height of the overview window as fractions of the window (saved by yaview). |
| `overview_offset` | `0 0` | Position of the overview window's center, where `0 0` is the window's center and ±1 its edges (saved by yaview). |
| `allow_main_view_scroll_while_in_overview` | `0` | Scrolling outside the open overview moves the document instead of stepping through the overview's targets. |
| `scrollview_sensitivity` | `1.0` | Multiplier for mouse wheel scrolling inside the overview window (range 0–10). |
| `link_highlight_color` | `0.0 0.0 1.0` | Color of highlighted links (`toggle_highlight`, `highlight_links`). |
| `highlight_links` | `0` | Always highlight links. |
| `align_link_dest_to_top` | `0` | After following a link, put its target at the top of the window instead of the middle. |
| `highlight_link_destination` | `0` | Briefly highlight the target of a followed link. |
| `link_destination_highlight_color` | `1.0 0.0 1.0` | Same setting as `synctex_highlight_color`. |
| `link_destination_highlight_timeout` | `1.0` | Same setting as `synctex_highlight_timeout`. |

#### Paper download

| Setting | Default | Meaning |
|---|---|---|
| `paper_search_url` | `https://search.fatcat.wiki/fatcat_release/_search?q=%{query}` | Search API used to download a paper by name; `%{query}` is replaced by the name. |
| `paper_download_url_path` | `hits.hits[]._source.best_pdf_url` | JSON path of the PDF URLs in the search response. |
| `paper_download_title_path` | `hits.hits[]._source.title` | JSON path of the titles in the search response. |
| `paper_download_contrib_path` | `hits.hits[]._source.contrib_names` | JSON path of the author names in the search response. |
| `paper_download_should_create_portal` | `1` | Link a downloaded paper to its reference with a portal; with 0 it opens in a new window. |
| `paper_download_should_detect_paper_name` | `1` | Extract the quoted title from a reference before searching for it. |
| `automatically_download_matching_paper_name` | `1` | Download a result directly when its title matches the query; otherwise show the result list. |
| `auto_rename_downloaded_papers` | `0` | Name downloaded files after the paper's title. |
| `papers_folder_path` |  | Folder for downloaded papers (default: `downloads` in the data folder); a paper added to it while a portal is pending becomes the portal's target. |

#### Rendering

| Setting | Default | Meaning |
|---|---|---|
| `render_pdf_annotations` | `1` | Render the annotations stored in the PDF itself. |
| `linear_filter` | `0` | Draw pages with linear instead of nearest-neighbor texture filtering. |
| `gamma` | `1` | Gamma correction applied to rendered pages; 1 changes nothing (range 0–1). |
| `background_pixel_fix` | `0` | Leave out each page's last pixel row and column to avoid a light strip on dark backgrounds. |
| `should_draw_unrendered_pages` | `0` | Draw a blank page-colored rectangle for pages that are still rendering; by default nothing is drawn. |
| `num_cached_pages` | `5` | Number of rendered pages kept in memory, doubled in two-page mode when below 6 (range 0–100). |
| `prerendered_page_count` | `0` | Number of pages after the last visible one to render in advance (range 0–10). |
| `sliced_rendering` | `0` | Render pages in five horizontal strips instead of whole; pages taller than 2000 units are always split. |

#### Presentation mode

| Setting | Default | Meaning |
|---|---|---|
| `prerender_next_page_presentation` | `1` | Render upcoming slides in advance in presentation mode to avoid flicker. |
| `num_prerendered_next_slides` | `1` | Number of following slides rendered in advance (range 0–5). |
| `num_prerendered_prev_slides` | `0` | Number of preceding slides rendered in advance (range 0–5). |
| `ignore_whitespace_in_presentation_mode` | `0` | Fit slides to the window without their blank margins. |

#### EPUB and other reflowable documents

| Setting | Default | Meaning |
|---|---|---|
| `epub_width` | `400` | Page width used to lay out reflowable documents such as EPUB; `fit_epub_to_window` sets it to the window (range 0–1000). |
| `epub_height` | `700` | Page height used to lay out reflowable documents (range 0–1000). |
| `epub_font_size` | `14` | Font size used to lay out reflowable documents (range 0–100). |
| `epub_css` |  | Extra CSS applied to reflowable documents. |

#### Startup, windows and files

| Setting | Default | Meaning |
|---|---|---|
| `open_last_file_on_startup` | `1` | Reopen the last session's tabs when yaview starts without a file. |
| `restore_all_windows_on_startup` | `0` | Save every window with its tabs and geometry, and restore them all at startup. |
| `should_launch_new_window` | `0` | Open files passed from outside (command line, Finder) in a new window instead of the current one. |
| `should_launch_new_instance` | `0` | Deprecated: start a separate yaview process for every opened file instead of reusing the running one. |
| `startup_commands` |  | Commands, separated by `;`, run at startup and in every new window. |
| `periodic_commands` |  | Commands, separated by `;`, run on every tick of the window's update timer (about every 200 ms). |
| `resize_command` |  | Command run whenever the window is resized. |
| `fail_safe_auto_save_session` | `0` | Save the session every time a document is opened, not only when quitting. |
| `persistance_period` | `-1` | Also save the session every this many seconds; `-1` turns this off (range -1–100000). |
| `start_with_helper_window` | `0` | Open the helper window at startup, if `helper_window_size` is set. |
| `main_window_size` | `-1 -1` | Main window size while the helper window is open; `-1 -1` picks one from the screen (saved by yaview). |
| `main_window_move` | `-1 -1` | Main window position while the helper window is open (saved by yaview). |
| `helper_window_size` | `-1 -1` | Helper window size (saved by yaview). |
| `helper_window_move` | `-1 -1` | Helper window position (saved by yaview). |
| `single_main_window_size` | `-1 -1` | Main window size without the helper window; `-1 -1` fills the primary screen (saved by yaview). |
| `single_main_window_move` | `-1 -1` | Main window position without the helper window (saved by yaview). |
| `default_open_file_path` |  | Start folder of the file browser (`open_document_embedded`, Ctrl+O) and the native open dialog; the file browser uses the home folder when empty. |
| `scan_path` |  | Folder whose new files `scan_new_files` adds to the document database. |
| `reload_interval_miliseconds` | `200` | How long a changed document must stay unchanged on disk before it is reloaded (range 0–10000). |

#### Synctex and external tools

| Setting | Default | Meaning |
|---|---|---|
| `inverse_search_command` |  | Command run for inverse search from the PDF to the LaTeX source; `%1` is the source file, `%2` the line, `%3` the column. |
| `synctex_highlight_color` | `1.0 0.0 1.0` | Color of synctex forward-search highlights and of followed-link highlights. |
| `synctex_highlight_timeout` | `1.0` | Seconds before synctex and link-target highlights disappear; `-1` keeps them (range -1–100). |
| `use_ruler_to_highlight_synctex_line` | `1` | Mark the forward-search target line with the ruler instead of a colored highlight. |
| `dont_center_if_synctex_rect_is_visible` | `0` | Don't scroll on forward search when the target line is already visible; needs `use_ruler_to_highlight_synctex_line 0`. |
| `vimtex_wsl_fix` | `0` | Windows only: pass synctex paths to `inverse_search_command` unchanged, for VimTeX under WSL. |

#### Other

| Setting | Default | Meaning |
|---|---|---|
| `use_legacy_keybinds` | `0` | Parse key bindings with the old method, for backwards compatibility. |
| `should_warn_about_user_key_override` | `1` | Print a warning when a key binding overrides one from another file; with 0 only redefinitions within one file warn. |
| `tts_rate` | `0` | Text-to-speech reading speed, from -1 (slowest) to 1 (fastest) (range -1–1). |

#### Touch mode

These only matter with `touch_mode 1`. Tap areas are written `enabled left right top bottom` in window coordinates from -1 to 1, where `-1` is the left or top edge.

| Setting | Default | Meaning |
|---|---|---|
| `touch_mode` | `0` | Use the touch interface: tap areas, touch menus and selection handles. |
| `ruler_auto_move_sensitivity` | `40` | Pixels of dragging inside a ruler tap area per line the ruler moves (range 0–200). |
| `portrait_back_ui_rect` | `1 -1 -0.7 -1 -0.7` | Back tap area in portrait orientation. |
| `landscape_back_ui_rect` | `1 -1 -0.7 -1 -0.7` | Back tap area in landscape orientation. |
| `portrait_forward_ui_rect` | `1 0.7 1 -1 -0.7` | Forward tap area in portrait orientation. |
| `landscape_forward_ui_rect` | `1 0.7 1 -1 -0.7` | Forward tap area in landscape orientation. |
| `portrait_visual_mark_next` | `1 0.7 1 0.7 1` | Portrait tap area that moves the ruler to the next line. |
| `portrait_visual_mark_prev` | `1 -1 -0.7 0.7 1` | Portrait tap area that moves the ruler to the previous line. |
| `landscape_visual_mark_next` | `1 0.7 1 0.7 1` | Landscape tap area that moves the ruler to the next line. |
| `landscape_visual_mark_prev` | `1 -1 -0.7 0.7 1` | Landscape tap area that moves the ruler to the previous line. |
| `portrait_middle_left_rect` | `1 -1 -0.7 -0.3 0.3` | Middle-left tap area in portrait orientation. |
| `portrait_middle_right_rect` | `1 0.7 1 -0.3 0.3` | Middle-right tap area in portrait orientation. |
| `landscape_middle_left_rect` | `1 -1 -0.7 -0.3 0.3` | Middle-left tap area in landscape orientation. |
| `landscape_middle_right_rect` | `1 0.7 1 -0.3 0.3` | Middle-right tap area in landscape orientation. |
| `back_rect_tap_command` | `prev_state` | Command run on a tap in the back area. |
| `back_rect_hold_command` | `goto_mark` | Command run on a long press in the back area. |
| `forward_rect_tap_command` | `next_state` | Command run on a tap in the forward area. |
| `forward_rect_hold_command` | `set_mark` | Command run on a long press in the forward area. |
| `top_center_tap_command` | `show_touch_main_menu` | Command run on a tap in the fixed top-center area. |
| `top_center_hold_command` | `toggle_dark_mode` | Command run on a long press in the fixed top-center area. |
| `middle_left_rect_tap_command` |  | Command run on a tap in the middle-left area. |
| `middle_left_rect_hold_command` |  | Command run on a long press in the middle-left area. |
| `middle_right_rect_tap_command` |  | Command run on a tap in the middle-right area. |
| `middle_right_rect_hold_command` |  | Command run on a long press in the middle-right area. |
| `visual_mark_next_tap_command` |  | Command run on a tap in the next-line ruler area while the ruler is off. |
| `visual_mark_next_hold_command` |  | Command run on a long press in the next-line ruler area. |
| `visual_mark_prev_tap_command` |  | Command run on a tap in the previous-line ruler area while the ruler is off. |
| `visual_mark_prev_hold_command` |  | Command run on a long press in the previous-line ruler area. |

#### No effect

Accepted, but currently without effect: `check_for_updates_on_startup`, `should_use_multiple_monitors`, `rerender_overview`, `enable_experimental_features`, `show_document_name_in_statusbar`, `context_menu_items`, `context_menu_items_for_links`, `google_scholar_address`, `libgen_address`, `highlight_delete_threshold`, `execute_command_<letter>`, `text_summary_url`, `text_summary_should_refine`, `text_summary_should_fill`, `use_heuristic_if_text_summary_not_available`, `text_summary_context_size`.
