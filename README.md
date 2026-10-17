<img src="resources/yaview-icon.png" alt="yaview icon" width="128">

# yaview -- a PDF Viewer

[![Memory leak tests](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/alexyavo/yaview/badges/memory-tests.json)](https://github.com/alexyavo/yaview/actions/workflows/memory-tests.yml)

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
  by the engine's search URL in `prefs_user.config` (see [Configuration](#configuration)); the selected text is
  appended to the URL. For example:

  ```
  search_url_s https://kagi.com/search?q=
  ```

- <kbd>:</kbd> for commands
- <kbd>/</kbd> to search. <kbd>n</kbd> / <kbd>Shift</kbd>+<kbd>n</kbd> to go to the next/previous match.

### Marks

- <kbd>m</kbd> + letter set a mark at the current location (e.g. <kbd>ma</kbd>). Lowercase marks belong to the
  document, uppercase marks are global (going to one opens its document). Marks are kept across restarts.
- <kbd>`</kbd> + letter go to a mark (e.g. <kbd>`</kbd><kbd>a</kbd>).
- `:show_marks` list the marks of all documents.
- `:clear_document_marks` delete the marks of the current document (including the uppercase ones set in it).
- `:clear_all_marks` delete the marks of all documents.
- Right click a line to put a ruler (visual mark) under it.
- <kbd>j</kbd> / <kbd>k</kbd> move the ruler to the next/previous line (also: <kbd>↓</kbd> / <kbd>↑</kbd>).
- <kbd>`</kbd><kbd>`</kbd> go back to the ruler's last location (also: <kbd>`</kbd> then right click).
- <kbd>F7</kbd> toggle ruler scrolling, where the mouse wheel moves the ruler.

### Bookmarks

- <kbd>b</kbd> add a bookmark at the current location: type its text and press <kbd>Enter</kbd>. Selected text is
  filled in as the bookmark text.
- <kbd>gb</kbd> search the bookmarks of the current document.
- <kbd>gB</kbd> search the bookmarks of all documents.
- <kbd>db</kbd> delete the closest bookmark.

### Highlights

- <kbd>h</kbd> + lowercase letter highlight the selected text (e.g. <kbd>ha</kbd>). The letter is the highlight's type;
  each type has its own color (`highlight_color_<letter>` in `prefs_user.config`).
- <kbd>gh</kbd> search the highlights of the current document.
- <kbd>gH</kbd> search the highlights of all documents.
- <kbd>gnh</kbd> / <kbd>gNh</kbd> go to the next/previous highlight.
- <kbd>dh</kbd> delete the clicked highlight.

### Smart Jump

- Middle click a figure name (the "1" in "Figure 1") or an in-line reference (the "2" in "[1, 2]") to jump to the
  figure or the reference entry, also in documents without links.
- Right click a figure name or in-line reference to show its target in an overview instead (right click again to close
  it).
- Middle click a full citation (e.g. in the References section) to search the paper with the `s` search engine
  (`middle_click_search_engine` in `prefs_user.config` picks another `search_url_<letter>`).

### Portals

- <kbd>p</kbd> start a portal at the current location; go to the destination (in any document) and press <kbd>p</kbd>
  again. Clicking a link after the first <kbd>p</kbd> uses the link's destination.
- <kbd>F12</kbd> toggle the helper window, which shows the destination of the closest portal. In it, the mouse wheel
  and dragging adjust the destination and <kbd>Cmd</kbd>/<kbd>Ctrl</kbd> + wheel zooms.
- <kbd>Tab</kbd> go to the closest portal's destination (also: <kbd>gp</kbd>).
- <kbd>Shift</kbd>+<kbd>Tab</kbd> edit the closest portal: go to its destination, adjust it, and go back
  (<kbd>Backspace</kbd>) to save it (also: <kbd>P</kbd>).
- <kbd>dp</kbd> delete the closest portal.

### Configuration

- `keys.config` and `prefs.config` hold the defaults; don't edit them. `keys_user.config` and `prefs_user.config` are
  loaded after them and override them.
- To change a setting, copy its line from `prefs.config` to `prefs_user.config` and edit it there. Key bindings in
  `keys_user.config` are added to the defaults. For example, to also open the table of contents with <kbd>T</kbd>,
  find its line in `keys.config`:

  ```
  # Open table of contents.
  goto_toc t
  ```

  and put this in `keys_user.config`:

  ```
  goto_toc T
  ```
- Location: on macOS `~/.config/yaview/` or `~/Library/Application Support/yaview/`; in portable builds, next to the
  `yaview` executable.
- `:prefs`, `:prefs_user`, `:keys`, `:keys_user` open the files.
