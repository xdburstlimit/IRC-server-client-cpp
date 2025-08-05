cmake_minimum_required(VERSION 3.10.0)
project(IRC VERSION 0.1.0 LANGUAGES C CXX)

file(GLOB CLIENT_SRC_FILES src/client/*.cpp thirdparty/imgui/*.cpp)
file(GLOB SERVER_SRC_FILES src/server/*.cpp thirdparty/imgui/*.cpp)

include_directories(thirdparty/imgui)

add_subdirectory(src/server)
add_subdirectory(src/client)

