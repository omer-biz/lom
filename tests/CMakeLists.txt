find_package(Check REQUIRED)

add_executable(test_parser test_parser.c)
target_link_libraries(test_parser PRIVATE lom check m)

add_test(NAME ParserTests COMMAND test_parser)
set_tests_properties(ParserTests PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
