QT += core gui widgets sql

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    connection.cpp \
    livraison.cpp \
    livraisonswidget.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    connection.h \
    livraison.h \
    livraisonswidget.h \
    mainwindow.h

FORMS += \
    livraisonswidget.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target