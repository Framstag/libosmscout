%{
#include <osmscout/GroundTile.h>
%}

%template(GroundTileList) std::list<osmscout::GroundTile>;
%template(GroundTileVector) std::vector<osmscout::GroundTile>;
%template(GroundTileCoordVector) std::vector<osmscout::GroundTile::Coord>;

%include <osmscout/GroundTile.h>

