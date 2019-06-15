%{
#include <osmscout/Node.h>
%}

%include <osmscout/TypeConfig.i>

%shared_ptr(osmscout::Node)

%include <osmscout/Node.h>

%template(NodeList) std::list<std::shared_ptr<osmscout::Node>>;
%template(NodeVector) std::vector<std::shared_ptr<osmscout::Node>>;

