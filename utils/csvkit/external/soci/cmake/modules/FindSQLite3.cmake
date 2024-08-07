###############################################################################
# CMake module to search for SQLite 3 library
#
# On success, the macro sets the following variables:
# SQLITE3_FOUND = if the library found
# SQLITE3_LIBRARY = full path to the library
# SQLITE3_LIBRARIES = full path to the library
# SQLITE3_INCLUDE_DIR = where to find the library headers
#
# Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
###############################################################################
#message( ${BINARY_DIR_FOR_SOCI})
find_path(SQLITE3_INCLUDE_DIR
  NAMES sqlite3.h
  PATH_PREFIXES sqlite sqlite3
  PATHS
  ${CMAKE_BINARY_DIR}/external_deps/src
  $ENV{LIB_DIR}/include
  $ENV{LIB_DIR}/include/sqlite
  $ENV{LIB_DIR}/include/sqlite3
  $ENV{ProgramFiles}/SQLite/*/include
  $ENV{ProgramFiles}/SQLite3/*/include
  $ENV{SystemDrive}/SQLite/*/include
  $ENV{SystemDrive}/SQLite3/*/include
  $ENV{SQLITE_ROOT}/include
  ${SQLITE_ROOT_DIR}/include
  $ENV{OSGEO4W_ROOT}/include)

set(SQLITE3_NAMES csvsql_sqlite3)
find_library(SQLITE3_LIBRARY
  NAMES ${SQLITE3_NAMES}
  PATHS
  ${CMAKE_BINARY_DIR}/external_deps
  $ENV{LIB_DIR}/lib
  $ENV{ProgramFiles}/SQLite/*/lib
  $ENV{ProgramFiles}/SQLite3/*/lib
  $ENV{SystemDrive}/SQLite/*/lib
  $ENV{SystemDrive}/SQLite3/*/lib
  $ENV{SQLITE_ROOT}/lib
  ${SQLITE_ROOT_DIR}/lib
  $ENV{OSGEO4W_ROOT}/lib)

set(SQLITE3_LIBRARIES
  ${SQLITE3_LIBRARIES}
  ${SQLITE3_LIBRARY})

#message(STATUS ${SQLITE3_LIBRARY})
# Handle the QUIETLY and REQUIRED arguments and set SQLITE3_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQLite3
  DEFAULT_MSG
  SQLITE3_LIBRARIES
  SQLITE3_INCLUDE_DIR)

if(NOT SQLITE3_FOUND)
	message(STATUS "SQLite3 not found (SQLITE3_INCLUDE_DIR=${SQLITE3_INCLUDE_DIR}, SQLITE3_LIBRARY=${SQLITE3_LIBRARY}.")
endif()

mark_as_advanced(SQLITE3_LIBRARY SQLITE3_INCLUDE_DIR SQLITE3_LIBRARIES)
