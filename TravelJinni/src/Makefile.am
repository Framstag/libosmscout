bin_PROGRAMS = TravelJinni

TravelJinni_SOURCES = DatabaseTask.h \
                      DatabaseTask.cpp \
                      Configuration.h \
                      Configuration.cpp \
                      CitySearchDialog.h \
                      CitySearchDialog.cpp \
                      CityStreetSearchDialog.h \
                      CityStreetSearchDialog.cpp \
                      RouteDialog.h \
                      RouteDialog.cpp \
                      Viewer.cpp

TravelJinni_CXXFLAGS=-I. \
                      $(ILLUMINATION_CFLAGS) \
                      $(LIBOSMSCOUT_CFLAGS) \
                      $(LIBOSMSCOUTMAP_CFLAGS) \
                      $(LIBCAIRO_CFLAGS) \
                      $(LIBPNG_CFLAGS)

TravelJinni_LDADD=$(ILLUMINATION_LIBS) \
                  $(LIBOSMSCOUT_LIBS) \
                  $(LIBOSMSCOUTMAP_LIBS) \
                  $(LIBCAIRO_LIBS) \
                  $(LIBPNG_LIBS)

