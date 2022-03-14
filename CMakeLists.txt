cmake_minimum_required(VERSION 3.15)
project(ThreadPool VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE src *.cpp *.hpp *.c *.h *.cc)
add_executable(ThreadPool ${src})

target_include_directories(ThreadPool PUBLIC ${INCLUDES})