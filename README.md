# yaview

[![Memory leak tests](https://github.com/alexyavo/yaview/actions/workflows/memory-tests.yml/badge.svg)](https://github.com/alexyavo/yaview/actions/workflows/memory-tests.yml)

yaview is a PDF viewer with a focus on textbooks and research papers.

yaview is a fork of [sioyek](https://github.com/ahrm/sioyek) by [ahrm](https://github.com/ahrm) and the sioyek
contributors. It was renamed from sioyek and modified by [alexyavo](https://github.com/alexyavo) starting on
2026-10-14. Like sioyek, it is free software licensed under the GNU General Public License version 3 (see
[LICENSE](LICENSE) and [NOTICE](NOTICE)). The git history records every change made on top of sioyek.

Changes compared to sioyek include a tab bar (one tab per open document, `:q` closes the current tab, Ctrl+Tab and
Ctrl+Shift+Tab switch tabs), vim and emacs style keys (`h`/`l` horizontal scrolling, Ctrl+D/Ctrl+U paging, Ctrl+N/Ctrl+P
in menus), themable popup menus that follow the system light/dark appearance, Kagi as the default `ss` search
engine and highlighting of all visible search results.

# Development Branch FAQ

## Q: There are build errors with Qt 5.*.

A: If you are building the development branch you need to use Qt 6.7 or 6.8.

## Q: On MacOS I get "yaview is damaged and cannot be opened. It is recommended to eject the image.".

A: This is related to macOS quarantine. See https://github.com/ahrm/sioyek/discussions/1156#discussioncomment-10822738 .


## Contents
* [Installation](#install)
* [Documentation](#documentation)
* [Video Demo](#feature-video-overview)
* [Features](#features)
* [Build Instructions](#build-instructions)
* [Buy Me a Coffee (or a Book!)](#donation)

## Install
yaview has no prebuilt releases or packages; build it from source (see [Build Instructions](#build-instructions)).
The installers and distribution packages listed in [sioyek's README](https://github.com/ahrm/sioyek#install) install
the original sioyek, not yaview.


## Documentation
yaview has no documentation of its own. [sioyek's documentation](https://sioyek-documentation.readthedocs.io/en/latest/)
applies, with `yaview` in place of `sioyek` in file and folder names.
## Feature Video Overview

These videos were made for sioyek.

[![sioyek feature overview](https://img.youtube.com/vi/yTmCI0Xp5vI/0.jpg)](https://www.youtube.com/watch?v=yTmCI0Xp5vI)

For a more in-depth tutorial, see this video:

[![sioyek tutorial](https://img.youtube.com/vi/RaHRvnb0dY8/0.jpg)](https://www.youtube.com/watch?v=RaHRvnb0dY8)

## Features

### Quick Open

https://user-images.githubusercontent.com/6392321/125321111-9b29dc00-e351-11eb-873e-94ea30016a05.mp4

You can quickly search and open any file you have previously interacted with using yaview.

### Table of Contents

https://user-images.githubusercontent.com/6392321/125321313-cf050180-e351-11eb-9275-c2759c684af5.mp4

You can search and jump to table of contents entries.

### Smart Jump

https://user-images.githubusercontent.com/6392321/125321419-e5ab5880-e351-11eb-9688-95374a22774f.mp4

You can jump to any referenced figure or bibliography item *even if the PDF file doesn't provide links*. You can also search the names of bibliography items in google scholar/libgen by middle clicking/shift+middle clicking on their name.

### Overview

https://user-images.githubusercontent.com/6392321/154683015-0bae4f92-78e2-4141-8446-49dd7c2bd7c9.mp4

You can open a quick overview of figures/references/tables/etc. by right clicking on them (Like Smart Jump, this feature works even if the document doesn't provide links).

### Mark

https://user-images.githubusercontent.com/6392321/125321811-505c9400-e352-11eb-85e0-ffc3ae5f8cb8.mp4

Sometimes when reading a document you need to go back a few pages (perhaps to view a definition or something) and quickly jump back to where you were. You can achieve this by using marks. Marks are named locations within a PDF file (each mark has a single character name for example 'a' or 'm') which you can quickly jump to using their name. In the aforementioned example, before going back to the definition you mark your location and later jump back to the mark by invoking its name. Lower case marks are local to the document and upper case marks are global (this should be very familiar to you if you have used vim).

### Bookmarks

https://user-images.githubusercontent.com/6392321/125322503-1a6bdf80-e353-11eb-8018-5e8fc43b8d05.mp4

Bookmarks are similar to marks except they are named by a text string and they are all global.

### Highlights


https://user-images.githubusercontent.com/6392321/130956728-7e0a87fa-4ada-4108-a8fc-9d9d04180f56.mp4


Highlight text using different kinds of highlights. You can search among all the highlights.

### Portals (this feature is most useful for users with multiple monitors)



https://user-images.githubusercontent.com/6392321/125322657-41c2ac80-e353-11eb-985e-8f3ce9808f67.mp4

Suppose you are reading a paragraph which references a figure which is not very close to the current location. Jumping back and forth between the current paragraph and the figure can be very annoying. Using portals, you can link the paragraph's location to the figure's location. Yaview shows the closest portal destination in a separate window (which is usually placed on a second monitor). This window is automatically updated to show the closest portal destination as the user navigates the document.


### Configuration


https://user-images.githubusercontent.com/6392321/125337160-e4832700-e363-11eb-8801-0bee58121c2d.mp4

You can customize all key bindings and some UI elements by editing `keys_user.config` and `prefs_user.config`. The default configurations are in `keys.config` and `prefs.config`.



## Build Instructions

### Linux

#### Fedora

Run the following commands to install dependencies, clone the repository and compile yaview on Fedora (tested on Fedora Workstation 36).

```
sudo dnf install qt5-qtbase-devel qt5-qtbase-static qt5-qt3d-devel harfbuzz-devel
git clone --recursive https://github.com/alexyavo/yaview
cd yaview
./build_linux.sh
``` 

#### Generic distribution
1. Install Qt 5 and make sure `qmake` is in `PATH`.

    Run `qmake --version` to make sure the `qmake` in path is using Qt 5.x.
2. Install `libharfbuzz`:
```
sudo apt install libharfbuzz-dev
```
3. Clone the repository and build:
```
git clone --recursive https://github.com/alexyavo/yaview
cd yaview
./build_linux.sh
```

### Windows
1. Install Visual Studio (tested on 2019, other relatively recent versions should work too)
2. Install Qt 5 and make sure qmake is in `PATH`.
3. Clone the repository and build using 64 bit Visual Studio Developer Command Prompt:
```
git clone --recursive https://github.com/alexyavo/yaview
cd yaview
build_windows.bat
```

### Mac
1. Uninstall previous Qt6 installed by Homebrew
2. Install Xcode.
3. Install Qt6.
```
pip install aqtinstall
cd /path/to/qt
aqt install-qt mac desktop 6.8.2 clang_64 -m all
export Qt6_DIR=/path/to/qt/6.8.2/macos/
export QT_PLUGIN_PATH=/path/to/qt/6.8.2/macos/plugins
export PKG_CONFIG_PATH=/path/to/qt/6.8.2/macos/lib/pkgconfig
export QML2_IMPORT_PATH=/path/to/qt/6.8.2/macos/qml
export PATH="/path/to/qt/6.8.2/macos/bin:$PATH"
```
4. Clone the repository, build and install. (On Apple Silicon with a local Qt SDK, `scripts/build_local_macos.sh --install`
   wraps these steps; see `AGENTS.md`.)
```
git clone --recursive https://github.com/alexyavo/yaview
cd yaview
chmod +x build_mac.sh
setopt PIPE_FAIL PRINT_EXIT_VALUE ERR_RETURN SOURCE_TRACE XTRACE
MAKE_PARALLEL=8 ./build_mac.sh

mv build/yaview.app /Applications/
sudo codesign --force --sign - --deep /Applications/yaview.app
```

## Donation
yaview is built on the work of sioyek's author. If you enjoy it, please consider supporting sioyek's development.

<a href="https://www.buymeacoffee.com/ahrm" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/default-orange.png" alt="Buy Me A Coffee" height="41" width="174"></a>
