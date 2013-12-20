TEMPLATE = app

CONFIG += qt warn_on debug link_pkgconfig thread c++11 silent

QT += core gui widgets

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
          src/DBThread.cpp \
          src/MapWidget.cpp \
          src/MainWindow.cpp \
          src/RoutingDialog.cpp \
          src/SearchLocationDialog.cpp \
          src/SettingsDialog.cpp

HEADERS = src/Settings.h \
          src/DBThread.h \
          src/MapWidget.h \
          src/MainWindow.h \
          src/RoutingDialog.h \
          src/SearchLocationDialog.h \
          src/SettingsDialog.h

