add_unit_test_module("test_parser" 
	"test_parser.ixx")

target_link_libraries("test_parser" "static_reflection")
target_link_libraries("test_parser" "logger")
