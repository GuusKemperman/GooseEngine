add_module("test_core_traits" 
	"test_core_traits.ixx"
)

target_link_libraries("test_core_traits" PRIVATE "test_core")
target_link_libraries("test_core_traits" PRIVATE "runtime_reflection")
target_link_libraries("test_core_traits" PRIVATE "core_traits")
