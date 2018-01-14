# This file is just for updating translation files:
# lupdate translations.pro

TARGET = OSMScout2

# find translations -type f | sort | sed 's/$/ \\/'
TRANSLATIONS += translations/cs.ts \
                translations/en.ts

lupdate_only {
SOURCES =   qml/*.qml \
            qml/custom/*.qml \
            qml/custom/*.js
}

SOURCES += \
    ../libosmscout-client-qt/src/osmscout/RouteDescriptionBuilder.cpp
