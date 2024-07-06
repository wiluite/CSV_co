include_directories(../include)
if (NOT MSVC)
    add_definitions(-Wno-narrowing)
    add_definitions(-Wno-old-style-cast)
endif()
#if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
#    add_definitions(-Wno-old-style-cast)
#endif()

add_executable(spanbench spanbench.cpp)
add_executable(rowsbench rowsbench.cpp)
add_executable(castbench castbench.cpp)
add_executable(validatebench validatebench.cpp)

add_custom_command(
        TARGET spanbench POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/benchmark/game.csv
        ${CMAKE_CURRENT_BINARY_DIR}/game.csv)

add_custom_command(
        TARGET rowsbench POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/benchmark/game.csv
        ${CMAKE_CURRENT_BINARY_DIR}/game.csv)

add_custom_command(
        TARGET validatebench POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/benchmark/game.csv
        ${CMAKE_CURRENT_BINARY_DIR}/game.csv)
