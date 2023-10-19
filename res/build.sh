#!/bin/bash


SELFDIR=$(dirname "$0")
KN=$(uname -s)

if test "$KN" = "Linux";then
    bash ${SELFDIR}/build_appimage.sh
elif test "$KN" = "Darwin";then
    bash ${SELFDIR}/build_dmg.sh
else
    echo "Not ready for `$KN`"
    exit 1
fi
