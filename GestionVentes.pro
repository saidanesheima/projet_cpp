QT += widgets
CONFIG += c++17
TEMPLATE = app
TARGET = GestionVentes

SOURCES += main.cpp \
           mainwindow.cpp \
           salesstore.cpp \
           barchartwidget.cpp \
           pdfexporter.cpp
HEADERS += mainwindow.h \
           salesstore.h \
           barchartwidget.h \
           pdfexporter.h
RESOURCES += resources.qrc
win32-msvc*: QMAKE_CXXFLAGS += /utf-8
