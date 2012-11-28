#!/bin/bash
emcc  slt.o ../../tsk3/.libs/libtsk3.a -o slt.js --pre-js slt-pre.js -s RUNTIME_LINKED_LIBS="['qemu-img-lib.js']" 
