%{
#include <osmscout/Area.h>
%}

%shared_ptr(osmscout::Area)

%include <osmscout/Area.h>

%template(AreaVector) std::vector<osmscout::AreaRef>;
