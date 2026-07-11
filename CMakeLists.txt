cmake_minimum_required(VERSION 3.16)

project(WebcamProject)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)

add_executable(webcam
    src/main.cpp
)

target_include_directories(webcam PRIVATE ${OpenCV_INCLUDE_DIRS})

target_link_libraries(webcam PRIVATE
    ${OpenCV_LIBS}
)