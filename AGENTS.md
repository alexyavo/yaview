# Local Sioyek development handoff

Last verified: 2026-10-12. The user requested a source checkout, build, and installation, and intends to customize Sioyek in future Codex CLI and Claude Code sessions.

## Checkout and current state

- `CLAUDE.md` is a symlink to this `AGENTS.md`, so Codex CLI and Claude Code read the same instructions. Edit `AGENTS.md`; never replace the symlink with a copy.
- Remotes: `origin` is the user's public fork `git@github.com:alexyavo/sioyek.git` (SSH, authenticated as `alexyavo`). `upstream` is `https://github.com/ahrm/sioyek.git`.
- Working branch: `main`, tracking `origin/main`. It is the fork's default branch. It started from upstream `development` at `78b94e2115f60eb124df266d13eb2838da42c5b5`. Upstream's own `main` is a stale release branch and is intentionally not used; the fork's `main` was force-replaced with `development` on 2026-10-14.
- Local `development` tracks `upstream/development`. To pull upstream changes: `git fetch upstream && git merge upstream/development` on `main`.
- Git identity: set repo-locally (`.git/config`) to `alexyavo <alxndr.yav@gmail.com>` on 2026-10-14. No global identity exists, so a fresh clone falls back to `alex <yavo@CASE.local>` until the same repo-local identity is set.
- Cloned recursively, including MuPDF, zlib, and nested submodules. Check with `git submodule status --recursive`; initialize missing submodules with `git submodule update --init --recursive`.
- Local tooling is under `.local/`, excluded through `.git/info/exclude`. Build artifacts under `build/` are already ignored by the repository. Do not add SDKs, virtualenvs, logs, or generated binaries to source control.
- Installed application: `/Applications/sioyek.app`. Build bundle: `build/local-macos/sioyek.app`.

## Customizations on `main`

- Vim-style `h`/`l` horizontal scrolling in `pdf_viewer/keys.config`, using mode-prefixed bindings (first matching mode wins; `t` = text selected, `r` = visual mark/ruler mode). `h` still adds a highlight when text is selected. `l` still runs `overview_definition` in ruler mode.
- `ss` (`external_search` + `s`) searches selected text on Kagi in the default browser. Google Scholar moved to `sc`. Both `pdf_viewer/prefs.config` and the hardcoded defaults in `pdf_viewer/main.cpp` were changed.
- `o` opens the embedded in-app file browser rooted at the current document's folder (`open_document_embedded_from_current_path`). `Ctrl+o` opens it at `default_open_file_path`, and `Ctrl+Shift+o` opens the native macOS dialog. `FileSelector` in `pdf_viewer/ui.h` now starts in the home folder when given no path, instead of the process working directory (`/` for Finder launches).
- All visible search matches are highlighted, not just the current one (`should_highlight_unselected_search`, enabled in `prefs.config` and `config.cpp`).
- `Ctrl+D` / `Ctrl+U` run `screen_down` / `screen_up`, like PageDown/PageUp. On macOS `<C-...>` in keys.config is the physical Ctrl key and `<D-...>` is Cmd. The user asked for Ctrl, not Cmd.
- Tab bar: `MainWidget::tab_bar` (a `QTabBar`) lists `document_manager->get_tabs()`. Every opened document already joins that list, so each new PDF gets a tab. The bar floats over the top edge of the view, like the status bar at the bottom, because mouse handling uses main-window coordinates with no offset correction. Putting it in the layout would shift every click. `update_all_tab_bars()` must run after any `add_tab`/`remove_tab`.
- Tab bar stacking: document view, then tab bar, then popups, then status bar. `set_current_widget` stacks popups directly under `status_label`, so the tab bar is kept under `status_label` and never raised. Raising it drew it over the command palette and table of contents. Restored tabs from a previous session refresh the bar in `open_tabs`.
- Ctrl+N / Ctrl+P move one line down / up in every list popup, including the table of contents (`t`), command palette and file browser. `BaseSelectorWidget::eventFilter` handles them on both the search box and the list. Focus moves to the list after the first move, and the list would otherwise pass Ctrl+N/P on to the main window as unknown keys.
- `close_tab` command, aliased as `:q`, closes the current tab and shows its neighbour. Closing the last tab leaves an empty window. Upstream had `:q` as an alias for `close_window`, which quit the app when only one window was open. The command palette now ranks an exact name match first.
- Ctrl+Tab / Ctrl+Shift+Tab run `goto_next_tab` / `goto_prev_tab` (`<C-<tab>>`, `<C-S-<tab>>`). The user first asked for Shift+Tab, then changed it. Shift+Tab is back on upstream's `edit_portal`. On macOS, AppKit swallows the Ctrl+Tab key press for its own focus cycling, so neither `keyPressEvent` nor an accepted ShortcutOverride sees it. The `MainWidget` constructor therefore registers macOS-only QShortcuts (`Qt::META | Qt::Key_Tab` and `Qt::META | Qt::SHIFT | Qt::Key_Tab`; the Backtab form never matches). They forward a synthesized key event into `key_event()`, so keys.config stays authoritative. They do nothing while a popup is open. `InputHandler::get_event_key` also treats `"\u0019"`, the text macOS sends with Shift+Tab, as a special key, so `<S-<tab>>` bindings match.
- Unverified: clicking a tab with the mouse. Synthetic clicks don't reach a background app, and the test copy can't be focused while it shares a bundle id with the installed app.
- Testing without disturbing the user's open app: `build/local-macos/sioyek.app/Contents/MacOS/sioyek --new-instance --instance-name sioyektest tutorial.pdf`, then drive it with `--instance-name sioyektest --execute-command <command> [--execute-command-data <data>]`. AppleScript keystrokes go to the first running Sioyek, because both copies share a bundle id; post key events to the test process's PID instead. A small Swift helper using `CGEvent.postToPid` works for keys. Beware: `q` is bound to `quit`, so a dropped `:` keystroke followed by `q` quits the test instance.

## Working build commands

Use the local helper for this Mac:

```sh
cd /Users/yavo/code/sioyek
./scripts/build_local_macos.sh
```

To install a new build, first quit the running Sioyek app, then run:

```sh
./scripts/build_local_macos.sh --install
```

The helper builds MuPDF with `HAVE_GLUT=no`, uses qmake in a separate build directory with `release non_portable`, builds for the host architecture, copies configs/shaders/tutorial into the bundle, bundles Qt with `macdeployqt`, applies an ad hoc code signature, verifies it, and checks `--version`. Repeated builds are incremental.

When replacing an installed app, the helper refuses to proceed if Sioyek is running and saves the previous bundle under `.local/install-backups/<timestamp>/sioyek.app`. User preferences and databases live outside the bundle.

Overrides:

- `MAKE_PARALLEL`: compilation jobs (default `8`).
- `SIOYEK_QT_ROOT`: alternate Qt SDK path (default `$repo/.local/qt/6.8.2/macos`).
- `SIOYEK_INSTALL_PATH`: alternate app destination (default `/Applications/sioyek.app`).

## Toolchain and SDK recovery

Verified machine: Apple Silicon `arm64`, macOS `27.0.1`, Apple Clang `21.0.0`, macOS SDK `27.0`. Developer directory is `/Library/Developer/CommandLineTools`; the build succeeded with command line tools, without a full Xcode installation.

Qt `6.8.2` is installed locally, using `aqtinstall 3.3.0` in `.local/venv` (created with Homebrew Python `3.14.7`). Required additional Qt modules are `qtspeech`, `qtshadertools`, and `qtmultimedia`; the latter is a dependency of the speech module. The installer also installs the standard base archives, including Qt Declarative, SVG, and Tools.

The existing `/opt/homebrew/anaconda3/bin/qmake` is Qt `5.15.2` and is unsuitable for this development branch. The helper selects the local Qt 6 SDK explicitly, sets its own PATH, and clears inherited Qt plugin/QML paths. Do not replace or uninstall the user's other Qt installations.

If the local SDK or virtualenv is missing, recreate it with:

```sh
cd /Users/yavo/code/sioyek
mkdir -p .local
/opt/homebrew/bin/python3 -m venv .local/venv
.local/venv/bin/python -m pip install 'aqtinstall==3.3.0'
.local/venv/bin/python -m aqt install-qt mac desktop 6.8.2 clang_64 \
  -b https://ftp.fau.de/qtproject --timeout 60 \
  -m qtspeech qtshadertools qtmultimedia -O .local/qt
```

Qt's `clang_64` package contains Apple Silicon support; the helper selects `arm64` on this machine. The default Qt mirror selection failed with connection/certificate errors during setup; the explicit FAU HTTPS mirror above succeeded. Keep certificate and archive integrity checks enabled.

## Compatibility fixes already in the helper

1. On `arm64`, adds `QMAKE_CXXFLAGS+=-include arm_acle.h`. Qt 6.8's `qyieldcpu.h` otherwise fails with newer Apple Clang because `__yield` lacks a declaration.
2. If the current SDK has no `AGL.framework`, removes the obsolete `-framework AGL` entry from the generated Makefile. Qt 6.8's link metadata still includes it, causing `ld: framework 'AGL' not found` on this SDK. This changes generated build output, not the Qt SDK or application sources.

The upstream `build_mac.sh` mutates the tracked `.pro` deployment target and deletes the `build/` directory. Prefer the local helper to retain incremental artifacts and these compatibility fixes. A `CMakeLists.txt` exists but was not used or validated in this setup.

In a restricted Codex sandbox, Qt tools may abort with `Incompatible processor ... neon` because hardware feature queries through `sysctl` are denied. They worked when the required build/launch commands ran with appropriate execution permissions. Do not diagnose this sandbox error as an actual missing CPU feature. Installing into `/Applications` also needs filesystem access outside a repo-only sandbox.

MuPDF objects were built with the host's default macOS 27 target, while the upstream qmake project targets macOS 15. Linker deployment-target warnings were nonfatal on this machine. Older macOS compatibility has not been verified; align deployment targets and rebuild MuPDF if it becomes a requirement.

## Validation and useful locations

Completed checks: native `arm64` executable, bundled Qt runtime, strict/deep signature verification, successful `--version`, visual rendering of the six-page tutorial, keyboard navigation with `2gg` and `gg`, and a successful repeat incremental build. The development checkout still reports `sioyek 2.0.0` because that version string is hardcoded; identify the source revision with Git, not `--version` alone.

Useful checks after relevant changes:

```sh
bash -n scripts/build_local_macos.sh
./scripts/build_local_macos.sh
codesign --verify --deep --strict /Applications/sioyek.app
/Applications/sioyek.app/Contents/MacOS/sioyek --version
open -a /Applications/sioyek.app /Users/yavo/code/sioyek/tutorial.pdf
```

The last three commands check the installed bundle; reinstall first if validating new source changes. Alternatively launch `build/local-macos/sioyek.app` to check a build without installing it. Build output alone and `--version` do not prove PDF rendering works.

- Setup logs: `.local/build-local.log`, `.local/incremental-build.log`, `.local/smoke-test.log`, `.local/mupdf-build.log`, and `.local/qt*.log`. The helper prints to the terminal; redirect output explicitly when a new saved log is needed. A nonfatal `warning: found duplicate fz_icc_link in the store` appeared during the PDF smoke test.
- User configuration: `~/.config/sioyek/prefs_user.config` and `keys_user.config`; Qt application config/data locations are also searched. On this installation, databases were created under `/Users/yavo/Library/Application Support/sioyek/` (`local.db`, `shared.db`, `test.db`). Preserve user annotations, databases, and preferences across reinstalls.
- Main startup, paths, and version: `pdf_viewer/main.cpp`.
- Commands and interaction: `pdf_viewer/main_widget.cpp`, `input.cpp`, `keys.config`, and `utils.cpp`.
- Configuration definitions/defaults: `pdf_viewer/config.cpp` and `prefs.config`.
- Rendering and document handling: `pdf_viewer/pdf_renderer.cpp`, `pdf_view_opengl_widget.cpp`, `document.cpp`, `document_view.cpp`, and `pdf_viewer/shaders/`.
- Native macOS integration: `pdf_viewer/macos_specific.mm` and `resources/Info.plist`.
- Build inputs: `pdf_viewer_build_config.pro` and `resources.qrc`. Upstream documentation: `README.md`.

Before starting new work, inspect the current Git status and read the helper; this handoff describes the setup checkpoint, and later user changes take precedence.
