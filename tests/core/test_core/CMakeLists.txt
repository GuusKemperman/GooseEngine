add_module("test_core"
	"test_core.ixx"
	)

target_link_libraries("test_core" PRIVATE "runtime_reflection")
