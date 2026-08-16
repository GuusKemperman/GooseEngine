add_executable(code_gen "code_gen.cpp")

target_link_libraries(code_gen core_lib)
target_link_libraries(code_gen static_reflection)

