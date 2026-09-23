cmake_minimum_required(VERSION 3.25)

project(morse-keyboard
	VERSION 1.3.1
	LANGUAGES CXX
)

configure_file(
    ${CMAKE_SOURCE_DIR}/version.h.in
    ${CMAKE_BINARY_DIR}/version.h
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(yaml-cpp REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)

add_executable(morse-keyboard main.cpp)

target_include_directories(morse-keyboard PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(morse-keyboard PRIVATE yaml-cpp::yaml-cpp)
target_link_libraries(morse-keyboard PRIVATE PkgConfig::LIBEVDEV)
