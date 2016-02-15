%{
#include <osmscout/Node.h>
%}

%include <osmscout/TypeConfig.i>

%shared_ptr(osmscout::Node)

%include <osmscout/Node.h>

%template(NodeVector) std::vector<osmscout::NodeRef>;
%template(NodeRef) std::shared_ptr<osmscout::Node>;

