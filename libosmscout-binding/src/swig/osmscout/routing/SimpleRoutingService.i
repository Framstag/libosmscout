%shared_ptr(osmscout::AbstractRoutingService<osmscout::RoutingProfile>)
%shared_ptr(osmscout::SimpleRoutingService)

%{
#include <osmscout/routing/SimpleRoutingService.h>
%}

%template(SimpleRoutingServiceBase) osmscout::AbstractRoutingService<osmscout::RoutingProfile>;

%include <osmscout/routing/SimpleRoutingService.h>

