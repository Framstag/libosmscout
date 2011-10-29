nobase_include_HEADERS= osmscout/Private/Config.h \
                        osmscout/Private/MapImportExport.h \
                        osmscout/MapFeatures.h \
                        osmscout/LoaderPNG.h \
                        osmscout/MapPainter.h

if HAVE_LIB_AGG
nobase_include_HEADERS += osmscout/MapPainterAgg.h
endif

if HAVE_LIB_CAIRO
nobase_include_HEADERS += osmscout/MapPainterCairo.h
endif

if HAVE_LIB_QTGUI
nobase_include_HEADERS += osmscout/MapPainterQt.h
endif
