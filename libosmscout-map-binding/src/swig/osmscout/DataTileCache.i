%{
#include <osmscout/DataTileCache.h>
%}

%shared_ptr(osmscout::Tile)

%template(TileList) std::list<std::shared_ptr<osmscout::Tile>>;

%include <osmscout/DataTileCache.h>

%template(TileAreaData) osmscout::TileData<std::shared_ptr<osmscout::Area>>;
%template(TileNodeData) osmscout::TileData<std::shared_ptr<osmscout::Node>>;
%template(TileWayData) osmscout::TileData<std::shared_ptr<osmscout::Way>>;


