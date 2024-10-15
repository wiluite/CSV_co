cmake_minimum_required(VERSION 3.16)

include_directories(. include)

add_subdirectory(iconv_c_connector)

if (MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
    add_definitions(/wd4245)
    add_definitions(/wd4706)
else()
    remove_definitions(-Wno-old-style-cast -fcoroutines -Wno-reorder)
    add_definitions(-Wno-implicit-fallthrough)
endif()
add_library(xlsreader STATIC src/xlstool.c src/endian.c src/locale.c src/ole.c src/xls.c)
target_link_libraries(xlsreader iconv_c_connector)

add_executable(test_xls test.c)
target_link_libraries(test_xls xlsreader)
