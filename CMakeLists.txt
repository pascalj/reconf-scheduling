cmake_minimum_required(VERSION 3.10)

project(Scheduler VERSION 1.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED)


add_library(algorithms OBJECT util.cpp algorithms.cpp scheduling.cpp)
target_link_libraries(algorithms Boost::boost)
target_compile_options(algorithms PRIVATE -Wall -Wextra)

add_executable(lsl lsl.cpp)
add_executable(cluster cluster.cpp)

target_link_libraries(lsl algorithms)
target_link_libraries(cluster algorithms)
target_compile_features(algorithms PUBLIC cxx_std_17)
target_compile_features(lsl PUBLIC cxx_std_17)
target_compile_features(cluster PUBLIC cxx_std_17)
