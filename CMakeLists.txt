cmake_minimum_required(VERSION 3.16)
project(WebcamProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------
# OpenCV (via vcpkg)
# ---------------------------------------------------------------
find_package(OpenCV CONFIG REQUIRED)

# ---------------------------------------------------------------
# ONNX Runtime (official prebuilt release, NOT via vcpkg)
# Download from: https://github.com/microsoft/onnxruntime/releases
# Extract to a folder and point ONNXRUNTIME_ROOT at it, e.g.:
#   ~/onnxruntime-linux-x64-1.23.2/
# ---------------------------------------------------------------
if(NOT DEFINED ONNXRUNTIME_ROOT)
    set(ONNXRUNTIME_ROOT "$ENV{HOME}/onnxruntime-linux-x64-1.23.2" CACHE PATH "Path to extracted onnxruntime release")
endif()

find_path(ORT_INCLUDE_DIR onnxruntime_cxx_api.h
    PATHS "${ONNXRUNTIME_ROOT}/include"
    NO_DEFAULT_PATH
)

find_library(ORT_LIBRARY
    NAMES onnxruntime
    PATHS "${ONNXRUNTIME_ROOT}/lib"
    NO_DEFAULT_PATH
)

if(NOT ORT_INCLUDE_DIR OR NOT ORT_LIBRARY)
    message(FATAL_ERROR
        "Could not find onnxruntime at ${ONNXRUNTIME_ROOT}.\n"
        "Download the official release from https://github.com/microsoft/onnxruntime/releases\n"
        "(look for onnxruntime-linux-x64-<version>.tgz), extract it, and either:\n"
        "  - place it at ~/onnxruntime-linux-x64-1.23.2, or\n"
        "  - reconfigure with -DONNXRUNTIME_ROOT=/path/to/extracted/folder"
    )
endif()

message(STATUS "onnxruntime include dir: ${ORT_INCLUDE_DIR}")
message(STATUS "onnxruntime library: ${ORT_LIBRARY}")

# ---------------------------------------------------------------
# Target
# ---------------------------------------------------------------
add_executable(main src/main.cpp src/inferrer/Inferrer.cpp)

target_include_directories(main PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${ORT_INCLUDE_DIR}
)

target_link_libraries(main PRIVATE
    ${OpenCV_LIBS}
    ${ORT_LIBRARY}
)

# Ensure the executable can find libonnxruntime.so at runtime without
# needing LD_LIBRARY_PATH set manually every time.
set_target_properties(main PROPERTIES
    BUILD_RPATH "${ONNXRUNTIME_ROOT}/lib"
    INSTALL_RPATH "${ONNXRUNTIME_ROOT}/lib"
)