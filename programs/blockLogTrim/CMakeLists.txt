cmake_minimum_required(VERSION 3.13)
project(eos)

set(CMAKE_CXX_STANDARD 14)

add_executable(eos
		blockLogTrim.cpp)
