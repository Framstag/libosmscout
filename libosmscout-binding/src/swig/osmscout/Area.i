%{
#include <osmscout/Area.h>
%}

%shared_ptr(osmscout::Area)

%include <osmscout/Area.h>

%template(AreaRingVector) std::vector<osmscout::Area::Ring>;

%template(AreaList) std::list<std::shared_ptr<osmscout::Area>>;
%template(AreaVector) std::vector<std::shared_ptr<osmscout::Area>>;
