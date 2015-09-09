QT       += quick opengl

TARGET = libosmscout-map-qt
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += include ../libosmscout-map/include ../libosmscout/include

SOURCES += \
    src/osmscout/MapPainterQt.cpp

HEADERS +=

macx {
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc+
    CONFIG +=c++11
}

unix {
    target.path = /usr/lib
    INSTALLS += target
}

windows {
    DEFINES += WIN32 OSMSCOUTMAPQTDLL DLL_EXPORT
    CONFIG(debug, debug|release): DESTDIR = ../windows/Debug
    CONFIG(release, debug|release): DESTDIR = ../windows/Release
}
