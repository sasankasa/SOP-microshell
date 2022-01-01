cmake_minimum_required(VERSION 3.21)
project(microshell C)

set(CMAKE_C_STANDARD 99)

add_executable(microshell microshell.c)
add_executable(sleep_test sleep_test.c)