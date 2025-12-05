cmake_minimum_required(VERSION 3.16)

project(MeasureTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(MeasureTool
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/CanvasScene.cpp
    src/CanvasScene.h
    src/CanvasView.cpp
    src/CanvasView.h
)

target_link_libraries(MeasureTool PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
)
