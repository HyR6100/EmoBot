QT += core gui widgets network

CONFIG += c++11
CONFIG += link_pkgconfig

TARGET = EmobotQtCompanion

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/companionbackend.cpp \
    src/chatinputedit.cpp \
    src/messagebubble.cpp \
    src/personadialog.cpp \
    src/mjpegviewerwidget.cpp

HEADERS += \
    src/companionbackend.h \
    src/mainwindow.h \
    src/chatinputedit.h \
    src/messagebubble.h \
    src/personadialog.h \
    src/mjpegviewerwidget.h

RESOURCES += resources.qrc

qmlcachegrind.file = $$OUT_PWD/qmlcachegrind.out

