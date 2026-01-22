add_module("parser" "parser.ixx" "tokeniser.ixx")
target_link_libraries("parser" "utils")
target_link_libraries("parser" "error_handling")
