add_library(bz2_connector STATIC bz2_connector.cpp)
if (MSVC)
    add_definitions(/wd4101)
else()
    add_definitions(-Wno-error=unused-but-set-variable)
endif()

target_link_libraries(bz2_connector bz2_static)
