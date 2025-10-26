find_package(Check REQUIRED)

add_executable(test_parser test_parser.c)
target_link_libraries(test_parser PRIVATE parser check m)

# Use absolute path relative to the binary dir:
add_test(NAME ParserTests COMMAND test_parser)
