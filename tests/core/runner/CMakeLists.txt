add_executable(test_runner "test_runner_main.cpp")
add_dependencies(test_runner all_modules_target)

target_link_libraries(test_runner core_static_lib)
target_link_libraries(test_runner test_core)

set_target_properties(test_runner
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

function(add_unit_test_module module_name)
    add_module(${module_name} "${ARGN}")

    target_include_directories(${module_name} PRIVATE "${CMAKE_SOURCE_DIR}/tests/core/test_core")

    target_compile_options(${module_name} PRIVATE 
        "/FIdefines.h"
    )

    target_link_libraries(${module_name} "test_core")

    # Process each source file to find UNIT_TEST macros
    foreach(file IN LISTS ARGN)
        # Read lines containing 'UNIT_TEST' from the file
        file(STRINGS "${file}" unit_test_lines REGEX "UNIT_TEST")
        foreach(line IN LISTS unit_test_lines)
            # Extract the arguments from the UNIT_TEST macro
            if(line MATCHES "UNIT_TEST\\( *([^,]+) *, *([^,]+) *\\)")
                set(category "${CMAKE_MATCH_1}")
                set(test_name "${CMAKE_MATCH_2}")
                # Remove any surrounding whitespace
                string(STRIP "${category}" category)
                string(STRIP "${test_name}" test_name)
                # Call add_unit_test for each test found
                add_unit_test("${module_name}" "${category}" "${test_name}")
            endif()
        endforeach()
    endforeach()
endfunction()

function(add_unit_test module_name category_name test_name)
    add_test(
      NAME "${module_name}.${category_name}.${test_name}"
      COMMAND $<TARGET_FILE:test_runner> "${module_name}" "${category_name}" "${test_name}"
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
      )
endfunction()
