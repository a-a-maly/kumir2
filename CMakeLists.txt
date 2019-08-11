project(Kumir2)
cmake_minimum_required(VERSION 3.0)
find_package(PythonInterp 3.2.0 REQUIRED)
message(STATUS "Cmake prefix path: ${CMAKE_PREFIX_PATH}")

# Query for version info and pass it as compile definitions
if(EXISTS "${CMAKE_SOURCE_DIR}/version_info.cmake")
    include(${CMAKE_SOURCE_DIR}/version_info.cmake)
    message(STATUS "File-provided version info: HASH=${GIT_HASH}")
    message(STATUS "File-provided version info: TIMESTAMP=${GIT_TIMESTAMP}")
    message(STATUS "File-provided version info: TAG=${GIT_TAG}")
    message(STATUS "File-provided version info: BRANCH=${GIT_BRANCH}")
elseif(PROVIDED_VERSION_INFO)
    message(STATUS "Variable-provided version info: HASH=${GIT_HASH}")
    message(STATUS "Variable-provided version info: TIMESTAMP=${GIT_TIMESTAMP}")
    message(STATUS "Variable-provided version info: TAG=${GIT_TAG}")
    message(STATUS "Variable-provided version info: BRANCH=${GIT_BRANCH}")
else()
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/query_version_info.py" "--mode=cmake_version_info_tbht" "--toplevel=${CMAKE_SOURCE_DIR}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE VERSION_DEFINITIONS
    )
    list(GET VERSION_DEFINITIONS 0 GIT_TAG)
    list(GET VERSION_DEFINITIONS 1 GIT_BRANCH)
    list(GET VERSION_DEFINITIONS 2 GIT_HASH)
    list(GET VERSION_DEFINITIONS 3 GIT_TIMESTAMP)
    message(STATUS "Queried version info: HASH=${GIT_HASH}")
    message(STATUS "Queried version info: TIMESTAMP=${GIT_TIMESTAMP}")
    message(STATUS "Queried version info: TAG=${GIT_TAG}")
    message(STATUS "Queried version info: BRANCH=${GIT_BRANCH}")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/custom_variables.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/custom_variables.cmake")
endif()

set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(KUMIR2_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
find_package(Kumir2 REQUIRED)

if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    include_directories("/usr/include/c++/v1")
endif ()
include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${CMAKE_SOURCE_DIR}/src")


add_subdirectory(src)


# Copy and create install targets for top-level resources
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${KUMIR2_RESOURCES_DIR}/userdocs")
file(GLOB_RECURSE resfiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/userdocs ${CMAKE_CURRENT_SOURCE_DIR}/userdocs/*)
foreach(res IN ITEMS ${resfiles})
    if(NOT ${res} MATCHES ^[.].*)
        get_filename_component(subdir ${res} PATH)
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${KUMIR2_RESOURCES_DIR}/userdocs/${subdir}")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/userdocs/${res}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${KUMIR2_RESOURCES_DIR}/userdocs/${subdir}")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/userdocs/${res}" DESTINATION "${KUMIR2_RESOURCES_DIR}/userdocs/${subdir}")
    endif()
endforeach(res)

# Courses in dist TODO move the code to function and reuse it
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${KUMIR2_RESOURCES_DIR}/courses")
file(GLOB_RECURSE resfiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/courses ${CMAKE_CURRENT_SOURCE_DIR}/courses/*)
foreach(res IN ITEMS ${resfiles})
    if(NOT ${res} MATCHES ^[.].*)
        get_filename_component(subdir ${res} PATH)
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${KUMIR2_RESOURCES_DIR}/courses/${subdir}")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/courses/${res}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${KUMIR2_RESOURCES_DIR}/courses/${subdir}")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/courses/${res}" DESTINATION "${KUMIR2_RESOURCES_DIR}/courses/${subdir}")
    endif()
endforeach(res)

# In-App icons TODO move the code to function and reuse it
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${KUMIR2_RESOURCES_DIR}/icons")
file(GLOB_RECURSE icons RELATIVE ${CMAKE_SOURCE_DIR}/share/kumir2/icons ${CMAKE_SOURCE_DIR}/share/kumir2/icons/*)
foreach(icon IN ITEMS ${icons})
    get_filename_component(subdir ${icon} PATH)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${KUMIR2_RESOURCES_DIR}/icons/${subdir}")
    file(COPY "${CMAKE_SOURCE_DIR}/share/kumir2/icons/${icon}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${KUMIR2_RESOURCES_DIR}/icons/${subdir}")
    install(FILES "${CMAKE_SOURCE_DIR}/${KUMIR2_RESOURCES_DIR}/icons/${icon}" DESTINATION "${KUMIR2_RESOURCES_DIR}/icons/${subdir}")
endforeach(icon)

# Development files
install(DIRECTORY include/ DESTINATION include/ FILES_MATCHING PATTERN "*.h*")
install(DIRECTORY src/kumir2-libs/ DESTINATION include/kumir2-libs/ FILES_MATCHING PATTERN "*.h*")
install(DIRECTORY src/kumir2-libs/ DESTINATION include/kumir2-libs/ FILES_MATCHING PATTERN "*.table")
install(
    FILES scripts/gen_actor_source.py
    DESTINATION ${KUMIR2_SDK_SCRIPTS_DIR}
    PERMISSIONS
        OWNER_EXECUTE OWNER_WRITE OWNER_READ
        GROUP_EXECUTE GROUP_READ
        WORLD_EXECUTE WORLD_READ
)

if(NOT DEFINED KUMIR2_SDK_SRC_DIR)
    set(KUMIR2_SDK_SRC_DIR "${KUMIR2_RESOURCES_DIR}/src")
endif(NOT DEFINED KUMIR2_SDK_SRC_DIR)  # sources required to build some components (launchers, etc.)

install(DIRECTORY cmake/ DESTINATION "${KUMIR2_SDK_CMAKE_DIR}")
install(FILES src/app/kumir2-launcher.cpp DESTINATION ${KUMIR2_SDK_SRC_DIR})

# Generate and install CMake file describing current build configuration
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/distribution_build_config.cmake" "# Installed build configuration\n")
file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/distribution_build_config.cmake"
    "set(USE_QT ${USE_QT})\n"
#    "set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE})\n"
    "set(KUMIR2_ROOT ${CMAKE_INSTALL_PREFIX})\n"
    "set(KUMIR2_SDK_SCRIPTS_DIR ${KUMIR2_SDK_SCRIPTS_DIR})\n"
    "set(KUMIR2_SDK_SRC_DIR ${KUMIR2_SDK_SRC_DIR})\n"
    "set(GIT_HASH ${GIT_HASH})\n"
    "set(GIT_TAG ${GIT_TAG})\n"
    "set(GIT_BRANCH ${GIT_BRANCH})\n"
    "set(GIT_TIMESTAMP ${GIT_TIMESTAMP})\n"
)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/distribution_build_config.cmake"
    DESTINATION "${KUMIR2_SDK_CMAKE_DIR}/kumir2"
)

# XDG mime types handling
if(NOT WIN32 AND NOT APPLE)
    set(ICONS_DIR "${CMAKE_SOURCE_DIR}/app_icons/linux/hicolor")
    message(STATUS "icons dir : ${ICONS_DIR}")
    file(GLOB_RECURSE icons RELATIVE ${ICONS_DIR} "${ICONS_DIR}/*/mimetypes/*.*")
    foreach(icon IN ITEMS ${icons})
        get_filename_component(subdir ${icon} PATH)
        install(FILES "${ICONS_DIR}/${icon}" DESTINATION "${KUMIR2_XDG_ICONS_DIR}/hicolor/${subdir}")
    endforeach(icon)
    install(FILES "${CMAKE_SOURCE_DIR}/share/mime/packages/kumir2-mimetypes.xml" DESTINATION ${KUMIR2_XDG_MIME_PACKAGES_DIR})
endif(NOT WIN32 AND NOT APPLE)
