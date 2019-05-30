%{
#include <osmscout/Point.h>
%}

%template(PointVector) std::vector<osmscout::Point>;

%include <osmscout/Point.h>

