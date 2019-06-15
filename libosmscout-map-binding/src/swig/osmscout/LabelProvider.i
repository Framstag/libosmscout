%{
#include <osmscout/LabelProvider.h>
%}

%shared_ptr(osmscout::LabelProvider)
%shared_ptr(osmscout::DynamicFeatureLabelReader)

%shared_ptr(osmscout::LabelProviderFactory)
%shared_ptr(osmscout::INameLabelProviderFactory)

%include <osmscout/LabelProvider.h>


