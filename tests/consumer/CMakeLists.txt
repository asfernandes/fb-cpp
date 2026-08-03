cmake_minimum_required(VERSION 3.18)

project(fb-cpp-installed-consumer LANGUAGES CXX)

find_package(fb-cpp CONFIG REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} PRIVATE fb-cpp::fb-cpp)
