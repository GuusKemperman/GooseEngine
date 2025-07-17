cmake_minimum_required(VERSION 3.30)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
project(GooseEngine LANGUAGES CXX)

set(CMAKE_CXX_MODULE_STD 1)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Find all module files
file(GLOB_RECURSE MODULE_FILES CONFIGURE_DEPENDS
    "Source/Modules/*.ixx"
    "Source/Core/*.ixx"
)

# Function to parse imports from a module file
function(parse_module_imports file_path base_name_var public_imports_var private_imports_var)
    file(STRINGS ${file_path} lines)
    set(public_imports)
    set(private_imports)
    set(${base_name_var} "no export statement found in ${file_path}" PARENT_SCOPE)
    
    foreach(line IN LISTS lines)
        # Skip comments and preprocessor directives
        if(line MATCHES "^[ \t]*//" OR line MATCHES "^[ \t]*#")
            continue()
        endif()

        if(line MATCHES "^(export[ \t]+)?module[ \t]+([^:;]+)[:;]")
            set(${base_name_var} ${CMAKE_MATCH_2} PARENT_SCOPE)
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
    # Parse imports for dependency tracking
    parse_module_imports(${module_file} module_base public_imports private_imports)
    
    # Add to module group
    if(NOT DEFINED MODULE_GROUP_${module_base})
        set(MODULE_GROUP_${module_base} "")
        list(APPEND MODULE_GROUPS ${module_base})
        
        set(MODULE_PUBLIC_DEPS_${module_base} "")
        set(MODULE_PRIVATE_DEPS_${module_base} "")
    endif()
    list(APPEND MODULE_GROUP_${module_base} ${module_file})
    
    foreach(import IN LISTS public_imports)
        list(APPEND MODULE_PUBLIC_DEPS_${module_base} ${import})
    endforeach()
    
    foreach(import IN LISTS private_imports)
        list(APPEND MODULE_PRIVATE_DEPS_${module_base} ${import})
    endforeach()
endforeach()

# Create targets for each module group
foreach(module_base IN LISTS MODULE_GROUPS)
    # all .ixx files for this module
    set(module_files ${MODULE_GROUP_${module_base}})

    add_library(${module_base} SHARED ${module_files})
    
    target_sources(${module_base}
        PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES FILES ${module_files}
    )

    set_target_properties(${module_base} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # some code generation for getting the module-class factory
    set(generated_file "${CMAKE_CURRENT_BINARY_DIR}/generated_${module_base}.cpp")
	file(WRITE ${generated_file} 
        "import ${module_base};\n"
        "import std;\n"
        "\n"
        "namespace ge::modules\n"
        "{\n"
        "   class module_base;\n"
        "   class module_manager;\n"
        "}\n"
        "\n"
        "struct module_generated_data\n"
        "{\n"
        "	using factory = ge::modules::module_base*(*)(ge::modules::module_manager&);\n"
        "	factory m_instance_factory{};\n"
        "};\n"
        "\n"
        "namespace ${module_base}\n"
        "{\n"
        "	class module;\n"
        "}\n"
        "\n"
        "template<typename, typename = void>\n"
        "constexpr bool is_type_complete_v = false;\n"
        "\n"
        "template<typename T>\n"
        "constexpr bool is_type_complete_v<T, std::void_t<decltype(sizeof(T))>> = true;\n"
        "\n"
        "extern \"C\"\n"
        "{\n"
        "	__declspec(dllexport) module_generated_data generated_module_func()\n"
        "	{\n"
        "		using module_t = ${module_base}::module;\n"
        "\n"
        "		static constexpr module_generated_data ret\n"
        "		{\n"
        "			[]() -> module_generated_data::factory\n"
        "			{\n"
        "				if constexpr (is_type_complete_v<${module_base}::module>)\n"
        "				{\n"
        "					static_assert(std::is_constructible_v<module_t, ge::modules::module_manager&>,\n"
        "						\"Classes deriving from module must be constructible using a ge::modules::module_manager&\");\n"
        "\n"
        "					return +[](ge::modules::module_manager& mngr) -> ge::modules::module_base*\n"
        "					{\n"
        "						return new module_t(mngr);\n"
        "					};\n"
        "				}\n"
        "				else\n"
        "				{\n"
        "					return nullptr;\n"
        "				}\n"
        "			}()\n"
        "		};\n"
        "\n"
        "		return ret;\n"
        "	}\n"
        "}\n"
	)

    target_sources(${module_base} PRIVATE ${generated_file})
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

set(exec_files
    "Source/Core/platform/windows/windows.ixx"
    "Source/Core/platform/windows/windows_modules.ixx"
    "Source/Core/modules/modules.ixx"
    "Source/Core/modules/modules_manager.ixx"
    "Source/Core/modules/modules_module_base.ixx"
    "Source/Core/modules/modules_platform.ixx"
)

add_executable(GooseEngine "Source/Core/main.cpp" ${exec_files})

target_sources(GooseEngine
    PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES ${exec_files})

add_dependencies(GooseEngine ${MODULE_GROUPS})
