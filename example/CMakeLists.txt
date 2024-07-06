include_directories(../include ../utils/csvkit/external/multiprecision/include)

if (MSVC)
else()
    add_definitions(-Wno-old-style-cast)
    add_definitions(-Wno-narrowing)
    # boost::multiprecision
    add_definitions(-D__STRICT_ANSI__)
endif()


add_executable(csv_2_matrix csv_2_matrix.cpp)
add_executable(csv_2_Ram csv_2_Ram.cpp)
add_executable(csv_sum csv_sum.cpp)
add_executable(csv_sum_if csv_sum_if.cpp)

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
