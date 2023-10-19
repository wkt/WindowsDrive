#!/bin/bash

infile=$1
if test -z "${infile}";then
	exit 1
fi

outfile=$2
test -z "${outfile}" && {
	outfile="icon.icns"
	i=1
	while test -f "$outfile";
	do
		outfile="icon_$i.icns"
		let i=$i+1
	done
}

tmpdir="/tmp/${USER}_tmp.iconset"
mkdir -p "${tmpdir}"
rm -rf "${tmpdir}"/*

sips -z 16 16     ${infile} --out "${tmpdir}"/icon_16x16.png >/dev/null
sips -z 32 32     ${infile} --out ${tmpdir}/icon_16x16@2x.png >/dev/null
sips -z 32 32     ${infile} --out ${tmpdir}/icon_32x32.png >/dev/null
sips -z 64 64     ${infile} --out ${tmpdir}/icon_32x32@2x.png >/dev/null
sips -z 128 128   ${infile} --out ${tmpdir}/icon_128x128.png >/dev/null
sips -z 256 256   ${infile} --out ${tmpdir}/icon_128x128@2x.png >/dev/null
sips -z 256 256   ${infile} --out ${tmpdir}/icon_256x256.png >/dev/null
sips -z 512 512   ${infile} --out ${tmpdir}/icon_256x256@2x.png >/dev/null
sips -z 512 512   ${infile} --out ${tmpdir}/icon_512x512.png >/dev/null
sips -z 1024 1024 ${infile} --out ${tmpdir}/icon_512x512@2x.png >/dev/null

iconutil -c icns ${tmpdir} -o ${outfile} && echo "Convert ${infile} to ${outfile}"
rm -rf ${tmpdir}