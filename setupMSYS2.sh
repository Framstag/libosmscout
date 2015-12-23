export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:${PWD}/libosmscout:${PWD}/libosmscout-import:${PWD}/libosmscout-map:${PWD}/libosmscout-map-svg:${PWD}/libosmscout-map-qt:${PWD}/libosmscout-map-agg:${PWD}/libosmscout-map-cairo:${PWD}/libosmscout-map-opengl

if [ -x /mingw64/bin/ccache.exe ]; then
  export CC="ccache gcc.exe"
  export CXX="ccache g++.exe"
else
  export CC="gcc.exe"
  export CXX="g++.exe"
fi  
