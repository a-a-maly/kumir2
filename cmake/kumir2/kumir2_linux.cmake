# To make LSB-compliant structure all project shared libraries
# should be placed into 'lib' subdirectory of /usr [/usr/local].
# Some distributions have distinct lib64 directory for 64-bit libraries.
# LIB_BASENAME value is a correct 'lib' dir name for current distro.
if((${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64") AND NOT (EXISTS /etc/ubuntu-release) AND NOT (EXISTS /etc/debian_version))
    set(LIB_BASENAME lib64)
else()
    set(LIB_BASENAME lib)
endif()

# Build filesystem layout, according to LSB
set(KUMIR2_EXEC_DIR "bin")                                  # executable binaries
set(KUMIR2_LIBS_DIR "${LIB_BASENAME}/kumir2")               # shared libraries
set(KUMIR2_PLUGINS_DIR "${LIB_BASENAME}/kumir2/plugins")    # libraries to be load at run time
set(KUMIR2_RESOURCES_DIR "share/kumir2")                    # non-executable resources
set(KUMIR2_XDG_APPLICATIONS_DIR "share/applications")       # X applications catalog
set(KUMIR2_XDG_ICONS_DIR "share/icons")                     # X icons catalog
set(KUMIR2_XDG_PIXMAPS_DIR "share/pixmaps")                 # X pixmaps directory
set(KUMIR2_XDG_MIME_PACKAGES_DIR "share/mime/packages")     # X MIME packages catalog

if(NOT DEFINED KUMIR2_LIBEXECS_DIR)
    # various Linux distros have different locations for supplementary executables,
    # so allow to keep previously defined variable if set
    set(KUMIR2_LIBEXECS_DIR "bin")                          # executable supplementary binaries
endif(NOT DEFINED KUMIR2_LIBEXECS_DIR)

if(NOT DEFINED KUMIR2_SDK_SCRIPTS_DIR)
    # various Linux distros have different locations for SDK supplementary executables,
    # so allow to keep previously defined variable if set
    set(KUMIR2_SDK_SCRIPTS_DIR "${KUMIR2_INSTALL_PREFIX}/${KUMIR2_LIBS_DIR}")          # devel scripts
endif(NOT DEFINED KUMIR2_SDK_SCRIPTS_DIR)

if(NOT DEFINED KUMIR2_SDK_CMAKE_DIR)
    set(KUMIR2_SDK_CMAKE_DIR "${LIB_BASENAME}/cmake/Kumir2") # cmake files to use from others
endif(NOT DEFINED KUMIR2_SDK_CMAKE_DIR)

if(NOT DEFINED KUMIR2_SDK_SRC_DIR)
    set(KUMIR2_SDK_SRC_DIR "${KUMIR2_RESOURCES_DIR}/src")
endif(NOT DEFINED KUMIR2_SDK_SRC_DIR)  # sources required to build some components (launchers, etc.)

# clear default CMake RPATH values
set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
set(CMAKE_INSTALL_RPATH "")

# Compile flags
set(KUMIR2_CXXFLAGS "-fno-exceptions -std=c++0x -fPIC")
set(KUMIR2_CXXFLAGS_Release "-O2 -DNDEBUG -DQT_NO_DEBUG")
set(KUMIR2_CXXFLAGS_Debug "-g -O0 -Werror -Wreorder -Wreturn-type -Wno-error=unused-variable -Wno-error=unused-parameter")

# Linkage flags
if(KUMIR2_ROOT)
    set(KUMIR2_LIBRARY_LINKER_FLAGS "-L${KUMIR2_ROOT}/${KUMIR2_LIBS_DIR} -Wl,--no-undefined -Wl,--enable-new-dtags")
    set(KUMIR2_PLUGIN_LINKER_FLAGS "-L${KUMIR2_ROOT}/${KUMIR2_LIBS_DIR} -Wl,--no-undefined -Wl,--enable-new-dtags -Wl,-rpath,'${KUMIR2_ROOT}/${KUMIR2_LIBS_DIR}'")
    set(KUMIR2_LAUNCHER_LINKER_FLAGS "-L${KUMIR2_ROOT}/${KUMIR2_LIBS_DIR} -Wl,--enable-new-dtags -Wl,-rpath,'${KUMIR2_ROOT}/${KUMIR2_LIBS_DIR}'")
    set(KUMIR2_TOOL_LINKER_FLAGS "-L${KUMIR2_ROOT}/${KUMIR2_LIBS_DIR} -Wl,--enable-new-dtags")
else()
    set(KUMIR2_LIBRARY_LINKER_FLAGS "-Wl,--no-undefined -Wl,--enable-new-dtags -Wl,-rpath,'\$ORIGIN'")
    set(KUMIR2_PLUGIN_LINKER_FLAGS "-Wl,--no-undefined -Wl,--enable-new-dtags -Wl,-rpath,'\$ORIGIN/..'")
    set(KUMIR2_LAUNCHER_LINKER_FLAGS "-Wl,--enable-new-dtags -Wl,-rpath,'\$ORIGIN/../${KUMIR2_LIBS_DIR}'")
    set(KUMIR2_TOOL_LINKER_FLAGS "-Wl,--enable-new-dtags -Wl,-rpath,'\$ORIGIN/../${KUMIR2_LIBS_DIR}'")
endif()

# CLang-specific compatibility options
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(KUMIR2_CXXFLAGS "${KUMIR2_CXXFLAGS} -stdlib=libstdc++")
    set(KUMIR2_LIBRARY_LINKER_FLAGS "${KUMIR2_LIBRARY_LINKER_FLAGS} -lstdc++ -lm")
    set(KUMIR2_PLUGIN_LINKER_FLAGS "${KUMIR2_PLUGIN_LINKER_FLAGS}-lstdc++ -lm")
    set(KUMIR2_LANUCHER_LINKER_FLAGS "${KUMIR2_LANUCHER_LINKER_FLAGS} -lstdc++ -lm")
    set(KUMIR2_TOOL_LINKER_FLAGS "${KUMIR2_TOOL_LINKER_FLAGS} -lstdc++ -lm")
endif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
