%include <osmscout/DataFile.i>

%shared_ptr(osmscout::DataFile<osmscout::Area>)
%shared_ptr(osmscout::AreaDataFile)

%{
#include <osmscout/AreaDataFile.h>
%}

%template(AreaDataFileBase) osmscout::DataFile<osmscout::Area>;

%include <osmscout/AreaDataFile.h>



