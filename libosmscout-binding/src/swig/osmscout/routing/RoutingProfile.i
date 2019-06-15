%shared_ptr(osmscout::RoutingProfile)
%shared_ptr(osmscout::AbstractRoutingProfile)
%shared_ptr(osmscout::ShortestPathRoutingProfile)
%shared_ptr(osmscout::FastestPathRoutingProfile)

%{
#include <osmscout/routing/RoutingProfile.h>
%}

%template(CarSpeedMap) std::map<std::string,double>;
%template(RoutingProfileVector) std::vector<std::shared_ptr<osmscout::RoutingProfile>>;

%include <osmscout/routing/RoutingProfile.h>

