%{
#include <osmscout/routing/RoutingService.h>
%}

%include <osmscout/util/Breaker.i>

%shared_ptr(osmscout::RoutingProgress)
%shared_ptr(osmscout::RoutingService)

%include <osmscout/routing/RoutingService.h>

