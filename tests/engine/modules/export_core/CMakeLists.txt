add_module("test_export_core" 
	"test_export_core.ixx"
	"test_pack_writer.ixx"
	"test_rel_ptr.ixx"
)

target_link_libraries("test_export_core" PRIVATE "test_core")
target_link_libraries("test_export_core" PRIVATE "export_core")
