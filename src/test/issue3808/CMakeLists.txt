cmake_minimum_required(VERSION 3.15)
project(issue3808 LANGUAGES CXX)

if(MSVC)
    add_link_options(/DEBUG:FULL /INCREMENTAL:NO /OPT:REF /OPT:ICF)
endif()

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(PLUGINSDK_DIR "${REPO_ROOT}/release/pluginsdk")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(X64DBG_ARCH x64)
    set(X64DBG_PLUGIN_SUFFIX .dp64)
else()
    set(X64DBG_ARCH x32)
    set(X64DBG_PLUGIN_SUFFIX .dp32)
endif()

set(X64DBG_DBG_LIB "")
set(X64DBG_BRIDGE_LIB "")
foreach(X64DBG_LIB_DIR
    "${REPO_ROOT}/bin/${X64DBG_ARCH}"
    "${REPO_ROOT}/bin/${X64DBG_ARCH}d"
    "${PLUGINSDK_DIR}")
    if(NOT X64DBG_DBG_LIB AND EXISTS "${X64DBG_LIB_DIR}/${X64DBG_ARCH}dbg.lib")
        set(X64DBG_DBG_LIB "${X64DBG_LIB_DIR}/${X64DBG_ARCH}dbg.lib")
    endif()
    if(NOT X64DBG_BRIDGE_LIB AND EXISTS "${X64DBG_LIB_DIR}/${X64DBG_ARCH}bridge.lib")
        set(X64DBG_BRIDGE_LIB "${X64DBG_LIB_DIR}/${X64DBG_ARCH}bridge.lib")
    endif()
endforeach()

if(NOT X64DBG_DBG_LIB OR NOT X64DBG_BRIDGE_LIB)
    message(FATAL_ERROR "Could not find x64dbg import libraries for ${X64DBG_ARCH}")
endif()

add_executable(Issue3808Target target.cpp)
set_target_properties(Issue3808Target PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

target_compile_definitions(Issue3808Target PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

add_library(FlagRepro3808 SHARED flagrepro.cpp)
target_include_directories(FlagRepro3808 PRIVATE
    "${REPO_ROOT}/src/dbg"
    "${REPO_ROOT}/src/bridge"
    "${PLUGINSDK_DIR}"
)
target_link_libraries(FlagRepro3808 PRIVATE
    "${X64DBG_DBG_LIB}"
    "${X64DBG_BRIDGE_LIB}"
)
set_target_properties(FlagRepro3808 PROPERTIES
    PREFIX ""
    SUFFIX "${X64DBG_PLUGIN_SUFFIX}"
    OUTPUT_NAME "FlagRepro3808"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)
target_compile_definitions(FlagRepro3808 PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
