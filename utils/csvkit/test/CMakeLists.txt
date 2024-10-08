include_directories(../include ../include/csv_co/external ${CMAKE_BINARY_DIR}/include ../utils/csvkit/include ../utils/csvkit/external/multiprecision/include)

add_definitions( -DONLY_C_LOCALE=1 )
if (MSVC)
    add_definitions(/wd4702)
else()
    add_definitions(-Wno-narrowing)
    add_definitions(-Wno-old-style-cast)
    if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
        message("CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
        add_definitions(-Wa,-mbig-obj)
    endif()
endif()

add_executable(csvkit_core_test csvkit_core_test.cpp)
target_link_libraries(csvkit_core_test libcppp-reiconv.static -lpthread)
add_test(csvkit_core_test csvkit_core_test)

add_executable(encoding_test encoding_test.cpp)
target_link_libraries(encoding_test libcppp-reiconv.static simdutf)
add_custom_command(
        TARGET encoding_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/bjarne.csv
        ${CMAKE_CURRENT_BINARY_DIR}/bjarne.csv)
add_custom_command(
        TARGET encoding_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_latin1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_latin1.csv)
add_test(encoding_test encoding_test)

add_executable(csvclean_bad_skip_lines_test csvclean_bad_skip_lines_test.cpp csvclean_test_funcs.h)
target_link_libraries(csvclean_bad_skip_lines_test libcppp-reiconv.static)
add_custom_command(
        TARGET csvclean_bad_skip_lines_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/bad_skip_lines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/bad_skip_lines.csv)
add_test(csvclean_bad_skip_lines_test csvclean_bad_skip_lines_test)

add_executable(csvclean_no_header_row_test csvclean_no_header_row_test.cpp csvclean_test_funcs.h)
target_link_libraries(csvclean_no_header_row_test libcppp-reiconv.static)
add_custom_command(
        TARGET csvclean_no_header_row_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row.csv)
add_test(csvclean_no_header_row_test csvclean_no_header_row_test)

add_executable(csvclean_optional_quote_characters_test csvclean_optional_quote_characters_test.cpp csvclean_test_funcs.h)
target_link_libraries(csvclean_optional_quote_characters_test libcppp-reiconv.static)
add_custom_command(
        TARGET csvclean_optional_quote_characters_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/optional_quote_characters.csv
        ${CMAKE_CURRENT_BINARY_DIR}/optional_quote_characters.csv)
add_test(csvclean_optional_quote_characters_test csvclean_optional_quote_characters_test)

add_executable(csvclean_mac_newlines_test csvclean_mac_newlines_test.cpp csvclean_test_funcs.h)
target_link_libraries(csvclean_mac_newlines_test libcppp-reiconv.static)
add_custom_command(
        TARGET csvclean_mac_newlines_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/mac_newlines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/mac_newlines.csv)
add_test(csvclean_mac_newlines_test csvclean_mac_newlines_test)

add_executable(csvclean_changes_character_encoding_test csvclean_changes_character_encoding_test.cpp csvclean_test_funcs.h)
target_link_libraries(csvclean_changes_character_encoding_test libcppp-reiconv.static)
add_custom_command(
        TARGET csvclean_changes_character_encoding_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_latin1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_latin1.csv)
add_test(csvclean_changes_character_encoding_test csvclean_changes_character_encoding_test)

add_executable(csvclean_removes_bom_test csvclean_removes_bom_test.cpp csvclean_test_funcs.h)
target_link_libraries(csvclean_removes_bom_test libcppp-reiconv.static)
add_custom_command(
        TARGET csvclean_removes_bom_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_utf8_bom.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8_bom.csv)
add_test(csvclean_removes_bom_test csvclean_removes_bom_test)

add_executable(csvclean_dry_run_test csvclean_dry_run_test.cpp)
target_link_libraries(csvclean_dry_run_test libcppp-reiconv.static)
add_custom_command(
        TARGET csvclean_dry_run_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/bad.csv
        ${CMAKE_CURRENT_BINARY_DIR}/bad.csv)
add_custom_command(
        TARGET csvclean_dry_run_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)

add_test(csvclean_dry_run_test csvclean_dry_run_test)

add_executable(csvcut_test csvcut_test.cpp)
add_custom_command(
        TARGET csvcut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_skip_lines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_skip_lines.csv)
add_custom_command(
        TARGET csvcut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv)
add_custom_command(
        TARGET csvcut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8.csv)
add_custom_command(
        TARGET csvcut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy.csv.gz
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv.gz)
add_custom_command(
        TARGET csvcut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row.csv)
add_custom_command(
        TARGET csvcut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)
add_custom_command(
        TARGET csvcut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/null_byte.csv
        ${CMAKE_CURRENT_BINARY_DIR}/null_byte.csv)

target_link_libraries(csvcut_test libcppp-reiconv.static)
add_test(csvcut_test csvcut_test)

add_executable(csvlook_test csvlook_test.cpp)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy3.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy3.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_latin1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_latin1.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/blanks.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/no_header_row3.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row3.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy4.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy4.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_precision.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_precision.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_utf8_bom.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8_bom.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)

target_link_libraries(csvlook_test libcppp-reiconv.static simdutf -lpthread)
add_test(csvlook_test csvlook_test)

add_executable(csvstat_test csvstat_test.cpp)
add_custom_command(
        TARGET csvstat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/testxls_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/testxls_converted.csv)
add_custom_command(
        TARGET csvstat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/ks_1033_data.csv
        ${CMAKE_CURRENT_BINARY_DIR}/ks_1033_data.csv)
add_custom_command(
        TARGET csvstat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/FY09_EDU_Recipients_by_State.csv
        ${CMAKE_CURRENT_BINARY_DIR}/FY09_EDU_Recipients_by_State.csv)
add_custom_command(
        TARGET csvstat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)
add_custom_command(
        TARGET csvstat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8.csv)
add_custom_command(
        TARGET csvstat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_latin1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_latin1.csv)


target_link_libraries(csvstat_test libcppp-reiconv.static simdutf -lpthread)
add_test(csvstat_test csvstat_test)

add_executable(csvsort_test csvsort_test.cpp)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_latin1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_latin1.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/testxls_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/testxls_converted.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/blanks.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_literal_order.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_literal_order.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/sort_ints_nulls.csv
        ${CMAKE_CURRENT_BINARY_DIR}/sort_ints_nulls.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/timedelta.csv
        ${CMAKE_CURRENT_BINARY_DIR}/timedelta.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)

target_link_libraries(csvsort_test libcppp-reiconv.static simdutf -lpthread)
add_test(csvsort_test csvsort_test)

add_executable(csvjoin_test csvjoin_test.cpp)
add_custom_command(
        TARGET csvjoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/join_a.csv
        ${CMAKE_CURRENT_BINARY_DIR}/join_a.csv)
add_custom_command(
        TARGET csvjoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/join_b.csv
        ${CMAKE_CURRENT_BINARY_DIR}/join_b.csv)
add_custom_command(
        TARGET csvjoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/blanks.csv)
add_custom_command(
        TARGET csvjoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv)
add_custom_command(
        TARGET csvjoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/join_no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/join_no_header_row.csv)
add_custom_command(
        TARGET csvjoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/sniff_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/sniff_limit.csv)
add_custom_command(
        TARGET csvjoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)
target_link_libraries(csvjoin_test libcppp-reiconv.static simdutf -lpthread)
add_test(csvjoin_test csvjoin_test)

add_executable(csvstack_test csvstack_test.cpp)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_skip_lines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_skip_lines.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy_col_shuffled.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy_col_shuffled.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy_col_shuffled_ragged.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy_col_shuffled_ragged.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy2.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy2.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/no_header_row2.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row2.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy3.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy3.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)

target_link_libraries(csvstack_test libcppp-reiconv.static simdutf -lpthread)
add_test(csvstack_test csvstack_test)

add_executable(csvjson_test csvjson_test.cpp)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/blanks.csv)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row.csv)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy3.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy3.csv)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_geo.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_geo.csv)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_geojson.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_geojson.csv)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/testjson_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/testjson_converted.csv)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)

target_link_libraries(csvjson_test libcppp-reiconv.static simdutf -lpthread)
add_test(csvjson_test csvjson_test)

add_executable(csvgrep_test csvgrep_test.cpp)
add_custom_command(
        TARGET csvgrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_skip_lines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_skip_lines.csv)
add_custom_command(
        TARGET csvgrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv)
target_link_libraries(csvgrep_test libcppp-reiconv.static simdutf -lpthread)
add_custom_command(
        TARGET csvgrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8.csv)
add_custom_command(
        TARGET csvgrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_utf8_bom.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8_bom.csv)
add_custom_command(
        TARGET csvgrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/FY09_EDU_Recipients_by_State.csv
        ${CMAKE_CURRENT_BINARY_DIR}/FY09_EDU_Recipients_by_State.csv)
add_custom_command(
        TARGET csvgrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)

target_link_libraries(csvgrep_test libcppp-reiconv.static simdutf -lpthread)
add_test(csvgrep_test csvgrep_test)


if (IO_CSVKIT_UTILS)
  if (NOT _SANITY_CHECK)
    add_executable(csvsql_test csvsql_test.cpp)
    target_include_directories(csvsql_test PRIVATE
        ${CMAKE_BINARY_DIR}/../external/soci/include
        ${SQLITE3_INCLUDE_DIR}
        ${POSTGRESQL_INCLUDE_DIR}
        ${MARIADB_INCLUDE_DIR}
        ${MYSQL_INCLUDE_DIR}
        ${FIREBIRD_INCLUDE_DIR}
        ${ORACLE_INCLUDE_DIR}
        ../external/ocilib/include
    )
    add_custom_command(
        TARGET csvsql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/testfixed_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/testfixed_converted.csv)
    add_custom_command(
        TARGET csvsql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/blanks.csv)
    add_custom_command(
        TARGET csvsql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row.csv)
    add_custom_command(
        TARGET csvsql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv)
    add_custom_command(
        TARGET csvsql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/iris.csv
        ${CMAKE_CURRENT_BINARY_DIR}/iris.csv)
    add_custom_command(
        TARGET csvsql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/irismeta.csv
        ${CMAKE_CURRENT_BINARY_DIR}/irismeta.csv)
    add_custom_command(
        TARGET csvsql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_query.sql
        ${CMAKE_CURRENT_BINARY_DIR}/test_query.sql)
    add_custom_command(
        TARGET csvsql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_query_1252.sql
        ${CMAKE_CURRENT_BINARY_DIR}/test_query_1252.sql)
    add_custom_command(
        TARGET csvsql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_query_empty.sql
        ${CMAKE_CURRENT_BINARY_DIR}/test_query_empty.sql)
    add_custom_command(
        TARGET csvsql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/foo1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/foo1.csv)
    add_custom_command(
        TARGET csvsql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/foo2.csv
        ${CMAKE_CURRENT_BINARY_DIR}/foo2.csv)
    add_custom_command(
        TARGET csvsql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/piped_stdin
        ${CMAKE_CURRENT_BINARY_DIR}/piped_stdin)

    target_link_libraries(csvsql_test libcppp-reiconv.static simdutf soci_core -lpthread ociliba)
    add_test(csvsql_test csvsql_test)

    if (UNIX)
    else()
        if (MSVC)
            add_custom_command(
                    TARGET csvsql_test POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/soci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/soci_core_4_1.dll)
            add_custom_command(
                    TARGET csvsql_test POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_BINARY_DIR}/../external/ocilib/ociliba.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/ociliba.dll)

        else()
            add_custom_command(
                    TARGET csvsql_test POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/libsoci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/libsoci_core_4_1.dll)
            add_custom_command(
                    TARGET csvsql_test POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_BINARY_DIR}/../external/ocilib/libociliba.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/libociliba.dll)
        endif()
    endif()

    add_executable(sql2csv_test sql2csv_test.cpp)
    target_include_directories(sql2csv_test PRIVATE
        ${CMAKE_BINARY_DIR}/../external/soci/include
        ${SQLITE3_INCLUDE_DIR}
        ${POSTGRESQL_INCLUDE_DIR}
        ${MARIADB_INCLUDE_DIR}
        ${MYSQL_INCLUDE_DIR}
        ${FIREBIRD_INCLUDE_DIR}
        ${ORACLE_INCLUDE_DIR}
        ../external/ocilib/include
    )
    add_custom_command(
            TARGET sql2csv_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test.sql
            ${CMAKE_CURRENT_BINARY_DIR}/test.sql)
    add_custom_command(
            TARGET sql2csv_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/test_utf8.csv
            ${CMAKE_CURRENT_BINARY_DIR}/test_utf8.csv)
    add_custom_command(
            TARGET sql2csv_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/dummy.csv
            ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv)
    add_custom_command(
            TARGET sql2csv_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/iris.csv
            ${CMAKE_CURRENT_BINARY_DIR}/iris.csv)
    add_custom_command(
            TARGET sql2csv_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/utils/csvkit/test/examples/stdin_select
            ${CMAKE_CURRENT_BINARY_DIR}/stdin_select)

    target_link_libraries(sql2csv_test libcppp-reiconv.static simdutf soci_core -lpthread ociliba)
    add_test(sql2csv_test sql2csv_test)

    if (UNIX)
    else()
        if (MSVC)
            add_custom_command(
                    TARGET sql2csv_test POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/soci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/soci_core_4_1.dll)
        else()
            add_custom_command(
                    TARGET sql2csv_test POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/libsoci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/libsoci_core_4_1.dll)
        endif()
    endif()

  endif()

add_executable(in2csv_test in2csv_test.cpp)
target_link_libraries(in2csv_test libcppp-reiconv.static simdutf -lpthread)
add_test(in2csv_test in2csv_test)

endif()

enable_testing ()
