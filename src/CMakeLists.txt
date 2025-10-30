add_library(parser STATIC parser.c)
target_include_directories(parser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Lua REQUIRED)
target_link_libraries(parser PRIVATE ${LUA_LIBRARIES})
target_include_directories(parser PRIVATE ${LUA_INCLUDE_DIR})

set(LUA_PARSER_EXTRAS "${CMAKE_SOURCE_DIR}/lua_scripts/parser_extras.lua")

file(READ "${LUA_PARSER_EXTRAS}" LUA_PARSER_EXTRAS_SCRIPT)
string(REPLACE "\\" "\\\\" LUA_PARSER_EXTRAS_SCRIPT "${LUA_PARSER_EXTRAS_SCRIPT}")
string(REPLACE "\"" "\\\"" LUA_PARSER_EXTRAS_SCRIPT "${LUA_PARSER_EXTRAS_SCRIPT}")
string(REPLACE "\n" "\\n" LUA_PARSER_EXTRAS_SCRIPT "${LUA_PARSER_EXTRAS_SCRIPT}")

add_compile_definitions(PARSER_EXTRAS_LUA="${LUA_PARSER_EXTRAS_SCRIPT}")
configure_file(${LUA_PARSER_EXTRAS} ${CMAKE_CURRENT_BINARY_DIR}/_parser_extras.lua COPYONLY)
