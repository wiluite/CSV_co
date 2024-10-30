if (MSVC)
    add_definitions(/wd4131)
else()
    remove_definitions(-Wno-old-style-cast -fcoroutines -Wno-reorder)
    add_definitions(-Wno-stringop-truncation)  
endif()

add_library(dbf STATIC src/dbf.c src/ioapi/ioapi.c)
