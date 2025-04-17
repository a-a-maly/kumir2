Kumir 2.x programming system
=============================================

***NOTE*** Maintainers from AltLinux please read this: [MAINTAINERS_ru.md](MAINTAINERS_ru.md) (in Russian).

Branches and tags
---------------------------------------------

Branch `master` contains Russian Algorithmic language implementation and all 
system modules, even not included in release.

Branch `python` contains Python-3 language implementation.

Releases are maked by tags and lists of excluded modules.
Run `remove-unstable-modules-v.X.X.X.sh` before bundling source package.

Build requirements (Linux)
---------------------------------------------

1. CMake version at least 2.8.11
2. Python interpreter version at least 2.7.0
3. Qt4 SDK version at least 4.7.0. It is possible to build using Qt5 (>= 5.3.0)
4. ZLib development files
5. Boost 1.54.0 development files. Required Boost files are bundled into
this repository, but you can use your distribution provided package by 
deleting `src/3rdparty/boost-1.54.0`
6. LLVM development files version at least 3.4 are optional to build native code
generation feature
7. Python development files at least 3.2 in case of building branch `python`

Build requirements (Windows)
---------------------------------------------

1. CMake version 2.8.11. There is known regression in version 2.8.12, so do
not use it
2. Python interpreter version at least 2.7.0
3. Microsoft Visual Studio Express 2010 or 2012
4. Qt4 SDK version at least 4.8.0. Qt5 not well-tested on this platform
5. Boost and ZLib development files which bundled in this repository
6. Python development files at least 3.2 in case of building branch `python`

In order to build native code generation feature on Windows you must use
MSVC2012 and provide several requirements:

1. Boost version at least 1.57.0 due to incompatibility of version 1.54 with
MSVC2012
2. LLVM development files version *exact* 3.4.0 prebuilt using *MSVC2012 
toolchain*
3. CLang compiler version *exact* 3.4.0 prebuilt using *MinGW 4.8 toolchain* 
4. MinGW version *exact* 4.6.1 files

Build and source-install instructions (Linux)
---------------------------------------------

Meet all requirements (see above)

Run cmake in separate directory: 
```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

If you want to use Qt5 instead of Qt4, pass `-DUSE_QT=5` option to cmake.

Run make:
```
make
```
This will build entire project into `build/Release` directory.

To install in `/usr/local/` prefix run `make install`.

In order to provide custom prefix, you can pass option
`-DCMAKE_INSTALL_PREFIX=` to cmake.

Build instructions (Windows)
---------------------------------------------

Meet all requirements and ensure that `qmake.exe`, `cmake.exe` and `python.exe`
are present in system `PATH` environment variable. Also ensure `QTDIR` and
`QMAKESPEC` environment variables are set to match your Qt and compiler
toolchain installation.

Start Microsoft Visual Studio Tools console.

Within the console create subdirectory `build` of project root, walk there and
run:
```
cmake -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles" ..
nmake
```

This will build entire project into `build/Release` directory.

To build native code generation feature there are additional options required
to be passed to cmake:

1. `-DCLANG_EXECUTABLE=` - path to `clang.exe`, which built using MinGW 
toolchain. This CLang compiler is used to generate LLVM bytecode for standard 
and runtime libraries
2. `-DLLVM_ROOT=`, `-DLlvm_INCLUDE_DIR=` and `-DLlvm_CONFIG_EXECUTABLE=` - 
paths to prebuild LLVM root directory, `include` subdirectory and 
`llvm-config.exe` executable
3. `-DBOOST_ROOT=` - path to Boost root version at least 1.57.0. Remove 
bundled boost first

Deployment 3-rd party libraries (Windows)
---------------------------------------------

The following files must be copied into `bin` build subdirectory:
```
phonon4.dll
QtCore4.dll
QtSql4.dll
QtDeclarative4.dll
QtGui4.dll
QtNetwork4.dll
QtOpenGL4.dll
QtScript4.dll
QtSvg4.dll
QtWebKit4.dll
QtXml4.dll
QtXmlPatterns4.dll
```

In order to use native code generation feature the following files from MinGW 
*version 4.6.1* must be copied into `llvm-mingw` build subdirectory:
```
as.exe
crt2.o
crtbegin.o
crtend.o
ld.exe
libadvapi32.a
libgcc.a
libgcc_s_dw2-1.dll
libkernel32.a
libmingw32.a
libmingwex.a
libmoldname.a
libmsvcrt.a
libpthread.a
libshell32.a
libstdc++-6.dll
libstdc++.a
libuser32.a
llc.exe
```
