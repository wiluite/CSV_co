include_directories(../include)

if (MSVC)
else()
    add_definitions(-Wno-old-style-cast)
    add_definitions(-Wno-narrowing)
    # boost::multiprecision
    add_definitions(-D__STRICT_ANSI__)
endif()


add_executable(csv_2_matrix csv_2_matrix.cpp)
target_link_libraries(csv_2_matrix bz2_connector)
add_executable(csv_2_Ram csv_2_Ram.cpp)
target_link_libraries(csv_2_Ram bz2_connector)
add_executable(csv_sum csv_sum.cpp)
target_link_libraries(csv_sum bz2_connector)
add_executable(csv_sum_if csv_sum_if.cpp)
target_link_libraries(csv_sum_if bz2_connector)

add_custom_command(
        TARGET csv_2_matrix POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/example/smallpop.csv
        ${CMAKE_CURRENT_BINARY_DIR}/smallpop.csv)

add_custom_command(
        TARGET csv_2_Ram POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/example/smallpop.csv
        ${CMAKE_CURRENT_BINARY_DIR}/smallpop.csv)

add_custom_command(
        TARGET csv_sum POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/example/smallpop.csv
        ${CMAKE_CURRENT_BINARY_DIR}/smallpop.csv)

add_custom_command(
        TARGET csv_sum_if POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/example/uspop.csv
        ${CMAKE_CURRENT_BINARY_DIR}/uspop.csv)
