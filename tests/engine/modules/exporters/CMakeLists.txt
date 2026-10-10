add_module("test_exporters" 
	"test_exporters.ixx"
	"test_exporter_helpers.ixx"
	"test_schedule_exporter.ixx"
	"test_export_schedule.ixx"
)

target_link_libraries("test_exporters" PRIVATE "test_core")
target_link_libraries("test_exporters" PRIVATE "runtime_reflection")
target_link_libraries("test_exporters" PRIVATE "io")
target_link_libraries("test_exporters" PRIVATE "exporters")
target_link_libraries("test_exporters" PRIVATE "export_core")
target_link_libraries("test_exporters" PRIVATE "core_traits")
