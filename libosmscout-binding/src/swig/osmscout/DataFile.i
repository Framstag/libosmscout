%{
#include <osmscout/DataFile.h>
%}

%shared_ptr(osmscout::DataFile)

%include <osmscout/DataFile.h>

%template(DataBlockSpanVector) std::vector<osmscout::DataBlockSpan>;

