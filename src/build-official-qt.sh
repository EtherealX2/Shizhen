#!/usr/bin/env bash
# 拾帧（Shizhen）Windows 构建脚本
#
# 使用 Qt 官方 5.15.2 mingw81_64 套件（不带 ICU/HarfBuzz/GLib 依赖），
# 配合 Qt 官方 MinGW 8.1 编译器，产出 40MB 左右的绿色包。
#
# 依赖（只需装一次，aqtinstall 可从 pypi 安装）：
#   python -m aqt install-qt windows desktop 5.15.2 win64_mingw81 -O C:/Qt \
#          --archives qtbase qtsvg qtwinextras
#   python -m aqt install-tool windows desktop tools_mingw qt.tools.win64_mingw810 -O C:/Qt
#
# 用法：
#   bash build-official-qt.sh            # 默认从 C:/Qt 找 Qt 与编译器
#   QTDIR=/path/to/mingw81_64 MINGW=/path/to/mingw810_64/bin bash build-official-qt.sh
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# CMake on Windows does not resolve MSYS/Git-Bash style "/d/..." paths when they
# are handed over through CMAKE_PREFIX_PATH, so normalise everything to "D:/...".
if command -v cygpath >/dev/null 2>&1; then
  ROOT="$(cygpath -m "$ROOT")"
fi
QTDIR="${QTDIR:-C:/Qt/5.15.2/mingw81_64}"
MINGW="${MINGW:-C:/Qt/Tools/mingw810_64/bin}"
PREFIX="${PREFIX:-$ROOT/prefix}"
DIST="${DIST:-$ROOT/../Shizhen}"

export PATH="$MINGW:$PATH"

CXX="$MINGW/g++.exe"
CC="$MINGW/gcc.exe"

COMMON=(
  -G Ninja
  -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_C_COMPILER="$CC"
  -DCMAKE_CXX_COMPILER="$CXX"
  -DCMAKE_PREFIX_PATH="$QTDIR;$PREFIX"
  -DBUILD_TESTS=OFF
  -DBUILD_EXAMPLE=OFF
)

echo "### kColorPicker"
cmake -S "$ROOT/kColorPicker" -B "$ROOT/build-kcp" "${COMMON[@]}" -DCMAKE_INSTALL_PREFIX="$PREFIX"
cmake --build "$ROOT/build-kcp" -j8
cmake --install "$ROOT/build-kcp"

echo "### kImageAnnotator"
cmake -S "$ROOT/kImageAnnotator" -B "$ROOT/build-kia" "${COMMON[@]}" -DCMAKE_INSTALL_PREFIX="$PREFIX"
cmake --build "$ROOT/build-kia" -j8
cmake --install "$ROOT/build-kia"

echo "### 拾帧 (ksnip)"
cmake -S "$ROOT/ksnip" -B "$ROOT/build-shizhen" "${COMMON[@]}" -DCMAKE_INSTALL_PREFIX="$DIST"
cmake --build "$ROOT/build-shizhen" -j8
cmake --install "$ROOT/build-shizhen"

echo "### 部署 Qt 运行库（windeployqt 在部分环境下找不到平台插件，这里直接拷最小集）"
Q="$QTDIR"
mkdir -p "$DIST/platforms" "$DIST/imageformats" "$DIST/iconengines" "$DIST/printsupport"
for dll in Qt5Core Qt5Gui Qt5Widgets Qt5Network Qt5Svg Qt5PrintSupport Qt5Xml Qt5WinExtras; do
  cp -f "$Q/bin/$dll.dll" "$DIST/"
done
cp -f "$Q/plugins/platforms/qwindows.dll" "$DIST/platforms/"
cp -f "$Q/plugins/imageformats/qjpeg.dll" "$DIST/imageformats/"
cp -f "$Q/plugins/imageformats/qico.dll" "$DIST/imageformats/"
cp -f "$Q/plugins/imageformats/qgif.dll" "$DIST/imageformats/"
cp -f "$Q/plugins/imageformats/qsvg.dll" "$DIST/imageformats/"
cp -f "$Q/plugins/iconengines/qsvgicon.dll" "$DIST/iconengines/"
cp -f "$Q/plugins/printsupport/windowsprintersupport.dll" "$DIST/printsupport/"
for dll in libgcc_s_seh-1 libstdc++-6 libwinpthread-1; do
  cp -f "$MINGW/$dll.dll" "$DIST/"
done

echo "### 只保留中文翻译，其余删除"
find "$DIST/translations" -type f ! -name "*zh_CN.qm" ! -name "*zh_Hant.qm" -delete

echo "### done -> $DIST"
du -sh "$DIST"
