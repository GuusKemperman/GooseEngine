add_module("test_static_reflection" 
	"test_static_reflection.ixx"
	"test_tokeniser.ixx"
	"test_parser.ixx"
	"test_converter.ixx"
)

target_link_libraries("test_static_reflection" PRIVATE "static_reflection")
target_link_libraries("test_static_reflection" PRIVATE "runtime_reflection")
target_link_libraries("test_static_reflection" PRIVATE "logger")
