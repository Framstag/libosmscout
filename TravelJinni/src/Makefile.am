bin_PROGRAMS = TravelJinni

TravelJinni_SOURCES = DatabaseTask.h \
                      DatabaseTask.cpp \
                      Configuration.h \
                      Configuration.cpp \
                      LocationSearchDialog.h \
                      LocationSearchDialog.cpp \
                      RouteDialog.h \
                      RouteDialog.cpp \
                      Viewer.cpp

TravelJinni_CXXFLAGS=-I. \
                      -DAPP_DATADIR=\"$(pkgdatadir)\" \
                      $(LIBILLUMINATION_CFLAGS) \
                      $(LIBOSMSCOUTMAPCAIRO_CFLAGS) \
                      $(LIBOSMSCOUTMAP_CFLAGS) \
                      $(LIBOSMSCOUT_CFLAGS)

TravelJinni_LDADD=$(LIBILLUMINATION_LIBS) \
                  $(LIBOSMSCOUTMAPCAIRO_LIBS) \
                  $(LIBOSMSCOUTMAP_LIBS) \
                  $(LIBOSMSCOUT_LIBS)

