How to compile sleuthkit using emscripten for VMXRay?
-----------------------------------------------------

* Pre-requisites:
-----------------

- Clang + llvm 
  - Get clang+llvm-3.1-x86_64-linux-ubuntu_12.04

- Emscripten from VMXRay git repository 
  - Repo: git@github.com:coriolis/emscripten.git
  - Branch: master

- QEMU from VMXRay git repository 
  - Repo: git@github.com:coriolis/QEMU.git 
  - Branch: emscripten

- Sleuthkit from VMXRay git repository 
  - Repo: git@github.com:coriolis/sleuthkit-1.git 
  - Branch: emscripten

- VM inspection from VMXRay git repository 
  - Repo: git@github.com:coriolis/vminspection.git 
  - Branch: emscripten


* Compilation:
--------------

- Set PATH to include emscripten 

  $ export PATH=<your_emscripten_path>:$PATH


- Compile QEMU library - qemu-img-lib.bc

  $ cd <qemu_library>
  $ emconfigure ./configure --disable-kvm --disable-spice --disable-guest-agent
  $ emmake make
  $ ./make_qemu_lib_js.sh
  
  - This will generate "qemu-img-lib.bc" in current folder.
  - "emmake make" will give some compilation errors. That is because QEMU lib
  hasn't been completely ported for emscripten. However, required modules,
  files for VMXRay get compiled appropriately.

- Compile sleuthkit - slt.js

 $ cd <sleuthkit>
 $ cp <qemu_library>/qemu-img-lib.bc tools/fstools/
   - Copy required QEMU library
 $ emconfigure ./configure
 $ emmake make
 $ cd tools/fstools
 $ ./make_slt_js.sh

 - The last step will give "slt.js" that is used on the VMXRay website.



