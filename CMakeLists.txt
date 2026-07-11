cmake_minimum_required(VERSION 3.16)
project(WebcamProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Locate the packages using vcpkg
find_package(OpenCV REQUIRED)
find_package(onnxruntime CONFIG REQUIRED)

# 2. Define the executable target first
add_executable(main src/main.cpp)

# 3. Locate onnxruntime headers explicitly (port doesn't reliably
#    propagate INTERFACE_INCLUDE_DIRECTORIES in all versions)
find_path(ORT_INCLUDE_DIR onnxruntime_cxx_api.h
    PATH_SUFFIXES onnxruntime
)

if(NOT ORT_INCLUDE_DIR)
    message(FATAL_ERROR "Could not locate onnxruntime_cxx_api.h - check vcpkg install")
endif()

message(STATUS "onnxruntime include dir: ${ORT_INCLUDE_DIR}")

# 4. Include directories and link libraries
target_include_directories(main PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${ORT_INCLUDE_DIR}
)

target_link_libraries(main PRIVATE
    ${OpenCV_LIBS}
    onnxruntime::onnxruntime
)