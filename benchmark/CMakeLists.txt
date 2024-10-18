include_directories(../include)
if (NOT MSVC)
    add_definitions(-Wno-narrowing)
    add_definitions(-Wno-old-style-cast)
endif()
#if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
#    add_definitions(-Wno-old-style-cast)
#endif()

add_executable(spanbench spanbench.cpp)
target_link_libraries(spanbench bz2_connector)
add_executable(rowsbench rowsbench.cpp)
target_link_libraries(rowsbench bz2_connector)
add_executable(castbench castbench.cpp)
target_link_libraries(castbench bz2_connector)
add_executable(validatebench validatebench.cpp)
target_link_libraries(validatebench bz2_connector)

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
