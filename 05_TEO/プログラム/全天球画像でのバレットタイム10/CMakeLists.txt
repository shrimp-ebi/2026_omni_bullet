cmake_minimum_required(VERSION 3.10)
project(spherical_bullettime)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3")

set(SOURCE main.cpp)

add_executable(spherical_bullettime ${SOURCE})

add_subdirectory(src)

target_include_directories(spherical_bullettime PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(spherical_bullettime PRIVATE src_lib)

find_package(OpenCV REQUIRED)
target_include_directories(spherical_bullettime PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(spherical_bullettime PRIVATE ${OpenCV_LIBS})
