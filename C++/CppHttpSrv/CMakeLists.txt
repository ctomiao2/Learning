cmake_minimum_required (VERSION 2.8)

project (SimpleHttpSrv)
add_definitions(-std=c++11)

include_directories("${PROJECT_SOURCE_DIR}/common")
include_directories("${PROJECT_SOURCE_DIR}/httpclient")
include_directories("${PROJECT_SOURCE_DIR}/httpserver")

add_subdirectory(common)
add_subdirectory(httpserver)
add_subdirectory(httpclient)

set(EXTRA_LIBS ${EXTRA_LIBS} Mongoose)

aux_source_directory(./httpserver DIR_SRCS)
add_executable(HttpSrv ${DIR_SRCS})

target_link_libraries(HttpSrv ${EXTRA_LIBS})

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
