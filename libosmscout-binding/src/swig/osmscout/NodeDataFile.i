%include <osmscout/DataFile.i>

%shared_ptr(osmscout::DataFile<osmscout::Node>)
%shared_ptr(osmscout::NodeDataFile)

%{
#include <osmscout/NodeDataFile.h>
%}

%template(NodeDataFileBase) osmscout::DataFile<osmscout::Node>;

%include <osmscout/NodeDataFile.h>


