add_executable(test_runner "test_runner_main.cpp")
add_dependencies(test_runner all_modules_target)
target_link_libraries(test_runner core_static_lib)

set_target_properties(test_runner
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

add_test(
  NAME test_runner
  COMMAND $<TARGET_FILE:test_runner> "logger"
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )