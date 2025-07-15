cmake_minimum_required(VERSION 3.30)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
project(GooseEngine LANGUAGES CXX)

set(CMAKE_CXX_MODULE_STD 1)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Set up common BMI output directory
set(MODULE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bmis")
file(MAKE_DIRECTORY ${MODULE_OUTPUT_DIR})

# Find all module files
file(GLOB_RECURSE MODULE_FILES CONFIGURE_DEPENDS
    "Source/Modules/*.ixx"
)

# Function to extract base module name from file
function(extract_base_module_name file_path base_name_var)
    # Read first 5 lines to find module declaration
    file(STRINGS ${file_path} lines LIMIT_COUNT 5)
    
    foreach(line IN LISTS lines)
        if(line MATCHES "^(export[ \t]+)?module[ \t]+([^:;]+)[:;]")
            set(${base_name_var} ${CMAKE_MATCH_2} PARENT_SCOPE)
            return()
        endif()
    endforeach()
    
    message(FATAL_ERROR "Failed to find module declaration in ${file_path}")
endfunction()

# Function to parse imports from a module file
function(parse_module_imports file_path public_imports_var private_imports_var)
    file(STRINGS ${file_path} lines)
    set(public_imports)
    set(private_imports)
    
    foreach(line IN LISTS lines)
        # Skip comments and preprocessor directives
        if(line MATCHES "^[ \t]*//" OR line MATCHES "^[ \t]*#")
            continue()
        endif()
        
        # Match export import statements
        if(line MATCHES "export[ \t]+import[ \t]+([^;]+);")
            set(imported_module ${CMAKE_MATCH_1})
            # Skip relative partitions (e.g., import :partition;)
            if(NOT imported_module MATCHES "^:")
                # Extract base module name if partition
                if(imported_module MATCHES "([^:]+):")
                    set(imported_module ${CMAKE_MATCH_1})
                endif()
                list(APPEND public_imports ${imported_module})
            endif()
        
        # Match regular import statements
        elseif(line MATCHES "import[ \t]+([^;]+);")
            set(imported_module ${CMAKE_MATCH_1})
            # Skip relative partitions
            if(NOT imported_module MATCHES "^:")
                # Extract base module name if partition
                if(imported_module MATCHES "([^:]+):")
                    set(imported_module ${CMAKE_MATCH_1})
                endif()
                list(APPEND private_imports ${imported_module})
            endif()
        endif()
    endforeach()
    
    # Remove duplicates and set output variables
    if(public_imports)
        list(REMOVE_DUPLICATES public_imports)
    endif()
    if(private_imports)
        list(REMOVE_DUPLICATES private_imports)
    endif()
    
    set(${public_imports_var} ${public_imports} PARENT_SCOPE)
    set(${private_imports_var} ${private_imports} PARENT_SCOPE)
endfunction()

# Group files by base module name and collect dependencies
set(MODULE_GROUPS)
set(MODULE_DEPENDENCIES)
foreach(module_file IN LISTS MODULE_FILES)
    # Extract base module name from file content
    extract_base_module_name(${module_file} module_base)
    
    # Sanitize module name for CMake target
    string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" module_base_safe ${module_base})
    
    # Add to module group
    if(NOT DEFINED MODULE_GROUP_${module_base_safe})
        set(MODULE_GROUP_${module_base_safe} "")
        list(APPEND MODULE_GROUPS ${module_base_safe})
        
        # Initialize dependency tracking
        set(MODULE_PUBLIC_DEPS_${module_base_safe} "")
        set(MODULE_PRIVATE_DEPS_${module_base_safe} "")
    endif()
    list(APPEND MODULE_GROUP_${module_base_safe} ${module_file})
    
    # Parse imports for dependency tracking
    parse_module_imports(${module_file} public_imports private_imports)
    
    # Process public imports
    foreach(import IN LISTS public_imports)
        string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" import_safe ${import})
        list(APPEND MODULE_PUBLIC_DEPS_${module_base_safe} ${import_safe})
    endforeach()
    
    # Process private imports
    foreach(import IN LISTS private_imports)
        string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" import_safe ${import})
        list(APPEND MODULE_PRIVATE_DEPS_${module_base_safe} ${import_safe})
    endforeach()
endforeach()

# Create targets for each module group
foreach(module_base IN LISTS MODULE_GROUPS)
    # Get all files for this module
    set(module_files ${MODULE_GROUP_${module_base}})

    # Create dummy source file to force .lib generation,
    # in case nothing else is exported.
    set(dummy_file "${CMAKE_CURRENT_BINARY_DIR}/generated_${module_base}.cpp")
    file(WRITE ${dummy_file} 
        "extern \"C\" __declspec(dllexport) void dummy_export_${module_base}() {}\n"
    )

    # Create shared library target
    add_library(${module_base} SHARED ${module_files})
    
    # Add module sources with FILE_SET
    target_sources(${module_base}
        PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES FILES ${module_files}
    )

    target_sources(${module_base} PRIVATE ${dummy_file})

    # Set common BMI directory
    set_target_properties(${module_base} PROPERTIES
        CXX_MODULES_DIRECTORY "${MODULE_OUTPUT_DIR}"
    )

    # MSVC-specific settings
    if(MSVC)
        target_compile_options(${module_base} PRIVATE
            "/interface"
        )
        
        # Set standard library module search path
        get_filename_component(COMPILER_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
        get_filename_component(VC_TOOLS_DIR ${COMPILER_DIR} DIRECTORY)
        set(STD_MODULE_DIR "${VC_TOOLS_DIR}/modules")
        
        if(EXISTS "${STD_MODULE_DIR}")
            target_compile_options(${module_base} PRIVATE
                "/module:search \"${STD_MODULE_DIR}\""
            )
        endif()
        
        # DLL export handling
        target_compile_definitions(${module_base} PRIVATE 
            "${module_base}_EXPORTS"
        )
    endif()

    # Output directory settings
    set_target_properties(${module_base} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endforeach()

# Process dependencies after all targets are created
foreach(module_base IN LISTS MODULE_GROUPS)
    # Get dependencies
    set(public_deps ${MODULE_PUBLIC_DEPS_${module_base}})
    set(private_deps ${MODULE_PRIVATE_DEPS_${module_base}})
    
    # Remove duplicates
    if(public_deps)
        list(REMOVE_DUPLICATES public_deps)
    endif()
    if(private_deps)
        list(REMOVE_DUPLICATES private_deps)
    endif()
    
    # Add public dependencies
    foreach(dep IN LISTS public_deps)
        # Skip self-dependencies and non-existent targets
        if(TARGET ${dep} AND NOT dep STREQUAL module_base)
            target_link_libraries(${module_base} PUBLIC ${dep})
            message(STATUS "Linking ${module_base} PUBLIC ${dep}")
        endif()
    endforeach()
    
    # Add private dependencies (only if not already public)
    foreach(dep IN LISTS private_deps)
        if(TARGET ${dep} AND 
           NOT dep STREQUAL module_base AND 
           NOT dep IN_LIST public_deps)
            target_link_libraries(${module_base} PRIVATE ${dep})
            message(STATUS "Linking ${module_base} PRIVATE ${dep}")
        endif()
    endforeach()
endforeach()

add_executable(GooseEngine "Source/Core/main.cpp" )

target_sources(GooseEngine
    PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES 
        "Source/Core/module_loading/module_loading.ixx"
        "Source/Core/meta/meta.ixx"
        "Source/Core/meta/meta_builder.ixx"
        "Source/Core/meta/meta_type.ixx"

        "Source/Core/utils/utils.ixx"
        "Source/Core/utils/utils_dependency_injection.ixx"
        "Source/Core/utils/utils_memory.ixx"
 "Source/Core/meta/meta_reflect.ixx")
