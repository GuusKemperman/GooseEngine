add_module("test_io" 
	"test_logger.ixx")

target_link_libraries("test_io" PRIVATE "test_core")
target_link_libraries("test_io" PRIVATE "io")