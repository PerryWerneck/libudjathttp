#!/bin/bash
make Debug
if [ "$?" != "0" ]; then
	exit -1
fi

MODULEDIR=$(pkg-config --variable module_path libudjat)

mkdir -p ${MODULEDIR}
sudo ln -sf $(readlink -f .bin/Debug/udjat-module-*.so) ${MODULEDIR}
if [ "$?" != "0" ]; then
	exit -1
fi

sudo ln -sf $(readlink -f .bin/Debug/libudjathttp.so.*.*) /usr/lib64
if [ "$?" != "0" ]; then
	exit -1
fi

sudo ln -sf $(readlink -f .bin/Debug/*.a) /usr/lib64
if [ "$?" != "0" ]; then
	exit -1
fi

