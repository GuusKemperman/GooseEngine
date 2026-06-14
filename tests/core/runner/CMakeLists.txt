add_executable(test_runner "test_runner_main.cpp")
add_dependencies(test_runner all_modules_target)

target_link_libraries(test_runner core_lib)
target_link_libraries(test_runner test_core)

set_target_properties(test_runner
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
