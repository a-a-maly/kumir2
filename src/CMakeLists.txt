file(GLOB THIRDPARTY_PACKAGES "${CMAKE_SOURCE_DIR}/src/3rdparty/*")
foreach(PACKAGE IN ITEMS ${THIRDPARTY_PACKAGES})
    if(EXISTS "${PACKAGE}/include")
        include_directories("${PACKAGE}/include")
    endif()
    if(EXISTS "${PACKAGE}/CMakeLists.txt")
        add_subdirectory("${PACKAGE}")
    endif()
endforeach(PACKAGE)

add_subdirectory(kumir2-libs)
add_subdirectory(plugins)
add_subdirectory(actors)
add_subdirectory(app)
add_subdirectory(tools)
