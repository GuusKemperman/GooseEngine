add_unit_test_module("test_runtime_reflection" 
	"test_runtime_reflection.ixx")

target_link_libraries("test_runtime_reflection" PRIVATE "runtime_reflection")
target_link_libraries("test_runtime_reflection" PRIVATE "logger")
