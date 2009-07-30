bin_PROGRAMS = TravelJinni

TravelJinni_SOURCES = MapPainter.h \
                      MapPainter.cpp \
                      DatabaseTask.h \
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

TravelJinni_CXXFLAGS=-I. $(ILLUMINATION_CFLAGS) $(LIBOSMSCOUT_CFLAGS) $(LIBCAIRO_CFLAGS)
TravelJinni_LDADD=$(ILLUMINATION_LIBS) $(LIBOSMSCOUT_LIBS) $(LIBCAIRO_LIBS)
