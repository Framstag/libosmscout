programs = libosmscout \
           libosmscout-import \
           libosmscout-map \
           libosmscout-map-svg \
           libosmscout-map-qt \
           Import \
           Demos \
           TravelJinni \
           OSMScout \
           Tests

all:
	@for x in $(programs); do \
	  if [ -d $$x ]; then \
	    (cd $$x && $(MAKE) -j3); \
	  fi \
	done

full:
	@for x in $(programs); do \
	  if [ -d $$x ]; then \
	    echo Building $$x...; \
	    (cd $$x && ./autogen.sh && ./configure && $(MAKE)); \
	  fi \
	done

clean:
	@for x in $(programs); do\
	  if [ -d $$x ]; then \
	    (cd $$x && $(MAKE) clean); \
	  fi \
	done

dist:
	@for x in $(programs); do\
	  if [ -d $$x ]; then \
	    (cd $$x && $(MAKE) dist); \
	  fi \
	done

distclean:
	@for x in $(programs); do\
	  if [ -d $$x ]; then \
	    (cd $$x && $(MAKE) distclean); \
	  fi \
	done
