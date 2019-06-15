%{
#include <osmscout/routing/RoutePostprocessor.h>
%}

%shared_ptr(osmscout::RoutePostprocessor)

%include <osmscout/routing/RoutePostprocessor.h>

%template(PostprocessorList) std::list<std::shared_ptr<osmscout::RoutePostprocessor::Postprocessor>>;
