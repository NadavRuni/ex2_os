cmake_minimum_required(VERSION 3.22)
project(ex2_os C)

set(CMAKE_C_STANDARD 99)

add_executable(ex2_os
        main.c
        uthreads.c
)
