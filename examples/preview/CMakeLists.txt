cmake_minimum_required(VERSION 3.20)
if(CMAKE_HOST_APPLE AND NOT CMAKE_SYSTEM_NAME AND NOT CMAKE_TOOLCHAIN_FILE
        AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET AND "$ENV{MACOSX_DEPLOYMENT_TARGET}" STREQUAL "")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS deployment target")
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(example_qrcode VERSION 0.1.0 LANGUAGES NONE)
    include("${CMAKE_CURRENT_SOURCE_DIR}/HuxerUIProject.cmake")
    if(HUXERUI_PROJECT_PLAN_ONLY)
        return()
    endif()
    set(HUXERUI_QRCODE_BUILD_VIEW ON CACHE BOOL "" FORCE)
    set(HUXERUI_QRCODE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../.." "${CMAKE_BINARY_DIR}/lib-qrcode")
endif()

set(_preview_sources src/app.cpp src/application.cpp)
if(NOT HUXERUI_LIBRARY_GRAPH_ONLY)
    if(WIN32)
        list(APPEND _preview_sources platform/windows/main.cpp)
    elseif(APPLE AND NOT IOS)
        list(APPEND _preview_sources platform/macos/main.cpp)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        list(APPEND _preview_sources platform/linux/main.cpp)
    elseif(EMSCRIPTEN)
        list(APPEND _preview_sources platform/web/main.cpp)
    endif()
endif()

huxerui_add_app(example_qrcode
    SOURCES ${_preview_sources}
    BUNDLE_NAME "Lib-QRCode Preview"
    BUNDLE_IDENTIFIER "org.huxerui.lib.qrcode.preview")
huxerui_use_library(example_qrcode TARGET HuxerUI::QRCode)
if(NOT HUXERUI_LIBRARY_GRAPH_ONLY)
    target_compile_options(example_qrcode PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8;/W4;/permissive->")
endif()
