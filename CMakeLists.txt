cmake_minimum_required(VERSION 3.25)

project(morse-keyboard
  LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(morse-keyboard main.cpp)
