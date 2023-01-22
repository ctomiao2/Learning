cmake_minimum_required(VERSION 3.10)
project(BezierCurve)

find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_BUILD_TYPE "Debug")

add_executable(BezierCurve main.cpp)

target_link_libraries(BezierCurve ${OpenCV_LIBRARIES})
