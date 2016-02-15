%{
#include <osmscout/RoutingProfile.h>
%}

%shared_ptr(osmscout::RoutingProfile)
%shared_ptr(osmscout::AbstractRoutingProfile)
%shared_ptr(osmscout::ShortestPathRoutingProfile)
%shared_ptr(osmscout::FastestPathRoutingProfile)

%include <osmscout/RoutingProfile.h>

%template(CarSpeedMap) std::map<std::string,double>;

