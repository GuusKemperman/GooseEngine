add_module("test_utils" 
	"test_utils.ixx")

target_link_libraries("test_utils" PRIVATE "test_core")
target_link_libraries("test_utils" PRIVATE "utils")

