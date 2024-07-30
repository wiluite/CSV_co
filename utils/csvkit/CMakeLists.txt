add_subdirectory(external/cppp-reiconv-v2.1.0)
add_subdirectory(external/simdutf)

include_directories(../../include ../../include/csv_co/external ${CMAKE_BINARY_DIR}/include include external/multiprecision/include external/soci/include)

add_definitions( -DONLY_C_LOCALE=1 )
if (MSVC)
    add_definitions(/wd4626)
    add_definitions(/wd4820)
    add_definitions(/wd4625)
    add_definitions(/wd5026)
    add_definitions(/wd4244)
    add_definitions(/wd4365)

else()
    add_definitions(-Wno-old-style-cast)
    add_definitions(-Wno-narrowing)
    add_definitions(-Wno-parentheses-equality)
endif()

find_package(Threads REQUIRED)

if (IO_CSVKIT_UTILS)

add_executable(csvsql csvsql.cpp)

target_include_directories(csvsql
        PRIVATE
        ${CMAKE_BINARY_DIR}/external/soci/include ${POSTGRESQL_INCLUDE_DIR} ${SQLITE3_INCLUDE_DIR} ${MYSQL_INCLUDE_DIR} ${FIREBIRD_INCLUDE_DIR})

target_link_libraries(csvsql PRIVATE libcppp-reiconv.static simdutf
        soci_core
)
add_custom_command(
        TARGET csvsql POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/run_csvsql
        ${CMAKE_CURRENT_BINARY_DIR}/run_csvsql)

add_executable(sql2csv sql2csv.cpp)

target_include_directories(sql2csv
        PRIVATE
        ${CMAKE_BINARY_DIR}/external/soci/include ${POSTGRESQL_INCLUDE_DIR} ${SQLITE3_INCLUDE_DIR} ${MYSQL_INCLUDE_DIR} ${FIREBIRD_INCLUDE_DIR})

target_link_libraries(sql2csv PRIVATE libcppp-reiconv.static simdutf
        soci_core
)
add_custom_command(
        TARGET sql2csv POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/run_sql2csv
        ${CMAKE_CURRENT_BINARY_DIR}/run_sql2csv)


endif()

add_executable(csvclean csvclean.cpp)
target_link_libraries(csvclean libcppp-reiconv.static simdutf)

add_executable(csvcut csvcut.cpp)
target_link_libraries(csvcut libcppp-reiconv.static simdutf)

add_executable(csvlook csvlook.cpp)
target_link_libraries(csvlook libcppp-reiconv.static simdutf -lpthread)

add_executable(csvstat csvstat.cpp)
target_link_libraries(csvstat libcppp-reiconv.static simdutf -lpthread)

add_executable(csvsort csvsort.cpp)
target_link_libraries(csvsort libcppp-reiconv.static simdutf -lpthread)

add_executable(csvjson csvjson.cpp)
target_link_libraries(csvjson libcppp-reiconv.static simdutf -lpthread)

add_executable(csvjoin csvjoin.cpp)
target_link_libraries(csvjoin libcppp-reiconv.static simdutf -lpthread)

add_executable(csvstack csvstack.cpp)
target_link_libraries(csvstack libcppp-reiconv.static simdutf)

add_executable(csvgrep csvgrep.cpp)
target_link_libraries(csvgrep libcppp-reiconv.static simdutf)

