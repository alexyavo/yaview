#!/usr/bin/env bash
set -e
# the paths below are relative to the repository root
cd "$(dirname "$0")/.."

if [ -z ${MAKE_PARALLEL+x} ]; then export MAKE_PARALLEL=$(nproc); else echo "MAKE_PARALLEL defined"; fi
echo "MAKE_PARALLEL set to $MAKE_PARALLEL"

# download linuxdeployqt if not exists
if [[ ! -f linuxdeployqt-continuous-x86_64.AppImage ]]; then
	wget -q https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
	chmod +x linuxdeployqt-continuous-x86_64.AppImage
fi

cd mupdf
make USE_SYSTEM_HARFBUZZ=yes -j$MAKE_PARALLEL
cd ..

if [ -z ${QMAKE+x} ]; then
    QMAKE=qmake
fi

if [[ $1 == console ]]; then
	$QMAKE "CONFIG+=linux_app_image console" pdf_viewer_build_config.pro
else
	$QMAKE "CONFIG+=linux_app_image" pdf_viewer_build_config.pro
fi

rm -rf yaview-release 2> /dev/null
make install INSTALL_ROOT=yaview-release -j$MAKE_PARALLEL

cp pdf_viewer/prefs.config yaview-release/usr/bin/prefs.config
cp pdf_viewer/keys.config yaview-release/usr/bin/keys.config
cp -r pdf_viewer/shaders yaview-release/usr/bin/shaders

./linuxdeployqt-continuous-x86_64.AppImage ./yaview-release/usr/share/applications/yaview.desktop -qmldir=./pdf_viewer/touchui -appimage
# ./linuxdeployqt-continuous-x86_64.AppImage ./yaview-release/usr/share/applications/yaview.desktop -appimage


mv Yaview-* Yaview-x86_64.AppImage
zip yaview-release-linux.zip Yaview-x86_64.AppImage
