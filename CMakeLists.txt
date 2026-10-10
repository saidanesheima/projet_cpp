cmake_minimum_required(VERSION 3.16)
project(GestionVentes VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Prefer Qt 6; also allow a Qt 5.15 desktop kit.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(GestionVentes
    main.cpp
    mainwindow.h mainwindow.cpp
    salesstore.h salesstore.cpp
    barchartwidget.h barchartwidget.cpp
    pdfexporter.h pdfexporter.cpp
    resources.qrc
)
target_link_libraries(GestionVentes PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
set_target_properties(GestionVentes PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
if(MSVC)
    target_compile_options(GestionVentes PRIVATE /W4 /utf-8)
else()
    target_compile_options(GestionVentes PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Optional model tests; no Qt Test or Qt Charts dependency is needed.
option(SALES_BUILD_TESTS "Build the in-memory order/inventory tests" OFF)
if(SALES_BUILD_TESTS)
    enable_testing()
    add_executable(salesstore_tests tests/salesstore_tests.cpp salesstore.cpp)
    target_include_directories(salesstore_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(salesstore_tests PRIVATE Qt${QT_VERSION_MAJOR}::Core)
    if(MSVC)
        target_compile_options(salesstore_tests PRIVATE /utf-8)
    endif()
    add_test(NAME salesstore_tests COMMAND salesstore_tests)
endif()
