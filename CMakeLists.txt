cmake_minimum_required(VERSION 3.16)
project(Interconnect_A2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

add_executable(Interconnect_A2 ${SOURCES})
target_link_libraries(Interconnect_A2 pthread)

