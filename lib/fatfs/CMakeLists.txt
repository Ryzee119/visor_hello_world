cmake_minimum_required(VERSION 3.13)

add_library(fatfs INTERFACE)

file(GLOB C_SOURCES "*.c")
target_sources(fatfs INTERFACE ${C_SOURCES})

target_link_libraries(fatfs INTERFACE c fatfs_config)
