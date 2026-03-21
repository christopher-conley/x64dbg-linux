set(ISSUE3808_RUNTIME_DIR "${X64DBG_TEST_OUTPUT_DIR}/issue3808")

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

add_executable(test_issue3808_target target.cpp)
target_compile_definitions(test_issue3808_target PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_features(test_issue3808_target PRIVATE cxx_std_17)
set_target_properties(test_issue3808_target PROPERTIES OUTPUT_NAME issue3808)
x64dbg_set_output_dir(test_issue3808_target "${X64DBG_TEST_OUTPUT_DIR}")

add_library(test_issue3808_plugin SHARED flagrepro.cpp)
target_include_directories(test_issue3808_plugin PRIVATE
    "${PROJECT_SOURCE_DIR}/src/dbg"
    "${PROJECT_SOURCE_DIR}/src/bridge"
)
target_link_libraries(test_issue3808_plugin PRIVATE
    bridge
    dbg
)
target_compile_definitions(test_issue3808_plugin PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_features(test_issue3808_plugin PRIVATE cxx_std_17)
set_target_properties(test_issue3808_plugin PROPERTIES
    PREFIX ""
    SUFFIX "${X64DBG_TEST_PLUGIN_SUFFIX}"
    OUTPUT_NAME "FlagRepro3808"
)
x64dbg_set_output_dir(test_issue3808_plugin "${ISSUE3808_RUNTIME_DIR}")

add_custom_command(TARGET test_issue3808_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ISSUE3808_RUNTIME_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/test.txt"
        "${ISSUE3808_RUNTIME_DIR}/test.txt"
)

add_dependencies(x64dbg_tests test_issue3808_target test_issue3808_plugin)
