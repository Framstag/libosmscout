TEMPLATE = app

QT_CONFIG -= no-pkg-config

CONFIG += qt warn_on debug link_pkgconfig thread c++11 silent

QT += core gui widgets qml quick
# positioning

PKGCONFIG += libosmscout-map-qt

gcc:QMAKE_CXXFLAGS += -fopenmp

INCLUDEPATH = src

release: DESTDIR = release
debug:   DESTDIR = debug

OBJECTS_DIR = $$DESTDIR/
MOC_DIR = $$DESTDIR/
RCC_DIR = $$DESTDIR/
UI_DIR = $$DESTDIR/

SOURCES = src/OSMScout.cpp \
          src/Settings.cpp \
          src/Theme.cpp \
          src/DBThread.cpp \
          src/MapWidget.cpp \
          src/MainWindow.cpp \
          src/SearchLocationModel.cpp \
          src/RoutingModel.cpp

HEADERS = src/Settings.h \
          src/Theme.h \
          src/DBThread.h \
          src/MapWidget.h \
          src/MainWindow.h \
          src/SearchLocationModel.h \
          src/RoutingModel.h

OTHER_FILES += \
    qml/custom/MapButton.qml \
    qml/main.qml \
    qml/SearchLocationDialog.qml \
    qml/custom/LineEdit.qml \
    qml/custom/DialogActionButton.qml \
    qml/SearchGeocodeDialog.qml \
    qml/SearchLocationDialog.qml \
    qml/custom/LocationEdit.qml \
    qml/custom/ScrollIndicator.qml \
    qml/custom/MapDialog.qml \
    qml/RoutingDialog.qml \
    qml/AboutDialog.qml

RESOURCES += \
    res.qrc

ANDROID_EXTRA_LIBS = ../libosmscout/src/.libs/libosmscout.so \
                     ../libosmscout-map/src/.libs//libosmscoutmap.so \
                     ../libosmscout-map-qt/src/.libs/libosmscoutmapqt.so

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

