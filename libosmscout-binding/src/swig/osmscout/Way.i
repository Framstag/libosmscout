%{
#include <osmscout/Way.h>
%}

%shared_ptr(osmscout::Way)

%include <osmscout/Way.h>

%template(WayVector) std::vector<osmscout::WayRef>;
