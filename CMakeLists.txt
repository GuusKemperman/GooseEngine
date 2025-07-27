cmake_minimum_required(VERSION 3.30)

message("Version: ${CMAKE_VERSION}")

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
project(GooseEngine LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_MODULE_STD 1)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

add_compile_definitions(API=__declspec\(dllexport\))

add_compile_options(/W4 /WX)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

function(target_module_sources module_name module_files)
    target_sources(${module_name}
        PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES FILES ${module_files}
    )
endfunction()

add_custom_target(all_modules_target)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

function(add_module module_name)
    add_library(${module_name} SHARED)

    add_dependencies(all_modules_target ${module_name})

    if (${ARGC} GREATER 1)
        target_module_sources(${module_name} "${ARGN}")
    endif()

    # some code generation for getting the module-class factory
    set(generated_file "${CMAKE_CURRENT_BINARY_DIR}/generated_${module_name}.cpp")
	file(WRITE ${generated_file} 
        "import ${module_name};\n"
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
        "namespace ${module_name}\n"
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
        "		using module_t = ${module_name}::module;\n"
        "\n"
        "		static constexpr module_generated_data ret\n"
        "		{\n"
        "			[]() -> module_generated_data::factory\n"
        "			{\n"
        "				if constexpr (is_type_complete_v<${module_name}::module>)\n"
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

    target_sources(${module_name} PRIVATE ${generated_file})
endfunction()

add_subdirectory("source")
add_subdirectory("tests")
