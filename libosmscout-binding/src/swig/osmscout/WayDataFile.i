%include <osmscout/DataFile.i>

%shared_ptr(osmscout::DataFile<osmscout::Way>)
%shared_ptr(osmscout::WayDataFile)

%{
#include <osmscout/WayDataFile.h>
%}

%template(WayDataFileBase) osmscout::DataFile<osmscout::Way>;

%include <osmscout/WayDataFile.h>
