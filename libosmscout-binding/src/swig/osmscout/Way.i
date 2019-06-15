%{
#include <osmscout/Way.h>
%}

%shared_ptr(osmscout::Way)

%include <osmscout/Way.h>

%template(WayList) std::list<std::shared_ptr<osmscout::Way>>;
%template(WayVector) std::vector<std::shared_ptr<osmscout::Way>>;
