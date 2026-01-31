cmake_minimum_required(VERSION 3.16)

project(Reconhecimento_Facial VERSION 0.1 LANGUAGES CXX)


set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools Sql Multimedia Network)
set(OpenCV_DIR "C:/opencv/build")
find_package(OpenCV REQUIRED)
set(TS_FILES Reconhecimento_Facial_pt_PT.ts)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui


    ${TS_FILES}
)

qt_add_executable(Reconhecimento_Facial
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    database.h
    database.cpp

    camerathread.h
    camerathread.cpp
    necessario.qrc
    capturastep.h capturastep.cpp
    reconhecimentostep.h reconhecimentostep.cpp
    circularprogress.h circularprogress.cpp
    facelandmarks.h
    facelandmarks_dummy.h
    facelandmarks_tflite.h facelandmarks_tflite.cpp




)

qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

target_link_libraries(Reconhecimento_Facial
    PRIVATE
        Qt6::Widgets
        Qt6::Sql
        Qt6::Multimedia
        Qt6::Network

        opencv_core
        opencv_imgproc
        opencv_highgui
        opencv_videoio
        opencv_dnn


)

set_target_properties(Reconhecimento_Facial PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Copiar o cascade para a pasta de build
file(COPY ${CMAKE_SOURCE_DIR}/resources/haarcascade_frontalface_default.xml
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

qt_finalize_executable(Reconhecimento_Facial)
