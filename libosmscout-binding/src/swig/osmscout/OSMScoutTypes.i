%{
#include <osmscout/OSMScoutTypes.h>
%}

%include <osmscout/OSMScoutTypes.h>

%apply unsigned long int { uint64_t }

// String
%template(StringList) std::list<std::string>;
//%template(StringUnorderedSet) std::unordered_set<std::string>;
%template(StringSet) std::set<std::string>;
%template(StringVector) std::vector<std::string>;
%template(StringStringUnorderedMap) std::unordered_map<std::string,std::string>;
//
%template(UInt32Vector) std::vector<uint32_t>;
//%template(UInt64Vector) std::vector<uint64_t>;

//%template(FileOffsetList) std::list<osmscout::FileOffset>;
//%template(FileOffsetSet) std::set<osmscout::FileOffset>;
%template(FileOffsetVector) std::vector<osmscout::FileOffset>;

%template(DoubleVector) std::vector<double>;

