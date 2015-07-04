TEMPLATE = app

CONFIG += qt warn_on debug link_pkgconfig thread c++11 silent

QT += core gui widgets qml quick

PKGCONFIG += libosmscout-map-qt

macx: {
    PKGCONFIG -= libosmscout-map-qt
    PKGCONFIG += libosmscout-map libosmscout
    QT_CONFIG -= no-pkg-config
}

gcc:QMAKE_CXXFLAGS += -fopenmp

INCLUDEPATH = src

release: DESTDIR = release
debug:   DESTDIR = debug

OBJECTS_DIR = $$DESTDIR/
MOC_DIR = $$DESTDIR/
RCC_DIR = $$DESTDIR/
UI_DIR = $$DESTDIR/

SOURCES = \
  src/Settings.cpp \
  src/DBThread.cpp \
  src/MapWidget.cpp \
  src/MainWindow.cpp \
  src/SettingsDialog.cpp \
  src/SearchLocationModel.cpp \
  src/FileIO.cpp \
  src/Highlighter.cpp \
  src/StyleEditor.cpp

HEADERS = \
  src/Settings.h \
  src/DBThread.h \
  src/MapWidget.h \
  src/MainWindow.h \
  src/SettingsDialog.h \
  src/SearchLocationModel.h \
  src/FileIO.h \
  src/Highlighter.h

OTHER_FILES += \
    qml/custom/MapButton.qml \
    qml/main.qml \
    qml/custom/LineEdit.qml \
    qml/custom/DialogActionButton.qml \
    qml/SearchGeocodeDialog.qml \
    qml/SearchLocationDialog.qml \
    qml/custom/LocationEdit.qml \
    qml/custom/ScrollIndicator.qml \
    qml/custom/MapDialog.qml \
    qml/TextEditor.qml \
    qml/AboutDialog.qml \
    qml/MapControl.qml

RESOURCES += \
    res.qrc

macx: {
    LIBS += -L$$PWD/../libosmscout-map-qt/build -llibosmscout-map-qt
    INCLUDEPATH += ../libosmscout/include ../libosmscout-map/include ../libosmscout-map-qt/include
    PRE_TARGETDEPS += $$PWD/../libosmscout-map-qt/liblibosmscout-map-qt.a
}
