%{
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeature.h>
%}

%shared_ptr(osmscout::TypeInfo)
%shared_ptr(osmscout::TypeConfig)
%shared_ptr(osmscout::Feature)

%include <osmscout/TypeFeature.h>
%include <osmscout/TypeConfig.h>

%template(FeatureVector) std::vector<osmscout::FeatureRef>;
%template(FeatureInstanceVector) std::vector<osmscout::FeatureInstance>;
%template(FeatureRef) std::shared_ptr<osmscout::Feature>;

