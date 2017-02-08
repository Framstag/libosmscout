TEMPLATE = app

QT_CONFIG -= no-pkg-config

CONFIG += qt warn_on debug link_pkgconfig thread c++11 silent

QT += core gui widgets qml quick

qtHaveModule(svg) {
 QT += svg
}

qtHaveModule(positioning) {
 QT += positioning
}

PKGCONFIG += libosmscout-map-qt libosmscout-client-qt

!macx {
  gcc:QMAKE_CXXFLAGS += -fopenmp
}

release: DESTDIR = release
debug:   DESTDIR = debug

OBJECTS_DIR = $$DESTDIR/
MOC_DIR = $$DESTDIR/
RCC_DIR = $$DESTDIR/
UI_DIR = $$DESTDIR/

SOURCES = src/OSMScout.cpp \
          src/Theme.cpp

HEADERS = src/Theme.h

DISTFILES += \
    qml/custom/MapButton.qml \
    qml/main.qml \
    qml/custom/LineEdit.qml \
    qml/custom/DialogActionButton.qml \
    qml/custom/LocationSearch.qml \
    qml/custom/ScrollIndicator.qml \
    qml/custom/MapDialog.qml \
    qml/AboutDialog.qml \
    qml/SearchDialog.qml \
    qml/MapDownloadDialog.qml \
    pics/DeleteText.svg \
    pics/Minus.svg \
    pics/Plus.svg \
    pics/Search.svg \
    pics/Download.svg

RESOURCES += \
    res.qrc

ANDROID_EXTRA_LIBS = ../libosmscout/src/.libs/libosmscout.so \
                     ../libosmscout-map/src/.libs/libosmscoutmap.so \
                     ../libosmscout-map-qt/src/.libs/libosmscoutmapqt.so

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
