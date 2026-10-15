# Building yaview

yaview needs Qt 6.7 or 6.8. The Linux and Windows steps below still say Qt 5; they come from sioyek and predate its move to Qt 6.

## Linux

### Fedora

Run the following commands to install dependencies, clone the repository and compile yaview on Fedora (tested on Fedora Workstation 36).

```
sudo dnf install qt5-qtbase-devel qt5-qtbase-static qt5-qt3d-devel harfbuzz-devel
git clone --recursive https://github.com/alexyavo/yaview
cd yaview
./build_linux.sh
```

### Generic distribution
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

## Windows
1. Install Visual Studio (tested on 2019, other relatively recent versions should work too)
2. Install Qt 5 and make sure qmake is in `PATH`.
3. Clone the repository and build using 64 bit Visual Studio Developer Command Prompt:
```
git clone --recursive https://github.com/alexyavo/yaview
cd yaview
build_windows.bat
```

## Mac
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
4. Clone the repository, build and install. (On Apple Silicon with a local Qt SDK, [`scripts/build_local_macos.sh`](../scripts/build_local_macos.sh) `--install`
   wraps these steps; see [`AGENTS.md`](../AGENTS.md).)
```
git clone --recursive https://github.com/alexyavo/yaview
cd yaview
chmod +x build_mac.sh
setopt PIPE_FAIL PRINT_EXIT_VALUE ERR_RETURN SOURCE_TRACE XTRACE
MAKE_PARALLEL=8 ./build_mac.sh

mv build/yaview.app /Applications/
sudo codesign --force --sign - --deep /Applications/yaview.app
```
