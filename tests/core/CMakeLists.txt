add_executable(test_runner "test_runner_main.cpp")
add_dependencies(test_runner all_modules_target)
target_link_libraries(test_runner core_static_lib)

add_test(
  NAME test_runner
  COMMAND $<TARGET_FILE:test_runner>
  )