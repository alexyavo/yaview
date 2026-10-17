REM the paths below are relative to the repository root
cd /d "%~dp0.."
cd mupdf\platform\win32\
msbuild -maxcpucount mupdf.sln /m /property:Configuration=Debug /property:MultiProcessorCompilation=true
msbuild -maxcpucount mupdf.sln /m /property:Configuration=Release /property:MultiProcessorCompilation=true
cd ..\..\..

cd zlib
nmake -f win32/makefile.msc
cd ..

qmake -tp vc "DEFINES+=NON_PORTABLE" "CONFIG+=release" pdf_viewer_build_config.pro

msbuild -maxcpucount yaview.vcxproj /m /property:Configuration=Release
rm -r yaview-release-windows 2> NUL
mkdir yaview-release-windows
cp release\yaview.exe yaview-release-windows\yaview.exe
cp pdf_viewer\keys.config yaview-release-windows\keys.config
cp pdf_viewer\prefs.config yaview-release-windows\prefs.config
cp -r pdf_viewer\shaders yaview-release-windows\shaders
cp tutorial.pdf yaview-release-windows\tutorial.pdf
windeployqt --qmldir ./pdf_viewer/touchui --release yaview-release-windows\yaview.exe
REM windeployqt yaview-release-windows\yaview.exe
cp windows_runtime\vcruntime140_1.dll yaview-release-windows\vcruntime140_1.dll
cp windows_runtime\libssl-1_1-x64.dll yaview-release-windows\libssl-1_1-x64.dll
cp windows_runtime\libcrypto-1_1-x64.dll yaview-release-windows\libcrypto-1_1-x64.dll

if %1 == portable (
    7z a yaview-release-windows-portable.zip yaview-release-windows

) else (
    7z a yaview-release-windows.zip yaview-release-windows
)
