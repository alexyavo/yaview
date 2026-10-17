<img src="resources/yaview-icon.png" alt="yaview icon" width="128">

# yaview -- a PDF Viewer

[![Memory leak tests](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/alexyavo/yaview/badges/memory-tests.json)](https://github.com/alexyavo/yaview/actions/workflows/memory-tests.yml)

yaview is a fork of [sioyek](https://github.com/ahrm/sioyek).

### Basics

- <kbd>o</kbd> open a file, starting in the current document's folder.
- <kbd>Ctrl</kbd>+<kbd>o</kbd> open a file, starting in `default_open_file_path` (the home folder if unset).
- <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>o</kbd> open a file with the system's file dialog (starts in
  `default_open_file_path` if set).
- <kbd>Shift</kbd>+<kbd>o</kbd> open a previously opened document.
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

  Try it out on the following text:

  > Vaswani, Ashish, et al. "**Attention is all you need**." Advances in neural information processing systems. 2017.

### Tabs

- Every opened document gets a tab in the tab bar at the top of the window (on macOS, in the title bar next to the
  window buttons). Click a tab to switch to it.
- Press <kbd>Ctrl</kbd>+<kbd>Tab</kbd> / <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Tab</kbd> to go to the next/previous tab.
- Press <kbd>Cmd</kbd>+<kbd>w</kbd> (macOS) or enter `:q` to close the current tab. You can also click a tab's ×
  button, or middle click the tab.

### Command Palette

- Press <kbd>:</kbd> to open a searchable list of all yaview commands.
- If the command is bound to a key, the keybinding is shown on the right (multiple keybinds are separated by `|` symbol).

### Search

- Press <kbd>Ctrl</kbd>+<kbd>f</kbd> or <kbd>/</kbd> to search.
- Press <kbd>n</kbd> / <kbd>Shift</kbd>+<kbd>n</kbd> to go to the next/previous match.
- In order to quickly jump in search results, you can use `[NUM]`<kbd>n</kbd> where `[NUM]` is a number. For example in
  order to jump to the 10th next occurance of current search term, enter <kbd>10n</kbd>. In fact most commands in yaview
  can be prefixed with a number `N` which is equivalent to repeating them `N` times.

### Marks

- Press <kbd>m</kbd> to set a mark in current location. After you press <kbd>m</kbd>, yaview waits for you to press
  another symbol which is the name of the mark. For example to create a mark named u, enter <kbd>mu</kbd>.
- In order to go to a mark, press <kbd>`</kbd> (backtick) followed by the name of the mark. For example in order to go to
  the mark in previous example, we enter <kbd>`</kbd> <kbd>u</kbd>. Note that the backtick character <kbd>`</kbd> is
  different from the single quote character <kbd>'</kbd>.
- Note that marks are persistent (they are saved even after yaview is closed). Lower case marks are local to each file,
  while upper case marks are global.
- You can right click on any line to create a visual mark which highlights below the line. This highlight has multiple
  functions. First, it can reduce disorientation when scrolling by first right clicking the line we are reading and then
  scrolling. Second, it can also act as a mark, so we can jump back to the location where we last put the highlight by
  pressing <kbd>`</kbd> and then right clicking. You can try it out on the "**Right click on me!**" line in the tutorial.
- You can move this visual mark to the next/previous line by pressing <kbd>j</kbd> and <kbd>k</kbd>. You can also press
  <kbd>F7</kbd> to enable visual scroll mode. In this mode, scrolling the mouse wheel moves this visual mark.

### Bookmarks

- Press <kbd>b</kbd> to create a bookmark in the current location.
- Press <kbd>gb</kbd> to search the bookmarks in the current file and <kbd>gB</kbd> (that is g followed by capital B) to
  search all the bookmarks.
- In order to delete a bookmark, first goto the bookmark and then enter <kbd>db</kbd> (delete the closest bookmark).
- If you select a piece of text and then press <kbd>b</kbd> then the selected text will automatically be used as the
  bookmark text. You can try it on the "**Bookmark me!**" text in the tutorial.

### Highlights

- Select a piece of text and then press <kbd>h</kbd> followed by a lower case letter to highlight the selected text (the
  letter can be though of as the "type" of the highlight). For example you can press <kbd>hh</kbd> to create a highlight
  of type "h". Different types of highlight are colored differently, and the color is configurable in the
  `prefs.config` file.
- Press <kbd>gh</kbd> to search the highlights in the current file and <kbd>gH</kbd> (that is g followed by capital H)
  to search all the highlights.
- In order to delete a highlight, you can click on a highlight and then press <kbd>dh</kbd>.

### Smart Jump

- You can middle click on figure names (for example on "1" in "Figure 1") to jump to the location where that figure is
  located. This works even if the document is not linked. You can also middle click on in-line references (for example
  on "2" in "[1, 2]") to go to the corresponding entry in the "References" section.
- You can also right click on figure names or in-line references to open a quick overview of their location instead of
  jumping to them.
- Try it out in the tutorial by middle / right clicking on the figure name / in-line reference in the following text:

  > You can see a beautiful colorization of mandlebrot set in Figure 1. For more information about the mandlebrot set,
  > see [1].
- You can also middle click on the full-citation of a paper to search it with the `s` search engine (Google; set
  `middle_click_search_engine` in `prefs_user.config` to use another one, for example `c` for Google Scholar). You can
  use this feature, for example, while in the "References" section of a document.

### Portals

- Press <kbd>F12</kbd> to open the helper window. This window automatically displays the closest portal to the current
  location.
- In order to create a portal, press <kbd>p</kbd> to enter the current location as the source of the portal. Now you
  navigate to the portal destination and press <kbd>p</kbd> again to mark that location as the destination of the portal
  (note that source and destination don't have to be in the same document, for example you can create a portal from a
  reference to a paper to the actual paper).
- You can also create a portal by pressing <kbd>p</kbd> and then clicking (or middle clicking using smart jumps) on a
  link. This automatically creates a link between the current location and the destination of the link.
- In order to delete a portal, press <kbd>dp</kbd>. This deletes the closest portal to the current location (the portal
  that is being displayed in the helper window).
- Sometimes you need to slightly edit a portal (especially when creating a portal using links, because sometimes for
  example a link to a figure doesn't center the figure properly in the screen). In order to do this, press
  <kbd>Shift</kbd>+<kbd>p</kbd> (<kbd>P</kbd>) while the portal is active. This takes you to the portal's destination.
  Now you can adjust the destination and once you are done press the back button (by default it is
  <kbd>Backspace</kbd>) to go back to where you were.
- You can also use the mouse wheel to adjust the portal in helper window. You can also pan it using mouse drag or zoom
  using mouse wheel while holding control.

### Configuration

- There are four configuration files: two of them are system-wide and are not meant to be edited by the user
  (`keys.config` and `prefs.config`), while the other two are user-editable (`keys_user.config` and
  `prefs_user.config`).
- The former (i.e., the system-wide configuration files) are always loaded by yaview before the latter. This implies
  that configuration options set in the user-editable configuration files always overwrite the ones in the system-wide
  files.
- In order to edit any configuration, copy the configuration line you want to edit to the user-editable config file and
  change it there. For example, suppose we want to change the key that opens the table of contents. We search for
  "table of contents" in `keys.config` and find the following:

  ```
  # Open table of contents.
  goto_toc t
  ```

  We copy this line to `keys_user.config` and change the key. For example, suppose we want to use capital <kbd>T</kbd>
  to open the table of contents. We can put the following line in `keys_user.config`:

  ```
  goto_toc T
  ```
- In portable builds, the config files are located in the same place where `yaview` executable file is located. In
  non-portable builds, their location is OS-dependent (on macOS, `~/.config/yaview/` and
  `~/Library/Application Support/yaview/`). You can also open them by pressing <kbd>:</kbd> to open the command window
  and then entering one of the following:
  - `prefs`
  - `prefs_user`
  - `keys`
  - `keys_user`

### Synctex

- Press <kbd>F4</kbd> to enable "synctex mode". While in synctex mode, right clicking on a piece of texts opens the
  corresponding latex file location.
- You can configure the synctex inverse search command using `inverse_search_command` config in `prefs.config`. Here is
  an example for VsCode:

  ```
  inverse_search_command 		"C:\path\to\vscode\Code.exe" -r -g %1:%2
  ```

  `%1` stands for the name of the file and `%2` stands for the line number in the file.
- Here is the forward search configuration for Latex Workshop for VsCode:

  ```json
  "latex-workshop.view.pdf.external.synctex.command": "C:\\path\\to\\yaview.exe",
  "latex-workshop.view.pdf.external.synctex.args": [
  	"--inverse-search",
  	"\"C:\\path\\to\\vscode\\Code.exe\" -r -g %1:%2",
  	"--reuse-window",
  	"--forward-search-file",
  	"%TEX%",
  	"--forward-search-line",
  	"%LINE%",
  	"%PDF%"
  ]
  ```

  And here is example configuration for VimTeX, which has no yaview viewer of its own; its sioyek viewer works with
  yaview's executable:

  ```vim
  let g:vimtex_view_method = 'sioyek'
  let g:vimtex_view_sioyek_exe = 'yaview'
  ```

### Extensions

You can extend yaview by writing your own extensions. See
[the documentation](https://sioyek-documentation.readthedocs.io/en/latest/scripting.html) on how to do it. sioyek also
has some extensions [available here](https://github.com/ahrm/sioyek-python-extensions). For example there is an extension
that downloads the paper from google scholar just by control+clicking on its name.

## Building

See [docs/build.md](docs/build.md).
