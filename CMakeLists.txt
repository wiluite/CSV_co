# Copyright (2022-2023) wiluite (additions)
# Copyright (c) Andreas Fertig.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16.2 FATAL_ERROR)

project(csv_co)

if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR, "You must use a 64-bit environment")
endif()

set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/config.h)

message(${CMAKE_SYSTEM_PROCESSOR})
option (_SANITY_CHECK "Build all with Clang sanitizers" OFF)
option (_STDLIB_LIBCPP "Build all with Clang STL (if Clang)" OFF)
option (_LLD_LINKER_FLAG "Build all lld linker" OFF)
option (IO_CSVKIT_UTILS "Build with csvsql, sql2csv, in2csv" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (IO_CSVKIT_UTILS) # Some C stuff (before any added C++ definitions)
configure_file(CMakeLists.txt.soci.obligate.dependency1.in external_deps/CMakeLists.txt)
file(COPY ${CMAKE_SOURCE_DIR}/utils/csvkit/external/csvsql_sqlite3/src DESTINATION "${CMAKE_BINARY_DIR}/external_deps")
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" . WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external_deps" )
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external_deps" )
endif()

#if (NOT MSVC)
#    set(CMAKE_CXX_FLAGS_DEBUG "-g")
#    set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native ")
#endif()

if ((_SANITY_CHECK) AND (UNIX) AND (NOT (CMAKE_SYSTEM_NAME STREQUAL "CYGWIN")))
    set (COMPILER_DEFINES "#define SANITY_CHECK")
    set (SANITY_COMPILE_FLAGS "-fsanitize=address,leak,undefined" "-fno-omit-frame-pointer")
#    set (SANITY_LINK_FLAGS "-fsanitize=address,leak,undefined,integer,array-bounds,nullability" )
    set (SANITY_LINK_FLAGS "-fsanitize=address,leak,undefined" )
endif()
if (_LLD_LINKER_FLAG)
    set (SANITY_LINK_FLAGS ${SANITY_LINK_FLAGS} "-fuse-ld=lld")
endif()

if ((_STDLIB_LIBCPP) AND (UNIX) AND (NOT (CMAKE_SYSTEM_NAME STREQUAL "CYGWIN")))
    set (STDLIB_COMPILE_FLAGS "-stdlib=libc++")
    set (STDLIB_LINK_FLAGS "-stdlib=libc++")
endif()


function(check_compiler COMPILER version)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "${COMPILER}")
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${version})
            message(FATAL_ERROR "${COMPILER} version must be at least ${version}!")
        endif()

        set(HAVE_COMPILER On PARENT_SCOPE)
    endif()
endfunction(check_compiler)

check_compiler("GNU" 10.2)
check_compiler("Clang" 15.0)
check_compiler("MSVC" 19.28.29914.0)


if(NOT HAVE_COMPILER)
    message(ERROR "You are using an unsupported compiler!")
endif()


if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(IS_GNU On)
    add_definitions(-DIS_GCC=true)
else()
    set(IS_GNU Off)
    add_definitions(-DIS_GCC=false)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(IS_CLANG On)
    add_definitions(-DIS_CLANG=true)
else()
    set(IS_CLANG Off)
    add_definitions(-DIS_CLANG=false)
endif()

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    if(IS_CLANG)
        # -Ofast with -fno-fast-math allows std::isnan(), std::isinf() workable in a Clang release
        #set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -fno-fast-math ")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")
        # alternatively may be like GCC variant
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native ")
    endif()
endif()

if(MSVC)
    add_definitions(-DIS_MSVC=true)
    add_definitions(/WX)
    #    add_definitions(/W4)
    add_definitions(/wd4189)
    add_definitions(/wd4100)
    add_definitions(/wd4996) # 'fopen': This function or variable may be unsafe. Consider using fopen_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS.
    add_definitions(/wd4267)
    add_definitions(/wd4505)

    # Use sane and nice C++ for MSVC.
    # This makes alternative tokens (not, and, ...) as actual keywords and
    # enables more conformant C++ in general
    add_definitions(/permissive-)
    add_definitions(/bigobj)

else()
    add_definitions(-DIS_MSVC=false)
    add_definitions(-Werror)
    add_definitions(-Wall)
    add_definitions(-Wextra)
    add_definitions(-Wno-unused-variable)
    add_definitions(-Wno-unused-parameter)
    add_definitions(-Wno-unused-value)
    add_definitions(-Wno-cpp)
    add_definitions(-Wno-unused-local-typedefs)
    add_definitions(-Wno-deprecated-declarations)

    add_definitions(${SANITY_COMPILE_FLAGS})
    add_link_options(${SANITY_LINK_FLAGS})

    if(IS_CLANG)
        add_definitions(-Wno-unused-private-field)
        add_definitions(-Wno-unneeded-internal-declaration)
        add_definitions(${STDLIB_COMPILE_FLAGS})
        add_link_options(${STDLIB_LINK_FLAGS})

    else()
        add_definitions(-fcoroutines) # required for GCC-10
    endif()
endif()

if (IO_CSVKIT_UTILS)
    add_subdirectory(utils/csvkit/external/soci)
endif()

include_directories(external/multiprecision/include)

add_subdirectory(example)
add_subdirectory(benchmark)
if (MSVC)
    add_compile_options(/utf-8)
endif()

add_subdirectory(utils/csvkit)
add_subdirectory(test)
