%{
#include <osmscout/routing/RouteNode.h>
%}

%shared_ptr(osmscout::RouteNode)

%template(ObjectVariantDataVector) std::vector<osmscout::ObjectVariantData>;

%include <osmscout/routing/RouteNode.h>

