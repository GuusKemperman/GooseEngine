add_module("test_scheduling" 
	"test_scheduling.ixx")

target_link_libraries("test_scheduling" PRIVATE "test_core")
target_link_libraries("test_scheduling" PRIVATE "scheduling")
target_link_libraries("test_scheduling" PRIVATE "runtime_reflection")
target_link_libraries("test_scheduling" PRIVATE "io")
