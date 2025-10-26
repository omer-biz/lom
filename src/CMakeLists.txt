add_library(parser STATIC parser.c)
target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Lua REQUIRED)
target_link_libraries(parser PUBLIC ${LUA_LIBRARIES})
target_include_directories(parser PUBLIC ${LUA_INCLUDE_DIR})
