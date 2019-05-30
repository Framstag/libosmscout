%{
#include <osmscout/Tag.h>
%}

%include <osmscout/Tag.h>

%shared_ptr(osmscout::TagCondition)

%template(TagMap) std::unordered_map<osmscout::TagId,std::string>;


