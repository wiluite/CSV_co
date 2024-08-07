include_directories(../include ../include/csv_co/external ${CMAKE_BINARY_DIR}/include ../utils/csvkit/include ../utils/csvkit/external/multiprecision/include)

add_definitions( -DONLY_C_LOCALE=1 )
if (MSVC)
else()
    add_definitions(-Wno-narrowing)
    add_definitions(-Wno-old-style-cast)
endif()
 
add_executable(test test.cpp)
add_test(test test)

add_executable(encoding_test encoding_test.cpp)
target_link_libraries(encoding_test libcppp-reiconv.static simdutf)
add_custom_command(
        TARGET encoding_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/bjarne.csv
        ${CMAKE_CURRENT_BINARY_DIR}/bjarne.csv)
add_custom_command(
        TARGET encoding_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_latin1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_latin1.csv)
add_test(encoding_test encoding_test)

add_executable(csvkit_core_test csvkit_core_test.cpp)
target_link_libraries(csvkit_core_test libcppp-reiconv.static -lpthread)
add_test(csvkit_core_test csvkit_core_test)

add_executable(csvclean_bad_skip_lines_test csvclean_bad_skip_lines_test.cpp csvclean_test_funcs.h)
target_link_libraries(csvclean_bad_skip_lines_test libcppp-reiconv.static)
add_custom_command(
        TARGET csvclean_bad_skip_lines_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/bad_skip_lines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/bad_skip_lines.csv)
add_test(csvclean_bad_skip_lines_test csvclean_bad_skip_lines_test)

add_executable(csvclean_no_header_row_test csvclean_no_header_row_test.cpp csvclean_test_funcs.h)
target_link_libraries(csvclean_no_header_row_test libcppp-reiconv.static)
add_custom_command(
        TARGET csvclean_no_header_row_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row.csv)
add_test(csvclean_no_header_row_test csvclean_no_header_row_test)

add_executable(csvclean_optional_quote_characters_test csvclean_optional_quote_characters_test.cpp csvclean_test_funcs.h)
target_link_libraries(csvclean_optional_quote_characters_test libcppp-reiconv.static)
add_custom_command(
        TARGET csvclean_optional_quote_characters_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/optional_quote_characters.csv
        ${CMAKE_CURRENT_BINARY_DIR}/optional_quote_characters.csv)
add_test(csvclean_optional_quote_characters_test csvclean_optional_quote_characters_test)

add_executable(csvclean_mac_newlines_test csvclean_mac_newlines_test.cpp csvclean_test_funcs.h)
target_link_libraries(csvclean_mac_newlines_test libcppp-reiconv.static)
add_custom_command(
        TARGET csvclean_mac_newlines_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/mac_newlines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/mac_newlines.csv)
add_test(csvclean_mac_newlines_test csvclean_mac_newlines_test)

add_executable(csvclean_changes_character_encoding_test csvclean_changes_character_encoding_test.cpp csvclean_test_funcs.h)
target_link_libraries(csvclean_changes_character_encoding_test libcppp-reiconv.static)
add_custom_command(
        TARGET csvclean_changes_character_encoding_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_latin1.csv
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
add_test(csvclean_dry_run_test csvclean_dry_run_test)

add_executable(csvcut_test csvcut_test.cpp)
add_custom_command(
        TARGET csvcut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_skip_lines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_skip_lines.csv)
add_custom_command(
        TARGET csvcut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv)
add_custom_command(
        TARGET csvcut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8.csv)
add_custom_command(
        TARGET csvcut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy.csv.gz
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv.gz)
add_custom_command(
        TARGET csvcut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row.csv)
add_custom_command(
        TARGET csvcut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)

target_link_libraries(csvcut_test libcppp-reiconv.static)
add_test(csvcut_test csvcut_test)

add_executable(csvlook_test csvlook_test.cpp)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy3.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy3.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_latin1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_latin1.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/blanks.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/no_header_row3.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row3.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy4.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy4.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_precision.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_precision.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_utf8_bom.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8_bom.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv)
add_custom_command(
        TARGET csvlook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)

target_link_libraries(csvlook_test libcppp-reiconv.static simdutf -lpthread)
add_test(csvlook_test csvlook_test)

add_executable(csvstat_test csvstat_test.cpp)
add_custom_command(
        TARGET csvstat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/testxls_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/testxls_converted.csv)
add_custom_command(
        TARGET csvstat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/ks_1033_data.csv
        ${CMAKE_CURRENT_BINARY_DIR}/ks_1033_data.csv)
add_custom_command(
        TARGET csvstat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/FY09_EDU_Recipients_by_State.csv
        ${CMAKE_CURRENT_BINARY_DIR}/FY09_EDU_Recipients_by_State.csv)
add_custom_command(
        TARGET csvstat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)

target_link_libraries(csvstat_test libcppp-reiconv.static simdutf -lpthread)
add_test(csvstat_test csvstat_test)

add_executable(csvsort_test csvsort_test.cpp)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_latin1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_latin1.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/testxls_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/testxls_converted.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/blanks.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_literal_order.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_literal_order.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/sort_ints_nulls.csv
        ${CMAKE_CURRENT_BINARY_DIR}/sort_ints_nulls.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/timedelta.csv
        ${CMAKE_CURRENT_BINARY_DIR}/timedelta.csv)
add_custom_command(
        TARGET csvsort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)

target_link_libraries(csvsort_test libcppp-reiconv.static simdutf -lpthread)
add_test(csvsort_test csvsort_test)

add_executable(csvjoin_test csvjoin_test.cpp)
add_custom_command(
        TARGET csvjoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/join_a.csv
        ${CMAKE_CURRENT_BINARY_DIR}/join_a.csv)
add_custom_command(
        TARGET csvjoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/join_b.csv
        ${CMAKE_CURRENT_BINARY_DIR}/join_b.csv)
add_custom_command(
        TARGET csvjoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/blanks.csv)
add_custom_command(
        TARGET csvjoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv)
add_custom_command(
        TARGET csvjoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/join_no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/join_no_header_row.csv)
add_custom_command(
        TARGET csvjoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/sniff_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/sniff_limit.csv)
add_custom_command(
        TARGET csvjoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)
target_link_libraries(csvjoin_test libcppp-reiconv.static simdutf -lpthread)
add_test(csvjoin_test csvjoin_test)

add_executable(csvstack_test csvstack_test.cpp)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_skip_lines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_skip_lines.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy_col_shuffled.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy_col_shuffled.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy_col_shuffled_ragged.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy_col_shuffled_ragged.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy2.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy2.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/no_header_row2.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row2.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy3.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy3.csv)
add_custom_command(
        TARGET csvstack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)

target_link_libraries(csvstack_test libcppp-reiconv.static simdutf -lpthread)
add_test(csvstack_test csvstack_test)

add_executable(csvjson_test csvjson_test.cpp)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/blanks.csv)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/no_header_row.csv)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy3.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy3.csv)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_geo.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_geo.csv)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_geojson.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_geojson.csv)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/testjson_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/testjson_converted.csv)
add_custom_command(
        TARGET csvjson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)

target_link_libraries(csvjson_test libcppp-reiconv.static simdutf -lpthread)
add_test(csvjson_test csvjson_test)

add_executable(csvgrep_test csvgrep_test.cpp)
add_custom_command(
        TARGET csvgrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_skip_lines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_skip_lines.csv)
add_custom_command(
        TARGET csvgrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/dummy.csv)
target_link_libraries(csvgrep_test libcppp-reiconv.static simdutf -lpthread)
add_custom_command(
        TARGET csvgrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8.csv)
add_custom_command(
        TARGET csvgrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_utf8_bom.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_utf8_bom.csv)
add_custom_command(
        TARGET csvgrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/FY09_EDU_Recipients_by_State.csv
        ${CMAKE_CURRENT_BINARY_DIR}/FY09_EDU_Recipients_by_State.csv)
add_custom_command(
        TARGET csvgrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)


target_link_libraries(csvgrep_test libcppp-reiconv.static simdutf -lpthread)
add_test(csvgrep_test csvgrep_test)


add_custom_command(
        TARGET csvclean_dry_run_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/bad.csv
        ${CMAKE_CURRENT_BINARY_DIR}/bad.csv)
add_custom_command(
        TARGET csvclean_dry_run_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/test_field_size_limit.csv)

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
