cmake_minimum_required(VERSION 3.16)

project(Facegate LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(OpenCV REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
include(FetchContent)

FetchContent_Declare(
    face_mesh_cpp
    SOURCE_DIR /home/abreu/Projetos/Face_Mesh.Cpp
)

FetchContent_MakeAvailable(face_mesh_cpp)
qt_standard_project_setup()

find_package(SQLite3 REQUIRED)


qt_add_resources(Facegate_resources resources.qrc)

qt_add_executable(Facegate
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    cameraworker.cpp cameraworker.h
    facedatabase.cpp facedatabase.h
    faceembedder.cpp faceembedder.h
    facemeshworker.cpp facemeshworker.h
    facedetectionworker.cpp facedetectionworker.h
    databasemanager.cpp databasemanager.h
    attendanceservice.cpp attendanceservice.h
    apiservice.cpp apiservice.h

    resources.qrc
)
target_include_directories(Facegate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(Facegate
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        ${OpenCV_LIBS}
        SQLite::SQLite3
        CLFML::face_mesh
)

target_sources(Facegate PRIVATE ${Facegate_resources})
