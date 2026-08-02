add_executable(test_runner "test_runner_main.cpp")
add_dependencies(test_runner all_modules_target)

target_link_libraries(test_runner core_lib)
target_link_libraries(test_runner test_core)
target_link_libraries(test_runner runtime_reflection)

