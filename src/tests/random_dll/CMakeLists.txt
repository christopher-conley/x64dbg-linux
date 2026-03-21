set(RANDOM_DLL_RUNTIME_DIR "${X64DBG_TEST_OUTPUT_DIR}/random_dll")

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

add_library(test_random_helper SHARED test_dll.cpp)
target_compile_definitions(test_random_helper PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_features(test_random_helper PRIVATE cxx_std_17)
set_target_properties(test_random_helper PROPERTIES PREFIX "" OUTPUT_NAME "random")
x64dbg_set_output_dir(test_random_helper "${X64DBG_TEST_OUTPUT_DIR}")

add_executable(test_random_dll_target target.cpp)
target_compile_definitions(test_random_dll_target PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_features(test_random_dll_target PRIVATE cxx_std_17)
set_target_properties(test_random_dll_target PROPERTIES OUTPUT_NAME "random_dll")
x64dbg_set_output_dir(test_random_dll_target "${X64DBG_TEST_OUTPUT_DIR}")

add_library(test_random_dll_plugin SHARED plugin.cpp)
target_include_directories(test_random_dll_plugin PRIVATE
    "${PROJECT_SOURCE_DIR}/src/dbg"
    "${PROJECT_SOURCE_DIR}/src/bridge"
)
target_link_libraries(test_random_dll_plugin PRIVATE
    bridge
    dbg
)
target_compile_definitions(test_random_dll_plugin PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_features(test_random_dll_plugin PRIVATE cxx_std_17)
set_target_properties(test_random_dll_plugin PROPERTIES
    PREFIX ""
    SUFFIX "${X64DBG_TEST_PLUGIN_SUFFIX}"
    OUTPUT_NAME "RandomDll3599"
)
x64dbg_set_output_dir(test_random_dll_plugin "${RANDOM_DLL_RUNTIME_DIR}")

add_custom_command(TARGET test_random_dll_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RANDOM_DLL_RUNTIME_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/test.txt"
        "${RANDOM_DLL_RUNTIME_DIR}/test.txt"
)

add_dependencies(x64dbg_tests test_random_helper test_random_dll_target test_random_dll_plugin)
