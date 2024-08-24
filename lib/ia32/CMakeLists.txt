cmake_minimum_required(VERSION 3.13)

add_library(ia32 INTERFACE)

file(GLOB C_SOURCES "*.c")
target_sources(ia32 INTERFACE ${C_SOURCES})

target_link_libraries(ia32 INTERFACE c)
