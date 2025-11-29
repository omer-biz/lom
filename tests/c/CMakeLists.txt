find_package(Check REQUIRED)
find_package(Lua REQUIRED)

add_executable(test_effect test_effect.c)
target_link_libraries(test_effect PRIVATE core check m ${LUA_LIBRARIES})
target_include_directories(test_effect PRIVATE ${LUA_INCLUDE_DIR})
target_compile_definitions(test_effect PRIVATE LUA_SRC_DIR="${CMAKE_SOURCE_DIR}/lua")

add_test(NAME EffectTests COMMAND test_effect)
set_tests_properties(EffectTests PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
