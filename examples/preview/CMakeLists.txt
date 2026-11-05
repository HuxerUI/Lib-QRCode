cmake_minimum_required(VERSION 3.20)

# Like other Lib previews, this directory is also an independent application project.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(qrcode_preview VERSION 0.1.0 LANGUAGES NONE)
    if(HUXERUI_PROJECT_PLAN_OUTPUT)
        get_filename_component(_preview_plan_dir "${HUXERUI_PROJECT_PLAN_OUTPUT}" DIRECTORY)
        file(MAKE_DIRECTORY "${_preview_plan_dir}")
        file(WRITE "${HUXERUI_PROJECT_PLAN_OUTPUT}" [=[
{
  "schema": 1,
  "kind": "app",
  "name": "QRCode Preview",
  "id": "org.huxerui.qrcode.preview",
  "target": "qrcode_preview"
}
]=])
    endif()
    if(HUXERUI_PROJECT_PLAN_ONLY)
        return()
    endif()
    set(HUXERUI_QRCODE_BUILD_VIEW ON CACHE BOOL "" FORCE)
    set(HUXERUI_QRCODE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../.." "${CMAKE_BINARY_DIR}/lib-qrcode")
endif()

huxerui_add_app(qrcode_preview
    SOURCES main.cpp src/preview.cpp
    BUNDLE_NAME "HuxerUI QRCode Preview"
    BUNDLE_IDENTIFIER "org.huxerui.qrcode.preview")
huxerui_use_library(qrcode_preview TARGET HuxerUI::QRCode)
if(NOT HUXERUI_LIBRARY_GRAPH_ONLY)
    target_compile_options(qrcode_preview PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8;/W4;/permissive->")
endif()
