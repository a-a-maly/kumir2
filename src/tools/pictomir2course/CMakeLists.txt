project(pictomir2course)
#cmake_minimum_required(VERSION 3.0)
#find_package(Kumir2 REQUIRED)

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/src/3rdparty/boost-1.54.0)
	find_package(Boost 1.54 REQUIRED)
	include_directories(${Boost_INCLUDE_DIRS})
endif()

option(USE_SYSTEM_ZLIB "Link against the system zlib instead of the bundled copy" OFF)

if(USE_SYSTEM_ZLIB)
	find_package(ZLIB REQUIRED)
	include_directories(${ZLIB_INCLUDE_DIRS})
	set(3RDPARTY_LIBS ${ZLIB_LIBRARIES})
	set(3RDPARTY_SOURCES )
else()
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zlib)
	set(3RDPARTY_LIBS )
	set(3RDPARTY_SOURCES
		3rdparty/zlib/uncompr.c
		3rdparty/zlib/inflate.c
		3rdparty/zlib/zutil.c
		3rdparty/zlib/inftrees.c
		3rdparty/zlib/inffast.c
		3rdparty/zlib/crc32.c
		3rdparty/zlib/adler32.c
	)
endif()

set(SOURCES
	parser.cpp
	generator.cpp
	options.cpp
	main.cpp
)

set(HEADERS
	pictomir_data.h
	parser.h
	generator.h
	options.h
)

if(NOT WIN32)
	#   1. -fexceptions:    Exceptions is widely used by Boost
	#   2. -std=c++03:      Due to bug in boost 1.49 (Ticket #6785)
	#   set(CXXFLAGS    "-fexceptions -std=c++03")
	set(CXXFLAGS "-DBOOST_EXCEPTION_DISABLE -Wall -Wno-deprecated-declarations -Wno-parentheses -Wno-unused-local-typedefs -Wno-deprecated-builtins -Wno-nonnull -Wno-unknown-warning-option")
endif()


kumir2_add_tool(
	NAME        pictomir2course
	SOURCES     ${SOURCES} ${3RDPARTY_SOURCES}
	LIBRARIES   ${3RDPARTY_LIBS}
	CXXFLAGS    ${CXXFLAGS}
)
