cmake_minimum_required(VERSION 3.8)
project(md5Crack)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES_serial serial.c)
set(SOURCE_FILES_parallel parallel.c)

add_executable(${PROJECT_NAME}_serial ${SOURCE_FILES_serial})
target_link_libraries(${PROJECT_NAME}_serial crypto)

add_executable(${PROJECT_NAME}_parallel ${SOURCE_FILES_parallel})
target_link_libraries(${PROJECT_NAME}_parallel crypto m)

