# NOTE: You should use MSVC 2010+ compiler

# Build filesystem layout, similar to Linux layout
set(KUMIR2_EXEC_DIR "bin")                                  # executable binaries
set(KUMIR2_LIBS_DIR "bin")                                  # common DLL's
set(KUMIR2_PLUGINS_DIR "lib/kumir2/plugins")                # libraries to be load at run time
set(KUMIR2_RESOURCES_DIR "share/kumir2")                    # non-executable resources
set(KUMIR2_XDG_APPLICATIONS_DIR "share/applications")       # X applications catalog for kumir2-open
set(KUMIR2_LIBEXECS_DIR "bin")                              # executable supplementary binaries

if(NOT DEFINED KUMIR2_SDK_SCRIPTS_DIR)
    set(KUMIR2_SDK_SCRIPTS_DIR "include")          # devel scripts
endif(NOT DEFINED KUMIR2_SDK_SCRIPTS_DIR)

if(NOT DEFINED KUMIR2_SDK_CMAKE_DIR)
    set(KUMIR2_SDK_CMAKE_DIR "include")            # cmake files to use from others
endif(NOT DEFINED KUMIR2_SDK_CMAKE_DIR)

if(NOT DEFINED KUMIR2_SDK_SRC_DIR)
    set(KUMIR2_SDK_SRC_DIR "src")
endif(NOT DEFINED KUMIR2_SDK_SRC_DIR)  # sources required to build some components (launchers, etc.)

if (MSVC)
# Compile flags
set(KUMIR2_CXXFLAGS " -Zm300 -EHsc -GR -Y- -DWIN32 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -wd4251 -utf-8")
set(KUMIR2_CXXFLAGS_Release "-DNDEBUG -DQT_NO_DEBUG -MD")
set(KUMIR2_CXXFLAGS_Debug "-MDd -Zi")

if(${USE_QT} GREATER 4)
    # Qt5 uses MSVC's built-in wchar_t type
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Zc:wchar_t")
else()
    # ... but Qt4 - not, so it should be explicitly disabled
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Zc:wchar_t-")
endif(${USE_QT} GREATER 4)

endif(MSVC)

if (MINGW)
	message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
	message(STATUS "CMAKE_CXX_FLAGS_RELEASE = ${CMAKE_CXX_FLAGS_RELEASE}")
	message(STATUS "CMAKE_CXX_FLAGS_DEBUG = ${CMAKE_CXX_FLAGS_DEBUG}")
	message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO = ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
	message(STATUS "CMAKE_EXE_LINKER_FLAGS = ${CMAKE_EXE_LINKER_FLAGS}")
	message(STATUS "CMAKE_DYNAMIC_LINKER_FLAGS = ${CMAKE_DYNAMIC_LINKER_FLAGS}")
	message(STATUS "CMAKE_STATIC_LINKER_FLAGS = ${CMAKE_STATIC_LINKER_FLAGS}")
	message(STATUS "CMAKE_MODULE_LINKER_FLAGS = ${CMAKE_MODULE_LINKER_FLAGS}")

	set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wno-unused-local-typedefs")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

message(STATUS "Leaving file kumir2_win32.cmake")
