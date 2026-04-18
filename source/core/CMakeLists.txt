
add_subdirectory("modules")
add_subdirectory("platform")

add_library(stl STATIC)
target_sources(stl
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES 
        "stl.ixx")

add_executable(GooseEngine "main.cpp")
add_dependencies(GooseEngine all_modules_target)

target_link_libraries(GooseEngine PUBLIC stl)
target_link_libraries(GooseEngine PUBLIC windows)
target_link_libraries(GooseEngine PUBLIC modules)

set_target_properties(GooseEngine
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
