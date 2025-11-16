cmake_minimum_required(VERSION 3.16)
option(CMAKE_EXPORT_COMPILE_COMMANDS "Create compile_commands.json" ON)

project(parser LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(core SHARED src/parser.c)
set_target_properties(core PROPERTIES
  PREFIX ""
  OUTPUT_NAME core
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

find_package(Lua REQUIRED)
target_link_libraries(core PRIVATE ${LUA_LIBRARIES})
target_include_directories(core PRIVATE ${LUA_INCLUDE_DIR})

install(TARGETS core
    LIBRARY DESTINATION lib/lua/${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}/parser
)

install(DIRECTORY lua/
    DESTINATION share/lua/${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}/parser
    FILES_MATCHING PATTERN "*.lua"
)
