set(GUARD_PAGE_RUNTIME_DIR "${X64DBG_TEST_OUTPUT_DIR}/guard_page")

function(x64dbg_set_output_dir target dir)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${dir}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${dir}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${dir}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${dir}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${dir}"
        LIBRARY_OUTPUT_DIRECTORY "${dir}"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${dir}"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${dir}"
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${dir}"
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${dir}"
    )
endfunction()

add_executable(test_guard_page_target target.cpp)
target_compile_definitions(test_guard_page_target PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_features(test_guard_page_target PRIVATE cxx_std_17)
set_target_properties(test_guard_page_target PROPERTIES OUTPUT_NAME guard_page)
x64dbg_set_output_dir(test_guard_page_target "${X64DBG_TEST_OUTPUT_DIR}")

add_library(test_guard_page_plugin SHARED plugin.cpp)
target_include_directories(test_guard_page_plugin PRIVATE
    "${PROJECT_SOURCE_DIR}/src/dbg"
    "${PROJECT_SOURCE_DIR}/src/bridge"
)
target_link_libraries(test_guard_page_plugin PRIVATE
    bridge
    dbg
)
target_compile_definitions(test_guard_page_plugin PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_features(test_guard_page_plugin PRIVATE cxx_std_17)
set_target_properties(test_guard_page_plugin PROPERTIES
    PREFIX ""
    SUFFIX "${X64DBG_TEST_PLUGIN_SUFFIX}"
    OUTPUT_NAME "GuardPageMonitor"
)
x64dbg_set_output_dir(test_guard_page_plugin "${GUARD_PAGE_RUNTIME_DIR}")

add_custom_command(TARGET test_guard_page_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GUARD_PAGE_RUNTIME_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/test.txt"
        "${GUARD_PAGE_RUNTIME_DIR}/test.txt"
)

add_dependencies(x64dbg_tests test_guard_page_target test_guard_page_plugin)
