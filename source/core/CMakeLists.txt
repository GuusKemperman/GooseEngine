
add_subdirectory("modules")
add_subdirectory("platform")

add_executable(GooseEngine "main.cpp")

set_target_properties(GooseEngine
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_sources(GooseEngine
    PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES 
        "platform/windows/windows.ixx"
        "platform/windows/windows_modules.ixx"
        "modules/modules.ixx"
        "modules/modules_manager.ixx"
        "modules/modules_module_base.ixx"
        "modules/modules_platform.ixx")

add_dependencies(GooseEngine all_modules_target)
