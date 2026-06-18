project(src_directory)

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cpp)

add_library(src_lib STATIC ${SOURCE_FILES})

find_package(OpenCV REQUIRED)
target_include_directories(src_lib PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(src_lib PRIVATE ${OpenCV_LIBS})
