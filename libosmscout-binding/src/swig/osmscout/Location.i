%{
#include <osmscout/Location.h>
%}

%shared_ptr(osmscout::Address)
%shared_ptr(osmscout::Location)
%shared_ptr(osmscout::POI)

// AdminRegion
%shared_ptr(osmscout::AdminRegion)
%template(AdminRegionRegionAliasVector) std::vector<osmscout::AdminRegion::RegionAlias>;

// AddressListVisitor
%template(AddressListVisitorAddressResultList) std::list<osmscout::AddressListVisitor::AddressResult>;

// PostalArea
%shared_ptr(osmscout::PostalArea)
%template(PostalAreaVector) std::vector<osmscout::PostalArea>;

%include <osmscout/Location.h>

