if (MSVC)
else()
    remove_definitions(-Wno-old-style-cast -fcoroutines -Wno-reorder)
    add_definitions(-Wno-stringop-truncation)  
endif()

add_library(dbf STATIC src/dbf.c src/ioapi/ioapi.c)

add_executable(dbftest test.cpp)
target_link_libraries(dbftest dbf)

