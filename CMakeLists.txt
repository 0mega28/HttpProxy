cmake_minimum_required(VERSION 4.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(HttpProxy VERSION 0.1)

add_compile_options(-Wall -Wextra -Werror -pedantic -Wconversion -Wsign-conversion)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

configure_file(config.h.in config.h)

add_executable(proxy src/main.cpp src/socket_raii.cpp)
target_include_directories(proxy PUBLIC
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
