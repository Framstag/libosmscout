%{
#include <osmscout/RoutingService.h>
%}

%include <osmscout/util/Breaker.i>

%shared_ptr(osmscout::RoutingProgress)
%shared_ptr(osmscout::RoutingService)

%include <osmscout/RoutingService.h>

