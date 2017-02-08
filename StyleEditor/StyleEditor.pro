TEMPLATE = app

QT_CONFIG -= no-pkg-config

CONFIG += qt warn_on debug link_pkgconfig thread c++11 silent

QT += core gui widgets qml quick

!macx {
  PKGCONFIG += libosmscout-map-qt libosmscout-client-qt
  gcc:QMAKE_CXXFLAGS += -fopenmp
} else {
  INCLUDEPATH+=../../libosmscout/include
  INCLUDEPATH+=../../libosmscout-client-qt/include
  INCLUDEPATH+=../../libosmscout-map/include
  INCLUDEPATH+=../../libosmscout-map-qt/include

  LIBS+=-L../../libosmscout-client-qt/build -losmscout-client-qt
  LIBS+=-L../../libosmscout-map-qt/build -losmscout-map-qt
  LIBS+=-L../../libosmscout-map/src/.libs -losmscoutmap
  LIBS+=-L../../libosmscout/src/.libs -losmscout
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
    res.qrc
