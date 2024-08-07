include_directories(../include ../include/csv_co/external )

add_definitions( -DONLY_C_LOCALE=1 )
if (MSVC)
else()
    add_definitions(-Wno-narrowing)
    add_definitions(-Wno-old-style-cast)
endif()
 
add_executable(test test.cpp)
add_test(test test)

add_custom_command(
        TARGET test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/game.csv
        ${CMAKE_CURRENT_BINARY_DIR}/game.csv)

add_custom_command(
        TARGET test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/example/smallpop.csv
        ${CMAKE_CURRENT_BINARY_DIR}/smallpop.csv)

add_custom_command(
        TARGET test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/game-invalid-format.csv
        ${CMAKE_CURRENT_BINARY_DIR}/game-invalid-format.csv)

add_custom_command(
        TARGET test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/utf8.csv)

add_custom_command(
        TARGET test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_utf8_bom.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8_bom.csv)

add_custom_command(
        TARGET test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy.csv.gz
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv.gz)

enable_testing ()
