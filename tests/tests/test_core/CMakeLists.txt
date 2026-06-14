add_module("test_test_core" 
	"test_test_core.ixx")


target_link_libraries("test_test_core" PRIVATE "test_core")
