cmake_minimum_required(VERSION 3.20)
project(DocFlowKit)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Pdf PdfWidgets)

add_executable(DocFlowKit
    src/main.cpp
    src/ui/MainWindow.cpp
    src/infrastructure/pdf/PdfService.cpp
)

target_include_directories(DocFlowKit PRIVATE
    src
    src/ui
    src/infrastructure/pdf
)

target_link_libraries(DocFlowKit PRIVATE
    Qt6::Widgets
    Qt6::Pdf
    Qt6::PdfWidgets
)