add_subdirectory(external/cppp-reiconv-v2.1.0)
add_subdirectory(external/simdutf)

include_directories(../../include ../../include/csv_co/external ${CMAKE_BINARY_DIR}/include include external/multiprecision/include)

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
endif()

find_package(Threads REQUIRED)

if (IO_CSVKIT_UTILS)

    if (SOCI_HAVE_SQLITE3)
        message("YOU WILL HAVE SQLITE3 BACKEND FOR YOUR I/O UTILS.")
    else()
        set(SQLITE3_INCLUDE_DIR "")
    endif()
    if (SOCI_HAVE_POSTGRESQL)
        message("YOU WILL HAVE POSTGRESQL BACKEND FOR YOUR I/O UTILS.")
    else()
        set(POSTGRESQL_INCLUDE_DIR "")
    endif()
    if (SOCI_HAVE_MYSQL)
        message("YOU WILL HAVE MYSQL BACKEND FOR YOUR I/O UTILS.")
    else()
        set(MYSQL_INCLUDE_DIR "")
    endif()
    if (SOCI_HAVE_FIREBIRD)
        message("YOU WILL HAVE FIREBIRD BACKEND FOR YOUR I/O UTILS.")
    else()
        set(FIREBIRD_INCLUDE_DIR "")
    endif()
    if (SOCI_HAVE_ORACLE)
        message("YOU WILL HAVE ORACLE BACKEND FOR YOUR I/O UTILS.")
    else()
        set(ORACLE_INCLUDE_DIR "")
    endif()

    add_executable(csvsql csvsql.cpp)
    target_include_directories(csvsql
        PRIVATE
        ${CMAKE_BINARY_DIR}/external/soci/include ${SQLITE3_INCLUDE_DIR} ${POSTGRESQL_INCLUDE_DIR} ${MYSQL_INCLUDE_DIR} ${FIREBIRD_INCLUDE_DIR} ${ORACLE_INCLUDE_DIR}
    )
    target_link_libraries(csvsql PRIVATE libcppp-reiconv.static simdutf -lpthread soci_core)
    if (UNIX)
    else()
        if (MSVC)
            add_custom_command(
                    TARGET csvsql POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/soci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/soci_core_4_1.dll)
        else()
            add_custom_command(
                    TARGET csvsql POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/libsoci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/libsoci_core_4_1.dll)
        endif()
    endif()

    add_executable(sql2csv sql2csv.cpp)
    target_include_directories(sql2csv
        PRIVATE
        ${CMAKE_BINARY_DIR}/external/soci/include ${SQLITE3_INCLUDE_DIR} ${POSTGRESQL_INCLUDE_DIR} ${MYSQL_INCLUDE_DIR} ${FIREBIRD_INCLUDE_DIR} ${ORACLE_INCLUDE_DIR}
    )
    target_link_libraries(sql2csv PRIVATE libcppp-reiconv.static simdutf -lpthread soci_core)
    if (UNIX)
    else()
        if (MSVC)
            add_custom_command(
                    TARGET sql2csv POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/soci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/soci_core_4_1.dll)
        else()
            add_custom_command(
                    TARGET sql2csv POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/libsoci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/libsoci_core_4_1.dll)
        endif()
    endif()

endif()

add_subdirectory(test)
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

