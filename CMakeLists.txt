cmake_minimum_required(VERSION 3.16)
option(CMAKE_EXPORT_COMPILE_COMMANDS "Create compile_commands.json" ON)

project(lom
  LANGUAGES C
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(tests)

# build liblom.a
add_library(lom STATIC src/parser.c src/lom.c src/inspect.c)
target_include_directories(lom PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_include_directories(lom PUBLIC ${CMAKE_SOURCE_DIR}/include)

find_package(Lua REQUIRED)
target_link_libraries(lom PRIVATE ${LUA_LIBRARIES})
target_include_directories(lom PRIVATE ${LUA_INCLUDE_DIR})


set(LUA_PARSER_EXTRAS "${CMAKE_SOURCE_DIR}/lua/parser_extras.lua")

file(READ "${LUA_PARSER_EXTRAS}" LUA_PARSER_EXTRAS_SCRIPT)
string(REPLACE "\\" "\\\\" LUA_PARSER_EXTRAS_SCRIPT "${LUA_PARSER_EXTRAS_SCRIPT}")
string(REPLACE "\"" "\\\"" LUA_PARSER_EXTRAS_SCRIPT "${LUA_PARSER_EXTRAS_SCRIPT}")
string(REPLACE "\n" "\\n" LUA_PARSER_EXTRAS_SCRIPT "${LUA_PARSER_EXTRAS_SCRIPT}")

add_compile_definitions(PARSER_EXTRAS_LUA="${LUA_PARSER_EXTRAS_SCRIPT}")
configure_file(${LUA_PARSER_EXTRAS} ${CMAKE_CURRENT_BINARY_DIR}/_parser_extras.lua COPYONLY)
