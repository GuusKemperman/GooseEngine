
add_subdirectory("modules")
add_subdirectory("platform")
add_subdirectory("code_gen")

add_library(stl STATIC)
target_sources(stl
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES 
        "stl.ixx")

add_library(core_lib STATIC)
target_sources(core_lib
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES 
        "modules/modules.ixx"
        "modules/modules_manager.ixx"
        "modules/modules_platform.ixx"
        "platform/windows/windows.ixx"
        "platform/windows/windows_modules.ixx"
)
target_link_libraries(core_lib PUBLIC stl)

add_executable(GooseEngine "main.cpp")
add_dependencies(GooseEngine all_modules_target)
target_link_libraries(GooseEngine PUBLIC core_lib)
set_target_properties(GooseEngine
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
