cmake_minimum_required(VERSION 3.17)
project(hw2 C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -pthread -mavx2")

add_executable(hw2 BMP_Processor_Static_Threads_AVX2.c)