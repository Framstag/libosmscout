TEMPLATE = app

QT_CONFIG -= no-pkg-config

CONFIG += qt warn_on debug link_pkgconfig thread c++11 silent

QT += core gui widgets qml quick

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

SOURCES = \
  src/MainWindow.cpp \
  src/SettingsDialog.cpp \
  src/FileIO.cpp \
  src/Highlighter.cpp \
  src/StyleEditor.cpp

HEADERS = \
  src/MainWindow.h \
  src/SettingsDialog.h \
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
    StyleEditor.qrc
