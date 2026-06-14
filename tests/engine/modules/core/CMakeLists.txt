add_module("test_logger" 
	"test_logger.ixx")

target_link_libraries("test_logger" PRIVATE "logger")