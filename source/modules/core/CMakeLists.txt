add_module("logger" "logger.ixx")
target_link_libraries("logger" "modules")

add_module("error_handling" "error_handling.ixx")
target_link_libraries("error_handling" "logger")