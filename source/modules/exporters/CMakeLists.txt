add_module("exporters" 
"exporters.ixx"
"environments_exporter.ixx"
"schedule_exporter.ixx"
)


target_link_libraries("exporters" PRIVATE "export_core")
target_link_libraries("exporters" PRIVATE "core_traits")


target_link_libraries("exporters" PRIVATE "runtime_reflection")
target_link_libraries("exporters" PRIVATE "io")
target_link_libraries("exporters" PRIVATE "utils")
