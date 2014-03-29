QT       += quick opengl

TARGET = libosmscout-map-qt
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += include ../libosmscout-map/include ../libosmscout/include

macx {
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc+
    CONFIG +=c++11
}

SOURCES += \
    src/osmscout/MapPainterQt.cpp

HEADERS +=

unix {
    target.path = /usr/lib
    INSTALLS += target
}



