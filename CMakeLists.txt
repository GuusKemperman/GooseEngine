cmake_minimum_required(VERSION 3.30)

message("Version: ${CMAKE_VERSION}")

project(GooseEngine LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

add_compile_options(/Wall /WX 
/wd5045 
/wd4623
/wd4624
/wd4625
/wd4626
/wd5026
/wd5027
/wd4820
/wd4686
/wd4191
/wd5039
/wd4865
/wd4868
/wd4355
/wd4710
/wd4711
)

add_compile_options("/FI${CMAKE_SOURCE_DIR}/source/core/globals/defines.h")

# Workaround needed with C++20 + modules + intellisense:
# https://developercommunity.visualstudio.com/t/C20-modules-causes-Intellisense-to-gen/10403192?sort=active&topics=windows+10.0
#
# Visual Studio's IntelliSense reuses these flags for its design-time module scan, but
# unlike CMake's own compile rules it never appends a /Fd. Without one, cl defaults to 
# placing vc140.pdb in the root of the project. Looks like Microsoft won't be fixing this, 
# so, we have this.
# 
# CMake appends /Fd$TARGET_COMPILE_PDB after these flags, so real compiles are unaffected.
add_compile_options("/Fd${CMAKE_BINARY_DIR}/vc140.pdb")

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

add_custom_target(all_modules_target)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

function(add_module module_name)
    add_library(${module_name} SHARED)

    # Include by default since it's so widely used
    target_link_libraries("${module_name}" PRIVATE "stl")

    add_dependencies(all_modules_target ${module_name})

    if (${ARGC} GREATER 1)
        target_sources(${module_name}
        PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES FILES "${ARGN}"
        )

        set(ABS_SOURCES "")
        foreach(src IN LISTS ARGN)
            list(APPEND ABS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
        endforeach()

        set(GEN_LIB ${module_name}_generated)
        set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
        set(GEN_SRC ${GEN_DIR}/${module_name}_generated.cpp)

        add_custom_command(
            OUTPUT ${GEN_SRC}
            COMMAND $<TARGET_FILE:code_gen>
                    ${module_name}
                    ${GEN_SRC}
                    ${ABS_SOURCES}
            DEPENDS ${ABS_SOURCES} code_gen
            COMMENT "Generating ${module_name}_generated code"
        )

        add_library(${GEN_LIB} SHARED ${GEN_SRC})
        target_link_libraries(${GEN_LIB} PRIVATE "runtime_reflection")
        target_link_libraries(${GEN_LIB} PRIVATE ${module_name})

        add_dependencies(all_modules_target ${GEN_LIB})
    endif()
endfunction()

add_subdirectory("source")
add_subdirectory("tests")
