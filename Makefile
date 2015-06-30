#export CXX = clang++

programs = libosmscout \
           libosmscout-import \
           libosmscout-map \
           libosmscout-map-agg \
           libosmscout-map-cairo \
           libosmscout-map-opengl \
           libosmscout-map-qt \
           libosmscout-map-svg \
           DumpData \
           Demos \
           Import \
           OSMScout2 \
           StyleEditor \
           Tests

.PHONY: all full clean dist distclean \
        libosmscout \
        libosmscout-import \
        libosmscout-map \
        libosmscout-map-agg \
        libosmscout-map-cairo \
        libosmscout-map-opengl \
        libosmscout-map-qt \
        libosmscout-map-svg \
        DumpData \
        Demos \
        Import \
        OSMScout2 \
        StyleEditor \
        Tests

all: libosmscout \
     libosmscout-import \
     libosmscout-map \
     libosmscout-map-agg \
     libosmscout-map-cairo \
     libosmscout-map-opengl \
     libosmscout-map-qt \
     libosmscout-map-svg \
     DumpData \
     Demos \
     Import \
     OSMScout2 \
     StyleEditor \
     Tests

full:
	@for x in $(programs); do\
	  if [ -d $$x ]; then \
	    echo Configuring $$x...; \
	    (cd $$x && ./autogen.sh && ./configure && $(MAKE)); \
	  fi \
	done
	@$(MAKE) all


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

# libosmscout

libosmscout:
	(cd libosmscout && $(MAKE))

# libosmscout-import

libosmscout-import: libosmscout
	(cd libosmscout-import && $(MAKE))

# libosmscout-map && backends

libosmscout-map: libosmscout
	(cd libosmscout-map && $(MAKE))

libosmscout-map-agg: libosmscout libosmscout-map
	(cd libosmscout-map-agg && $(MAKE))

libosmscout-map-cairo: libosmscout libosmscout-map
	(cd libosmscout-map-cairo && $(MAKE))

libosmscout-map-opengl: libosmscout libosmscout-map
	(cd libosmscout-map-opengl && $(MAKE))

libosmscout-map-qt: libosmscout libosmscout-map
	(cd libosmscout-map-qt && $(MAKE))

libosmscout-map-svg: libosmscout libosmscout-map
	(cd libosmscout-map-svg && $(MAKE))

# Applications & Demos

Demos: libosmscout \
       libosmscout-map \
       libosmscout-map-agg \
       libosmscout-map-cairo \
       libosmscout-map-opengl \
       libosmscout-map-qt \
       libosmscout-map-svg
	(cd Demos && $(MAKE))

DumpData: libosmscout
	(cd DumpData && $(MAKE))

Import: libosmscout libosmscout-import
	(cd Import && $(MAKE))

Tests: libosmscout
	(cd Tests && $(MAKE))

OSMScout2: libosmscout libosmscout-map libosmscout-map-qt
	(cd OSMScout2 && $(MAKE))

StyleEditor: libosmscout libosmscout-map libosmscout-map-qt
	(cd StyleEditor && $(MAKE))

