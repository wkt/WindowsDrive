#!/bin/bash


realpath() {
    [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
}


export MACOSX_DEPLOYMENT_TARGET=10.9

SELFDIR=$(dirname $(realpath "$0"))
BUILD_DIR=${SELFDIR}/../build-$(uname -m)-macOS
QT_DIR=$(for f in ~/Applications/Qt/[0-9.]* ~/Qt5/[0-9.]*;do test -d "$f" && echo "$f";done |head -n1)
CMAKE="${QT_DIR}"/../Tools/CMake/CMake.app/Contents/bin/cmake
MACDEPLOYQT="${QT_DIR}"/clang_64/bin/macdeployqt
APP_NAME=WindowsDrive
MAKE=make

rm -rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

${CMAKE} -DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_FRAMEWORK_PATH=${QT_DIR}/clang_64/lib \
	-DCMAKE_VERBOSE_MAKEFILE=on \
    -DSHOW_ABOUT_QT=1 \
	-DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET}" \
	-DCMAKE_PREFIX_PATH=${QT_DIR}/clang_64 ..
${MAKE} || exit 1

${MACDEPLOYQT} ${BUILD_DIR}/${APP_NAME}.app -executable=${BUILD_DIR}/${APP_NAME}.app/Contents/MacOS/wd_regf || exit 2


usage()
{
cat <<__end
Usage:
    `basename $0` [release string] 
__end
}

is_app()
{
    test -d "$1" && (echo $1|grep -q '\.app$')
    return $?
}

appfile=${BUILD_DIR}/${APP_NAME}.app
release="$1"

is_app "$appfile" || {
    usage
    exit 1
}


appDir="$(dirname "$appfile")"

applicationName=$(basename "$appfile"|sed 's|\.app$||g')
appDisName=$(mdls "$appfile"|grep kMDItemDisplayName|sed 's/.*=[[:space:]]"//g;s/"//g')

title=$(echo $applicationName|sed 's|[ \t]||g')

source="$appDir/tmp"

rm -rf "$source"
mkdir -p "$source/.background"

##复制.app
cp -a "$appfile" "$source"

ln -sf /Applications   "$source"

dmgOutDir="$appDir/output"
mkdir -p "${dmgOutDir}"


if test -z "${release}";then
	version_short=$(plutil -convert json -r  -o -  "$source/${applicationName}.app/Contents/Info.plist" |grep '"CFBundleShortVersionString"' |sed 's|.*[\t ]\"||g;s|\"[,]*||g')
	version_full=$(plutil -convert json -r  -o -  "$source/${applicationName}.app/Contents/Info.plist" |grep '"CFBundleVersion"'|sed 's|.*[\t ]\"||g;s|\"[,]*||g')
	test -n "$version_full" && release="${version_full}" || release="${version_short}"
fi

finalDMGName="${dmgOutDir}/${title}-${release}.dmg"

dmgpath="${finalDMGName}.dmg"

hdiutil eject /Volumes/"${title}" &>/dev/null
rm -rf $dmgpath

hdiutil create -format UDRW -fs HFS+J -volname "${title}" -srcfolder "${source}" "$dmgpath"
sleep 1
device=$(hdiutil attach -readwrite -noverify -noautoopen "$dmgpath" | egrep '^/dev/' | sed 1q | awk '{print $1}')
sleep 1

{
chmod -Rf go-w /Volumes/"${title}" &>/dev/null
sync
sync
hdiutil detach "${device}" &>/dev/null
rm -rf "${finalDMGName}"
hdiutil convert "$dmgpath" -format UDBZ -imagekey zlib-level=9 -o "${finalDMGName}"
rm -f "$dmgpath"
}
