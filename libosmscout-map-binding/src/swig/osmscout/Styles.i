%{
#include <osmscout/Styles.h>
%}

%shared_ptr(osmscout::Symbol)

%shared_ptr(osmscout::DrawPrimitive)
%shared_ptr(osmscout::CirclePrimitive)
%shared_ptr(osmscout::RectanglePrimitive)
%shared_ptr(osmscout::PolygonPrimitive)

%shared_ptr(osmscout::BorderStyle)
%shared_ptr(osmscout::FillStyle)
%shared_ptr(osmscout::IconStyle)
%shared_ptr(osmscout::LabelStyle)
%shared_ptr(osmscout::LineStyle)
%shared_ptr(osmscout::PathShieldStyle)
%shared_ptr(osmscout::PathSymbolStyle)
%shared_ptr(osmscout::PathTextStyle)
%shared_ptr(osmscout::ShieldStyle)
%shared_ptr(osmscout::TextStyle)

%template(BorderStyleAttributeSet) std::set<osmscout::BorderStyle::Attribute>;
%template(FillStyleAttributeSet) std::set<osmscout::FillStyle::Attribute>;
%template(IconStyleAttributeSet) std::set<osmscout::IconStyle::Attribute>;
%template(LineStyleAttributeSet) std::set<osmscout::LineStyle::Attribute>;
%template(PathShieldStyleAttributeSet) std::set<osmscout::PathShieldStyle::Attribute>;
%template(PathSymbolStyleAttributeSet) std::set<osmscout::PathSymbolStyle::Attribute>;
%template(PathTextStyleAttributeSet) std::set<osmscout::PathTextStyle::Attribute>;
%template(ShieldStyleAttributeSet) std::set<osmscout::ShieldStyle::Attribute>;
%template(TextStyleAttributeSet) std::set<osmscout::TextStyle::Attribute>;

%include <osmscout/Styles.h>


