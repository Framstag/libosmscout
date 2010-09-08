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
                      $(LIBILLUMINATION_CFLAGS) \
                      $(LIBOSMSCOUT_CFLAGS) \
                      $(LIBOSMSCOUTMAP_CFLAGS) \
                      $(LIBCAIRO_CFLAGS)

TravelJinni_LDADD=$(LIBILLUMINATION_LIBS) \
                  $(LIBOSMSCOUT_LIBS) \
                  $(LIBOSMSCOUTMAP_LIBS) \
                  $(LIBCAIRO_LIBS)

