#export CXX = clang++

programs = libosmscout \
           libosmscout-import \
           libosmscout-map \
           libosmscout-map-agg \
           libosmscout-map-cairo \
           libosmscout-map-iOSX \
           libosmscout-map-opengl \
           libosmscout-map-qt \
           libosmscout-client-qt \
           libosmscout-map-svg \
           DumpData \
           Demos \
           BasemapImport \
           Import \
           OSMScout2 \
           StyleEditor \
           Tests

.PHONY: all autogen configure full clean dist distclean \
        libosmscout \
        libosmscout-import \
        libosmscout-map \
        libosmscout-map-agg \
        libosmscout-map-cairo \
        libosmscout-map-iOSX \
        libosmscout-map-opengl \
        libosmscout-map-qt \
        libosmscout-client-qt \
        libosmscout-map-svg \
        DumpData \
        Demos \
        BasemapImport \
        Import \
        OSMScout2 \
        StyleEditor \
        Tests

all: libosmscout \
     libosmscout-import \
     libosmscout-map \
     libosmscout-map-agg \
     libosmscout-map-cairo \
     libosmscout-map-iOSX \
     libosmscout-map-opengl \
     libosmscout-map-qt \
     libosmscout-client-qt \
     libosmscout-map-svg \
     DumpData \
     Demos \
     BasemapImport \
     Import \
     OSMScout2 \
     StyleEditor \
     Tests

full:
	$(MAKE) autogen
	$(MAKE) configure 
	$(MAKE) all

autogen:
	@for x in $(programs); do\
	  if [ -d $$x ] && [ -f $$x/autogen.sh ]; then \
	    echo Preparing $$x...; \
	    (cd $$x && ./autogen.sh); \
	  fi \
	done

configure:
	@for x in $(programs); do\
	  if [ -d $$x ] && [ -f $$x/configure ]; then \
	    echo Configuring autoconf project $$x...; \
	    (cd $$x && ./configure); \
	  elif [ -d $$x ] && [ -f $$x/*.pro ]; then \
	    echo Configuring Qt project $$x...; \
	    (cd $$x && qmake); \
	  else \
	    echo NOT configuring $$x...; \
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

# libosmscout

libosmscout:
	if [ -f libosmscout/Makefile ]; then \
	  (cd libosmscout && $(MAKE)) \
	fi

# libosmscout-import

libosmscout-import: libosmscout
	if [ -f libosmscout-import/Makefile ]; then \
	  (cd libosmscout-import && $(MAKE)) \
	fi

# libosmscout-map

libosmscout-map: libosmscout
	(cd libosmscout-map && $(MAKE))

# Rendering backends (optional)

libosmscout-map-agg: libosmscout libosmscout-map
	if [ -f libosmscout-map-agg/Makefile ]; then \
	  (cd libosmscout-map-agg && $(MAKE)) \
	fi

libosmscout-map-cairo: libosmscout libosmscout-map
	if [ -f libosmscout-map-cairo/Makefile ]; then \
	  (cd libosmscout-map-cairo && $(MAKE)) \
	fi

libosmscout-map-opengl: libosmscout libosmscout-map
	if [ -f libosmscout-map-opengl/Makefile ]; then \
	  (cd libosmscout-map-opengl && $(MAKE)) \
	fi

libosmscout-map-qt: libosmscout libosmscout-map
	if [ -f libosmscout-map-qt/Makefile ]; then \
	  (cd libosmscout-map-qt && $(MAKE)) \
	fi

libosmscout-client-qt: libosmscout libosmscout-map libosmscout-map-qt
	if [ -f libosmscout-client-qt/Makefile ]; then \
	  (cd libosmscout-client-qt && $(MAKE)) \
	fi

libosmscout-map-iOSX: libosmscout libosmscout-map
	if [ -f libosmscout-map-iOSX/Makefile ]; then \
	  (cd libosmscout-map-iOSX && $(MAKE)) \
	fi

libosmscout-map-svg: libosmscout libosmscout-map
	if [ -f libosmscout-map-svg/Makefile ]; then \
	  (cd libosmscout-map-svg && $(MAKE)) \
	fi

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

BasemapImport: libosmscout libosmscout-import
	(cd BasemapImport && $(MAKE))

Import: libosmscout libosmscout-import
	(cd Import && $(MAKE))

Tests: libosmscout \
       libosmscout-map
	(cd Tests && $(MAKE))

OSMScout2: libosmscout libosmscout-map libosmscout-map-qt libosmscout-client-qt
	if [ -f OSMScout2/Makefile ]; then \
	  (cd OSMScout2 && $(MAKE)) \
	fi

StyleEditor: libosmscout libosmscout-map libosmscout-map-qt libosmscout-client-qt
	if [ -f StyleEditor/Makefile ]; then \
	  (cd StyleEditor && $(MAKE)) \
	fi

