%{
#include <osmscout/Area.h>
%}

%shared_ptr(osmscout::Area)

%include <osmscout/Area.h>

%template(AreaRingVector) std::vector<osmscout::Area::Ring>;

%template(AreaVector) std::vector<osmscout::AreaRef>;
