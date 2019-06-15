%{
#include <osmscout/Database.h>
%}

%shared_ptr(osmscout::Database)

%include <osmscout/Database.h>

%template(AreaRegionSearchResultEntryList) std::list<osmscout::AreaRegionSearchResultEntry>;

%template(DatabaseVector) std::vector<std::shared_ptr<osmscout::Database>>;

//%template(FileOffsetAreaRefMap) std::unordered_map<osmscout::FileOffset,osmscout::AreaRef>;
//%template(FileOffsetNodeRefMap) std::unordered_map<osmscout::FileOffset,osmscout::NodeRef>;
//%template(FileOffsetWayRefMap) std::unordered_map<osmscout::FileOffset,osmscout::WayRef>;




