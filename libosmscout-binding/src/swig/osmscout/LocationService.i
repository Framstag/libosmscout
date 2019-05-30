%{
#include <osmscout/LocationService.h>
%}

%shared_ptr(osmscout::LocationService)

%include <osmscout/LocationService.h>

%template(FileOffsetAdminRegionRefMap) std::map<osmscout::FileOffset,osmscout::AdminRegionRef>;


