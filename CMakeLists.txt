cmake_minimum_required(VERSION 3.20)
project(DocFlowKit)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(DocFlowKit
    src/main.cpp
    src/ui/MainWindow.cpp
)

target_include_directories(DocFlowKit PRIVATE
    src
    src/ui
)

target_link_libraries(DocFlowKit PRIVATE Qt6::Widgets)