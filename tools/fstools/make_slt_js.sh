#!/bin/bash
if [ ! -f ./qemu-img-lib.bc ] ; then
    echo "Error: file  qemu-img-lib.bc not found."
    echo "Copy qemu-img-lib.bc file from QEMU source directory in this folder, generated after compiling QEMU."
    exit 0
fi

emcc  slt.o ../../tsk3/.libs/libtsk3.a ./qemu-img-lib.bc -o slt.js --pre-js slt-pre.js 
#-s RUNTIME_LINKED_LIBS="['qemu-img-lib.js']" 
