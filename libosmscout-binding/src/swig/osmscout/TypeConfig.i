%{
#include <osmscout/TypeConfig.h>
%}

%shared_ptr(osmscout::TypeInfo)
%shared_ptr(osmscout::TypeConfig)

%include <osmscout/TypeConfig.h>

%template(FeatureVector) std::vector<osmscout::FeatureRef>;
%template(FeatureInstanceVector) std::vector<osmscout::FeatureInstance>;
%template(TypeInfoVector) std::vector<osmscout::TypeInfoRef>;

