#!/bin/bash


SELFDIR=$(dirname $(readlink -f "$0"))
PROJECT_DIR=$(readlink -f ${SELFDIR}/..)

BUILD_DIR=${PROJECT_DIR}/build-$(uname -m)-appimage
APP_NAME=$(basename ${PROJECT_DIR})
CMAKE=cmake
LINUXDEPLOYQT=/linuxdeployqt/linuxdeployqt-5-x86_64.AppImage
#test -x "${LINUXDEPLOYQT}" ||LINUXDEPLOYQT=/linuxdeployqt/linuxdeployqt-continuous-x86_64.AppImage


APP_DIR=/tmp/${APP_NAME}/AppDir
QT_DIR=/Qt/5.11.3
QMAKE=${QT_DIR}/gcc_64/bin/qmake
CMAKE=/Qt/Tools/CMake/bin/cmake

test -d "${QT_DIR}" || {
QT_DIR=$(echo /Qt/[0-9][0-9.]*|head -n1)
}

test -d "${QT_DIR}" || {
echo "QT_DIR=${QT_DIR}"
exit -1
}

rm -rf "${APP_DIR}"
export DESTDIR=${APP_DIR}
rm -rf "${BUILD_DIR}"
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

${CMAKE} -DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_FRAMEWORK_PATH=${QT_DIR}/gcc_64/lib \
	-DCMAKE_VERBOSE_MAKEFILE=on \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DBUILD_APPIMAGE=1	\
	-DSHOW_ABOUT_QT=1 \
	-DCMAKE_PREFIX_PATH=${QT_DIR}/gcc_64 ..
${CMAKE} --build .|| exit 1

mkdir -p "${APP_DIR}"

${CMAKE} -P cmake_install.cmake
ln -sf WindowsDrive "${APP_DIR}"/usr/bin/windows-drive
mkdir -p "${APP_DIR}/usr/lib" && { 
	for f in /usr/lib/x86_64-linux-gnu/libssl.so.1.0.0 /lib/x86_64-linux-gnu/libssl.so.1.0.0
	do
	  if test -f ${f} ;then
	    cp -va ${f} "${APP_DIR}/usr/lib"
	    break
	  fi
	done
}

verf=/tmp/.verf
grep VERSION ${BUILD_DIR}/config.h |sed "s|.*define[ \t]\+||g;s|[)(]||g;s|[ \t]\+|=|g;s|VERSION|VER|g" >${verf}
source ${verf}

export VERSION="${VER}-r${VER_CODE}"
export PATH=${QT_DIR}/gcc_64/bin:$PATH
export LD_LIBRARY_PATH=${QT_DIR}/gcc_64/lib
${LINUXDEPLOYQT} \
	"${APP_DIR}"/usr/share/applications/windows-drive.desktop \
	-always-overwrite \
	-appimage -verbose=1 \
	-qmake=${QMAKE} \
	-executable="${APP_DIR}"/usr/bin/wd_regf \
	-executable="${APP_DIR}"/usr/bin/wd_helper \
	-executable="${APP_DIR}/usr/lib/libssl.so.1.0.0" \
	-bundle-non-qt-libs \
	-no-copy-copyright-files \
	-no-translations \
	-extra-plugins=platforms/libqwayland-generic.so,imageformats/libqsvg.so
