#!/usr/bin/env bash
set -euo pipefail

# Incremental native macOS build with the same Qt version as the local setup.
repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
qt_dir="${YAVIEW_QT_ROOT:-$repo_dir/.local/qt/6.8.2/macos}"
build_dir="$repo_dir/build/local-macos"
app_path="$build_dir/yaview.app"
jobs="${MAKE_PARALLEL:-8}"
install_app=false

case "${1:-}" in
  "") ;;
  --install) install_app=true ;;
  *) printf 'Usage: %s [--install]\n' "$0" >&2; exit 2 ;;
esac

if [[ ! -x "$qt_dir/bin/qmake" || ! -x "$qt_dir/bin/macdeployqt" ]]; then
  printf 'Qt tools missing in %s; set YAVIEW_QT_ROOT to a Qt 6.8.2 macOS SDK.\n' "$qt_dir" >&2
  exit 1
fi

export PATH="$qt_dir/bin:/usr/bin:/bin:/usr/sbin:/sbin"
unset QT_PLUGIN_PATH QML2_IMPORT_PATH QML_IMPORT_PATH

make -C "$repo_dir/mupdf" HAVE_GLUT=no -j"$jobs"
mkdir -p "$build_dir"
cd "$build_dir"
qmake_extra_args=()
if [[ "$(uname -m)" == arm64 ]]; then
  # Qt 6.8's ARM yield intrinsic needs its declaration with recent Apple Clang.
  qmake_extra_args+=('QMAKE_CXXFLAGS+=-include arm_acle.h')
fi
"$qt_dir/bin/qmake" -spec macx-clang "$repo_dir/pdf_viewer_build_config.pro" \
  'CONFIG+=release non_portable' 'CONFIG-=debug' \
  "QMAKE_APPLE_DEVICE_ARCHS=$(uname -m)" "${qmake_extra_args[@]}"
# Qt 6.8's link metadata names legacy AGL, removed from recent macOS SDKs.
if [[ ! -d "$(xcrun --show-sdk-path)/System/Library/Frameworks/AGL.framework" ]]; then
  /usr/bin/sed -i '' 's/-framework AGL//g' Makefile
fi
make -j"$jobs"

resources_dir="$app_path/Contents/Resources"
mkdir -p "$resources_dir/shaders"
ditto "$repo_dir/pdf_viewer/shaders" "$resources_dir/shaders"
for config_name in prefs.config keys.config; do
  cp "$repo_dir/pdf_viewer/$config_name" "$resources_dir/$config_name"
done
cp "$repo_dir/tutorial.pdf" "$resources_dir/tutorial.pdf"

# Bundle Qt so the installed app also runs without this SDK in PATH.
"$qt_dir/bin/macdeployqt" "$app_path" -always-overwrite -no-strip
codesign --force --deep --sign - "$app_path"
codesign --verify --deep --strict "$app_path"
"$app_path/Contents/MacOS/yaview" --version

if "$install_app"; then
  install_target="${YAVIEW_INSTALL_PATH:-/Applications/yaview.app}"
  if [[ -e "$install_target" || -L "$install_target" ]]; then
    if pgrep -x yaview >/dev/null; then
      printf 'Quit yaview before replacing the installed app.\n' >&2
      exit 1
    fi
    backup_dir="$repo_dir/.local/install-backups/$(date +%Y%m%d-%H%M%S)"
    mkdir -p "$backup_dir"
    mv "$install_target" "$backup_dir/yaview.app"
    printf 'Previous installation saved to %s\n' "$backup_dir/yaview.app"
  fi
  ditto "$app_path" "$install_target"
  # ditto keeps the build's timestamps, and the Dock and Finder keep showing a
  # cached icon while the bundle looks unchanged. Mark it as changed.
  touch "$install_target" "$install_target/Contents/Info.plist"
  /System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister -f "$install_target"
  codesign --verify --deep --strict "$install_target"
  printf 'Installed: %s\n' "$install_target"
else
  printf 'Built: %s\n' "$app_path"
fi
