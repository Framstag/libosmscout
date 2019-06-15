%{
#include <osmscout/DataFile.h>
%}

%shared_ptr(osmscout::DataFile)
%template(DataBlockSpanVector) std::vector<osmscout::DataBlockSpan>;

%include <osmscout/DataFile.h>

