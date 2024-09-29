  /*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/TypeConfig.h>

#include <algorithm>

#include <osmscout/feature/AccessFeature.h>
#include <osmscout/feature/AccessRestrictedFeature.h>
#include <osmscout/feature/AddressFeature.h>
#include <osmscout/feature/AdminLevelFeature.h>
#include <osmscout/feature/BrandFeature.h>
#include <osmscout/feature/BridgeFeature.h>
#include <osmscout/feature/BuildingFeature.h>
#include <osmscout/feature/ChargingStationFeature.h>
#include <osmscout/feature/ClockwiseDirectionFeature.h>
#include <osmscout/feature/ColorFeature.h>
#include <osmscout/feature/ConstructionYearFeature.h>
#include <osmscout/feature/DestinationFeature.h>
#include <osmscout/feature/EleFeature.h>
#include <osmscout/feature/EmbankmentFeature.h>
#include <osmscout/feature/FeeFeature.h>
#include <osmscout/feature/FromToFeature.h>
#include <osmscout/feature/GradeFeature.h>
#include <osmscout/feature/IsInFeature.h>
#include <osmscout/feature/IsInFeature.h>
#include <osmscout/feature/LanesFeature.h>
#include <osmscout/feature/LayerFeature.h>
#include <osmscout/feature/LocationFeature.h>
#include <osmscout/feature/MaxSpeedFeature.h>
#include <osmscout/feature/MaxStayFeature.h>
#include <osmscout/feature/NameFeature.h>
#include <osmscout/feature/NameAltFeature.h>
#include <osmscout/feature/NameShortFeature.h>
#include <osmscout/feature/NetworkFeature.h>
#include <osmscout/feature/OpeningHoursFeature.h>
#include <osmscout/feature/OperatorFeature.h>
#include <osmscout/feature/PhoneFeature.h>
#include <osmscout/feature/PostalCodeFeature.h>
#include <osmscout/feature/RefFeature.h>
#include <osmscout/feature/RoundaboutFeature.h>
#include <osmscout/feature/SidewayFeature.h>
#include <osmscout/feature/TunnelFeature.h>
#include <osmscout/feature/WebsiteFeature.h>
#include <osmscout/feature/WidthFeature.h>

#include <osmscout/system/Assert.h>

#include <osmscout/ost/Parser.h>
#include <osmscout/ost/Scanner.h>

#include <osmscout/log/Logger.h>
#include <osmscout/util/Number.h>

#include <osmscout/io/File.h>

  namespace osmscout {

  const char* TypeConfig::FILE_TYPES_DAT="types.dat";

  TypeInfo::TypeInfo(const std::string& name)
    : name(name)
  {

  }

  TypeInfo& TypeInfo::SetNodeId(TypeId id)
  {
    this->nodeId=id;

    return *this;
  }

  TypeInfo& TypeInfo::SetWayId(TypeId id)
  {
    this->wayId=id;

    return *this;
  }

  TypeInfo& TypeInfo::SetAreaId(TypeId id)
  {
    this->areaId=id;

    return *this;
  }

  TypeInfo& TypeInfo::SetRouteId(TypeId id)
  {
    this->routeId=id;

    return *this;
  }

  TypeInfo& TypeInfo::SetIndex(size_t index)
  {
    this->index=index;

    return *this;
  }

  TypeInfo& TypeInfo::SetInternal()
  {
    this->internal=true;

    return *this;
  }

  TypeInfo& TypeInfo::SetType(const std::string& name)
  {
    this->name=name;

    return *this;
  }

  TypeInfo& TypeInfo::AddCondition(unsigned char types,
                                   const TagConditionRef& condition)
  {
    TypeCondition typeCondition;

    if ((types & typeNode)!=0) {
      canBeNode=true;
    }

    if ((types & typeWay)!=0) {
      canBeWay=true;
    }

    if ((types & typeArea)!=0) {
      canBeArea=true;
    }

    if ((types & typeRelation)!=0) {
      canBeRelation=true;
    }

    typeCondition.types=types;
    typeCondition.condition=condition;

    conditions.push_back(typeCondition);

    return *this;
  }

  TypeInfo& TypeInfo::AddFeature(const FeatureRef& feature)
  {
    assert(feature);
    assert(!nameToFeatureMap.contains(feature->GetName()));

    size_t featureBit=0;
    size_t index=0;
    size_t offset=0;

    if (!features.empty()) {
      featureBit=features.back().GetFeatureBit()+1+feature->GetFeatureBitCount();
      index=features.back().GetIndex()+1;
      offset=features.back().GetOffset()+features.back().GetFeature()->GetValueSize();
      size_t alignment=feature->GetValueAlignment();
      if (alignment!=0 && (offset%alignment!=0)) {
        offset=(offset/alignment+1)*alignment;
      }
    }


    features.emplace_back(feature,
                          this,
                          featureBit,
                          index,
                          offset);
    nameToFeatureMap.emplace(feature->GetName(),index);

    size_t featureBitCount=0;

    if (!features.empty()) {
      featureBitCount=features.back().GetFeatureBit()+feature->GetFeatureBitCount()+1;
    }

    featureMaskBytes=BitsToBytes(featureBitCount);
    specialFeatureMaskBytes=BitsToBytes(featureBitCount+1);

    valueBufferSize=offset+feature->GetValueSize();

    return *this;
  }

  TypeInfo& TypeInfo::AddGroup(const std::string& groupName)
  {
    groups.insert(groupName);

    return *this;
  }

  /**
   * Add a description of the type for the given language code
   * @param languageCode
   *    language code like for example 'en'or 'de'
   * @param description
   *    description of the type
   * @return
   *    type info instance
   */
  TypeInfo& TypeInfo::AddDescription(const std::string& languageCode,
                                     const std::string& description)
  {
    descriptions[languageCode]=description;

    return *this;
  }

  bool TypeInfo::HasFeature(const std::string& featureName) const
  {
    return nameToFeatureMap.contains(featureName);
  }

  /**
   * Return the feature with the given name
   */
  bool TypeInfo::GetFeature(const std::string& name,
                            size_t& index) const
  {
    auto entry=nameToFeatureMap.find(name);

    if (entry!=nameToFeatureMap.end()) {
      index=entry->second;

      return true;
    }

    return false;
  }

  uint8_t TypeInfo::GetDefaultAccess() const
  {
    uint8_t access=0;

    if (CanRouteFoot()) {
      access|=(AccessFeatureValue::footForward|AccessFeatureValue::footBackward);
    }

    if (CanRouteBicycle()) {
      access|=(AccessFeatureValue::bicycleForward|AccessFeatureValue::bicycleBackward);
    }

    if (CanRouteCar()) {
      access|=(AccessFeatureValue::carForward|AccessFeatureValue::carBackward);
    }

    return access;
  }

  /**
   * Returns the description for the given language code. Returns an empty string, if
   * no description is available for the given language code.
   *
   * @param languageCode
   *    languageCode like for example 'en' or 'de'
   * @return
   *    Description or empty string
   */
  std::string TypeInfo::GetDescription(const std::string& languageCode) const
  {
    auto entry=descriptions.find(languageCode);

    if (entry!=descriptions.end()) {
      return entry->second;
    }

    return "";
  }

  TypeInfoRef TypeInfo::Read(FileScanner& scanner)
  {
    std::string name=scanner.ReadString();

    TypeInfoRef typeInfo=std::make_shared<TypeInfo>(name);

    bool        canBeNode;
    bool        canBeWay;
    bool        canBeArea;
    bool        canBeRelation;
    bool        isPath;
    bool        canRouteFoot;
    bool        canRouteBicycle;
    bool        canRouteCar;
    bool        indexAsAddress;
    bool        indexAsLocation;
    bool        indexAsRegion;
    bool        indexAsPOI;
    bool        optimizeLowZoom;
    uint8_t     specialType;
    bool        pinWay;
    bool        mergeAreas;
    bool        ignore;
    bool        ignoreSeaLand;
    uint8_t     lanes;
    uint8_t     onewayLanes;

    canBeNode=scanner.ReadBool();
    canBeWay=scanner.ReadBool();
    canBeArea=scanner.ReadBool();
    canBeRelation=scanner.ReadBool();
    isPath=scanner.ReadBool();
    canRouteFoot=scanner.ReadBool();
    canRouteBicycle=scanner.ReadBool();
    canRouteCar=scanner.ReadBool();
    indexAsAddress=scanner.ReadBool();
    indexAsLocation=scanner.ReadBool();
    indexAsRegion=scanner.ReadBool();
    indexAsPOI=scanner.ReadBool();
    optimizeLowZoom=scanner.ReadBool();
    specialType=scanner.ReadUInt8();
    pinWay=scanner.ReadBool();
    mergeAreas=scanner.ReadBool();
    ignoreSeaLand=scanner.ReadBool();
    ignore=scanner.ReadBool();
    lanes=scanner.ReadUInt8();
    onewayLanes=scanner.ReadUInt8();

    typeInfo->CanBeNode(canBeNode);
    typeInfo->CanBeWay(canBeWay);
    typeInfo->CanBeArea(canBeArea);
    typeInfo->CanBeRelation(canBeRelation);
    typeInfo->SetIsPath(isPath);
    typeInfo->CanRouteFoot(canRouteFoot);
    typeInfo->CanRouteBicycle(canRouteBicycle);
    typeInfo->CanRouteCar(canRouteCar);
    typeInfo->SetIndexAsAddress(indexAsAddress);
    typeInfo->SetIndexAsLocation(indexAsLocation);
    typeInfo->SetIndexAsRegion(indexAsRegion);
    typeInfo->SetIndexAsPOI(indexAsPOI);
    typeInfo->SetOptimizeLowZoom(optimizeLowZoom);
    typeInfo->SetSpecialType(static_cast<SpecialType>(specialType));
    typeInfo->SetPinWay(pinWay);
    typeInfo->SetMergeAreas(mergeAreas);
    typeInfo->SetIgnoreSeaLand(ignoreSeaLand);
    typeInfo->SetIgnore(ignore);
    typeInfo->SetLanes(lanes);
    typeInfo->SetOnewayLanes(onewayLanes);

    return typeInfo;
  }

  FeatureValueBuffer::FeatureValueBuffer(const FeatureValueBuffer& other)
  {
    Set(other);
  }

  FeatureValueBuffer::FeatureValueBuffer(FeatureValueBuffer&& other) noexcept
  {
    std::swap(type, other.type);
    std::swap(featureBits, other.featureBits);
    std::swap(featureValueBuffer, other.featureValueBuffer);
  }

  FeatureValueBuffer::~FeatureValueBuffer()
  {
    if (type) {
      DeleteData();
    }
  }

  FeatureValueBuffer& FeatureValueBuffer::operator=(const FeatureValueBuffer& other)
  {
    if (this!=&other) {
      Set(other);
    }

    return *this;
  }

  FeatureValueBuffer& FeatureValueBuffer::operator=(FeatureValueBuffer&& other) noexcept
  {
    std::swap(type, other.type);
    std::swap(featureBits, other.featureBits);
    std::swap(featureValueBuffer, other.featureValueBuffer);
    return *this;
  }


  void FeatureValueBuffer::Set(const FeatureValueBuffer& other)
  {
    if (type) {
      DeleteData();
    }

    if (other.GetType()) {
      SetType(other.GetType());

      for (size_t idx=0; idx<other.GetFeatureCount(); idx++) {
        if (other.HasFeature(idx)) {
          if (other.GetFeature(idx).GetFeature()->HasValue()) {
            const FeatureValue* otherValue=other.GetValue(idx);
            FeatureValue* thisValue=AllocateValue(idx);

            *thisValue=*otherValue;
          }
          else {
            size_t featureBit=GetFeature(idx).GetFeatureBit();
            size_t byteIdx=featureBit/8;

            featureBits[byteIdx]=featureBits[byteIdx] | (1u << featureBit%8);
          }
        }
      }
    }
  }

  void FeatureValueBuffer::SetType(const TypeInfoRef& type)
  {
    if (this->type) {
      DeleteData();
    }

    this->type=type;

    AllocateBits();
    featureValueBuffer=nullptr; // buffer is allocated on first usage
  }

  void FeatureValueBuffer::DeleteData()
  {
    if (featureValueBuffer!=nullptr) {
      for (size_t i=0; i<type->GetFeatureCount(); i++) {
        if (HasFeature(i)) {
          FreeValue(i);
        }
      }

      ::operator delete((void*)featureValueBuffer);
      featureValueBuffer=nullptr;
    }

    if (featureBits!=nullptr) {
      delete [] featureBits;
      featureBits=nullptr;
    }

    type=nullptr;
  }

  void FeatureValueBuffer::AllocateBits()
  {
    if (type && type->HasFeatures()) {
      featureBits=new uint8_t[type->GetFeatureMaskBytes()]();
    }
    else
    {
      featureBits=nullptr;
    }
  }

  void FeatureValueBuffer::AllocateValueBufferLazy()
  {
    if (featureValueBuffer==nullptr &&
        type &&
        type->HasFeatures()) {
      featureValueBuffer=static_cast<char*>(::operator new(type->GetFeatureValueBufferSize()));
    }
  }

  FeatureValue* FeatureValueBuffer::AllocateValue(size_t idx)
  {
    size_t featureBit=GetFeature(idx).GetFeatureBit();
    size_t byteIdx=featureBit/8;

    featureBits[byteIdx]=featureBits[byteIdx] | (1u << featureBit%8);

    if (type->GetFeature(idx).GetFeature()->HasValue()) {
      FeatureValue* value=GetValueAndAllocateBuffer(idx);

      return type->GetFeature(idx).GetFeature()->AllocateValue(value);
    }

    return nullptr;
  }

  void FeatureValueBuffer::FreeValue(size_t idx)
  {
    if (HasFeature(idx)) {
      if (type->GetFeature(idx).GetFeature()->HasValue()) {
        FeatureValue* value=GetValue(idx);
        value->~FeatureValue();
      }

      // clear feature bit
      size_t featureBit = GetFeature(idx).GetFeatureBit();
      size_t byteIdx = featureBit / 8;
      featureBits[byteIdx] = featureBits[byteIdx] & ~(1u << featureBit % 8);
    }
  }

  void FeatureValueBuffer::Parse(TagErrorReporter& errorReporter,
                                 const TagRegistry& tagRegistry,
                                 const ObjectOSMRef& object,
                                 const TagMap& tags)
  {
    for (const auto &feature : type->GetFeatures()) {
      feature.GetFeature()->Parse(errorReporter,
                                  tagRegistry,
                                  feature,
                                  object,
                                  tags,
                                  *this);
    }
  }

  void FeatureValueBuffer::Read(FileScanner& scanner)
  {
    std::array<bool,0> specialFlags;
    Read<0>(scanner, specialFlags);
  }

  void FeatureValueBuffer::Read(FileScanner& scanner,
                                bool& specialFlag)
  {
    std::array<bool,1> specialFlags;
    Read<1>(scanner, specialFlags);
    specialFlag=specialFlags[0];
  }

  void FeatureValueBuffer::Read(FileScanner& scanner,
                                bool& specialFlag1,
                                bool& specialFlag2)
  {
    std::array<bool,2> specialFlags;
    Read<2>(scanner, specialFlags);
    specialFlag1=specialFlags[0];
    specialFlag2=specialFlags[1];
  }

  void FeatureValueBuffer::Read(FileScanner& scanner,
                                bool& specialFlag1,
                                bool& specialFlag2,
                                bool& specialFlag3)
  {
    std::array<bool,3> specialFlags;
    Read<3>(scanner, specialFlags);
    specialFlag1=specialFlags[0];
    specialFlag2=specialFlags[1];
    specialFlag3=specialFlags[2];
  }

  void FeatureValueBuffer::Write(FileWriter& writer) const
  {
    Write<0>(writer, std::array<bool,0>());
  }

  void FeatureValueBuffer::Write(FileWriter& writer,
                                 bool specialFlag) const
  {
    Write<1>(writer, std::array<bool,1>{specialFlag});
  }

  void FeatureValueBuffer::Write(FileWriter& writer,
                                 bool specialFlag1,
                                 bool specialFlag2) const
  {
    Write<2>(writer, std::array<bool,2>{specialFlag1, specialFlag2});
  }

  void FeatureValueBuffer::Write(FileWriter& writer,
                                 bool specialFlag1,
                                 bool specialFlag2,
                                 bool specialFlag3) const
  {
    Write<3>(writer, std::array<bool,3>{specialFlag1, specialFlag2, specialFlag3});
  }

  bool FeatureValueBuffer::operator==(const FeatureValueBuffer& other) const
  {
    if (this->type!=other.type) {
      return false;
    }

    for (size_t i=0; i<GetFeatureCount(); i++) {
      if (HasFeature(i)!=other.HasFeature(i)) {
        return false;
      }

      // If a feature has a value, we compare the values
      if (HasFeature(i) &&
          other.HasFeature(i) &&
          GetFeature(i).GetFeature()->HasValue()) {
        const FeatureValue *thisValue=GetValue(i);
        const FeatureValue *otherValue=other.GetValue(i);

        if (!(*thisValue==*otherValue)) {
          return false;
        }
      }
    }

    return true;
  }

  bool FeatureValueBuffer::operator!=(const FeatureValueBuffer& other) const
  {
    return !operator==(other);
  }

  void FeatureValueBuffer::CopyMissingValues(const FeatureValueBuffer& other)
  {
    for (size_t i=0; i<other.GetFeatureCount(); i++) {
      // Feature set?
      if (!other.HasFeature(i)) {
        continue;
      }

      std::string featureName=other.GetFeature(i).GetFeature()->GetName();
      size_t      featureIndex;

      // Does our type has this feature, too?
      if (!GetType()->GetFeature(featureName,
                                 featureIndex)) {
        continue;
      }

      // We do not overwrite existing feature values
      if (HasFeature(featureIndex)) {
        continue;
      }

      // Copy feature with/without value
      if (other.GetFeature(i).GetFeature()->HasValue()) {
        const FeatureValue* otherValue=other.GetValue(i);
        FeatureValue* thisValue=AllocateValue(featureIndex);

        *thisValue=*otherValue;
      }
      else {
        size_t featureBit=GetFeature(featureIndex).GetFeatureBit();
        size_t byteIdx=featureBit/8;

        featureBits[byteIdx]=featureBits[byteIdx] | (1u << featureBit%8);
      }
    }
  }

  void FeatureValueBuffer::ClearFeatureValues()
  {
    for (size_t i=0; i<GetFeatureCount(); i++) {
      FreeValue(i);
    }
  }

  TypeConfig::TypeConfig()
  {
    log.Debug() << "TypeConfig::TypeConfig()";

    featureName=std::make_shared<NameFeature>();
    RegisterFeature(featureName);

    RegisterFeature(std::make_shared<NameAltFeature>());

    featureNameShort=std::make_shared<NameShortFeature>();
    RegisterFeature(featureNameShort);

    featureRef=std::make_shared<RefFeature>();
    RegisterFeature(featureRef);

    featureLocation=std::make_shared<LocationFeature>();
    RegisterFeature(featureLocation);

    featureAddress=std::make_shared<AddressFeature>();
    RegisterFeature(featureAddress);

    featureAccess=std::make_shared<AccessFeature>();
    RegisterFeature(featureAccess);

    featureAccessRestricted=std::make_shared<AccessRestrictedFeature>();
    RegisterFeature(featureAccessRestricted);

    featureLayer=std::make_shared<LayerFeature>();
    RegisterFeature(featureLayer);

    featureWidth=std::make_shared<WidthFeature>();
    RegisterFeature(featureWidth);

    featureMaxSpeed=std::make_shared<MaxSpeedFeature>();
    RegisterFeature(featureMaxSpeed);

    featureGrade=std::make_shared<GradeFeature>();
    RegisterFeature(featureGrade);

    RegisterFeature(std::make_shared<AdminLevelFeature>());

    featurePostalCode = std::make_shared<PostalCodeFeature>();
    RegisterFeature(featurePostalCode);

    featureWebsite = std::make_shared<WebsiteFeature>();
    RegisterFeature(featureWebsite);

    featurePhone = std::make_shared<PhoneFeature>();
    RegisterFeature(featurePhone);

    featureBridge=std::make_shared<BridgeFeature>();
    RegisterFeature(featureBridge);

    featureTunnel=std::make_shared<TunnelFeature>();
    RegisterFeature(featureTunnel);

    featureEmbankment=std::make_shared<EmbankmentFeature>();
    RegisterFeature(featureEmbankment);

    featureRoundabout=std::make_shared<RoundaboutFeature>();
    RegisterFeature(featureRoundabout);

    RegisterFeature(std::make_shared<ClockwiseDirectionFeature>());

    RegisterFeature(std::make_shared<EleFeature>());
    RegisterFeature(std::make_shared<DestinationFeature>());
    RegisterFeature(std::make_shared<BuildingFeature>());

    RegisterFeature(std::make_shared<IsInFeature>());

    RegisterFeature(std::make_shared<ConstructionYearFeature>());

    RegisterFeature(std::make_shared<SidewayFeature>());

    featureLanes=std::make_shared<LanesFeature>();
    RegisterFeature(featureLanes);

    RegisterFeature(std::make_shared<BrandFeature>());
    RegisterFeature(std::make_shared<OperatorFeature>());
    RegisterFeature(std::make_shared<NetworkFeature>());
    RegisterFeature(std::make_shared<FromToFeature>());
    RegisterFeature(std::make_shared<ColorFeature>());

    featureOpeningHours=std::make_shared<OpeningHoursFeature>();
    RegisterFeature(featureOpeningHours);

    RegisterFeature(std::make_shared<ChargingStationFeature>());
    RegisterFeature(std::make_shared<MaxStayFeature>());
    RegisterFeature(std::make_shared<FeeFeature>());

    // Make sure, that this is always registered first.
    // It assures that id 0 is always reserved for typeIgnore
    typeInfoIgnore=std::make_shared<TypeInfo>("");
    typeInfoIgnore->SetIgnore(true);

    RegisterType(typeInfoIgnore);

    //
    // Internal type for showing routes
    //

    TypeInfoRef route=std::make_shared<TypeInfo>("_route");
    route->SetInternal().CanBeWay(true);
    RegisterType(route);

    //
    // Internal types for the land/sea/coast tiles building the base layer for map drawing
    //

    typeInfoTileLand=std::make_shared<TypeInfo>("_tile_land");
    typeInfoTileLand->SetInternal().CanBeArea(true);
    RegisterType(typeInfoTileLand);


    typeInfoTileSea=std::make_shared<TypeInfo>("_tile_sea");
    typeInfoTileSea->SetInternal().CanBeArea(true);
    RegisterType(typeInfoTileSea);


    typeInfoTileCoast=std::make_shared<TypeInfo>("_tile_coast");
    typeInfoTileCoast->SetInternal().CanBeArea(true);
    RegisterType(typeInfoTileCoast);


    typeInfoTileUnknown=std::make_shared<TypeInfo>("_tile_unknown");
    typeInfoTileUnknown->SetInternal().CanBeArea(true);
    RegisterType(typeInfoTileUnknown);


    typeInfoCoastline=std::make_shared<TypeInfo>("_tile_coastline");
    typeInfoCoastline->SetInternal().CanBeWay(true);
    RegisterType(typeInfoCoastline);

    typeInfoOSMTileBorder=std::make_shared<TypeInfo>("_osm_tile_border");
    typeInfoOSMTileBorder->SetInternal().CanBeWay(true);
    RegisterType(typeInfoOSMTileBorder);

    typeInfoOSMSubTileBorder=std::make_shared<TypeInfo>("_osm_subtile_border");
    typeInfoOSMSubTileBorder->SetInternal().CanBeWay(true);
    RegisterType(typeInfoOSMSubTileBorder);

    tagArea=tagRegistry.GetTagId("area");
    tagNatural=tagRegistry.GetTagId("natural");
    tagDataPolygon=tagRegistry.GetTagId("datapolygon");
    tagType=tagRegistry.GetTagId("type");
    tagRestriction=tagRegistry.GetTagId("restriction");
    tagJunction=tagRegistry.GetTagId("junction");

    assert(tagArea!=tagIgnore);
    assert(tagNatural!=tagIgnore);
    assert(tagDataPolygon!=tagIgnore);
    assert(tagType!=tagIgnore);
    assert(tagRestriction!=tagIgnore);
    assert(tagJunction!=tagIgnore);
  }

  TypeConfig::~TypeConfig()
  {
    log.Debug() << "TypeConfig::~TypeConfig()";
  }

  void TypeConfig::RegisterFeature(const FeatureRef& feature)
  {
    assert(feature);
    assert(!feature->GetName().empty());

    if (nameToFeatureMap.contains(feature->GetName())) {
      return;
    }

    features.push_back(feature);
    nameToFeatureMap[feature->GetName()]=feature;

    feature->Initialize(tagRegistry);
  }

  FeatureRef TypeConfig::GetFeature(const std::string& name) const
  {
    auto feature=nameToFeatureMap.find(name);

    if (feature!=nameToFeatureMap.end()) {
      return feature->second;
    }

    return nullptr;
  }

  TypeInfoRef TypeConfig::RegisterType(const TypeInfoRef& typeInfo)
  {
    assert(typeInfo);

    auto existingType=nameToTypeMap.find(typeInfo->GetName());

    if (existingType!=nameToTypeMap.end()) {
      return existingType->second;
    }

    // All ways have a layer
    if (typeInfo->CanBeWay() &&
        !typeInfo->HasFeature(LayerFeature::NAME)) {
      typeInfo->AddFeature(featureLayer);
    }

    // All that is PATH-like automatically has a number of features,
    // even if it is not routable
    if (typeInfo->CanBeWay() &&
        typeInfo->IsPath()) {
      if (!typeInfo->HasFeature(WidthFeature::NAME)) {
        typeInfo->AddFeature(featureWidth);
      }
      if (!typeInfo->HasFeature(GradeFeature::NAME)) {
        typeInfo->AddFeature(featureGrade);
      }
      if (!typeInfo->HasFeature(BridgeFeature::NAME)) {
        typeInfo->AddFeature(featureBridge);
      }
      if (!typeInfo->HasFeature(TunnelFeature::NAME)) {
        typeInfo->AddFeature(featureTunnel);
      }
      if (!typeInfo->HasFeature(EmbankmentFeature::NAME)) {
        typeInfo->AddFeature(featureEmbankment);
      }
      if (!typeInfo->HasFeature(RoundaboutFeature::NAME)) {
        typeInfo->AddFeature(featureRoundabout);
      }
      if (!typeInfo->HasFeature(LanesFeature::NAME)) {
        typeInfo->AddFeature(featureLanes);
      }
    }

    // Everything routable should have access information and max speed information
    if ((typeInfo->CanBeArea() ||
         typeInfo->CanBeWay()) &&
         typeInfo->CanRoute()) {
      if (!typeInfo->HasFeature(AccessFeature::NAME)) {
        typeInfo->AddFeature(featureAccess);
      }
      if (!typeInfo->HasFeature(AccessRestrictedFeature::NAME)) {
        typeInfo->AddFeature(featureAccessRestricted);
      }
      if (!typeInfo->HasFeature(MaxSpeedFeature::NAME)) {
        typeInfo->AddFeature(featureMaxSpeed);
      }
    }

    // All addressable areas and nodes get the postal code, location and address feature
    if ((typeInfo->CanBeArea() ||
         typeInfo->CanBeNode()) &&
        typeInfo->GetIndexAsAddress()) {
      if (!typeInfo->HasFeature(PostalCodeFeature::NAME)) {
        typeInfo->AddFeature(featurePostalCode);
      }
      if (!typeInfo->HasFeature(LocationFeature::NAME)) {
        typeInfo->AddFeature(featureLocation);
      }
      if (!typeInfo->HasFeature(AddressFeature::NAME)) {
        typeInfo->AddFeature(featureAddress);
      }
    }

    // All ways with a name have a postal code and a location
    if (typeInfo->CanBeWay() &&
        typeInfo->HasFeature(NameFeature::NAME)) {
      if (!typeInfo->HasFeature(PostalCodeFeature::NAME)) {
        typeInfo->AddFeature(featurePostalCode);
      }
    }

    // Something that has a name and is a POI automatically gets the
    // postal code, location, address, website, phone and opening hours features, too.
    if (typeInfo->HasFeature(NameFeature::NAME) &&
        typeInfo->GetIndexAsPOI()) {
      if (!typeInfo->HasFeature(PostalCodeFeature::NAME)) {
        typeInfo->AddFeature(featurePostalCode);
      }
      if (!typeInfo->HasFeature(LocationFeature::NAME)) {
        typeInfo->AddFeature(featureLocation);
      }
      if (!typeInfo->HasFeature(AddressFeature::NAME)) {
        typeInfo->AddFeature(featureAddress);
      }
      if (!typeInfo->HasFeature(WebsiteFeature::NAME)) {
        typeInfo->AddFeature(featureWebsite);
      }
      if (!typeInfo->HasFeature(PhoneFeature::NAME)) {
        typeInfo->AddFeature(featurePhone);
      }
      if (!typeInfo->HasFeature(OpeningHoursFeature::NAME)) {
        typeInfo->AddFeature(featureOpeningHours);
      }
    }

    typeInfo->SetIndex(types.size());

    types.push_back(typeInfo);

    if (!typeInfo->GetIgnore() &&
        !typeInfo->IsInternal() &&
        (typeInfo->CanBeNode() ||
         typeInfo->CanBeWay() ||
         typeInfo->CanBeArea() ||
         typeInfo->IsRoute())) {

      if (typeInfo->CanBeNode()) {
        typeInfo->SetNodeId((TypeId)(nodeTypes.size()+1));
        nodeTypes.push_back(typeInfo);

        nodeTypeIdBytes=BytesNeededToEncodeNumber(typeInfo->GetNodeId());
      }

      if (typeInfo->CanBeWay()) {
        typeInfo->SetWayId((TypeId)(wayTypes.size()+1));
        wayTypes.push_back(typeInfo);

        wayTypeIdBytes=BytesNeededToEncodeNumber(typeInfo->GetWayId());
      }

      if (typeInfo->CanBeArea()) {
        typeInfo->SetAreaId((TypeId)(areaTypes.size()+1));
        areaTypes.push_back(typeInfo);

        areaTypeIdBytes=BytesNeededToEncodeNumber(typeInfo->GetAreaId());
        areaTypeIdBits=BitsNeededToEncodeNumber(typeInfo->GetAreaId());
      }

      if (typeInfo->IsRoute()) {
        typeInfo->SetRouteId((TypeId)(routeTypes.size()+1));
        routeTypes.push_back(typeInfo);

        routeTypeIdBytes=BytesNeededToEncodeNumber(typeInfo->GetRouteId());
      }
    }

    nameToTypeMap[typeInfo->GetName()]=typeInfo;

    return typeInfo;
  }

  TypeId TypeConfig::GetMaxTypeId() const
  {
    if (types.empty()) {
      return 0;
    }

    return (TypeId)types.size();
  }

  TypeInfoRef TypeConfig::GetTypeInfo(const std::string& name) const
  {
    auto typeEntry=nameToTypeMap.find(name);

    if (typeEntry!=nameToTypeMap.end()) {
      return typeEntry->second;
    }

    return {};
  }

  TypeInfoRef TypeConfig::GetNodeType(const TagMap& tagMap) const
  {
    if (tagMap.empty()) {
      return typeInfoIgnore;
    }

    for (const auto &type : types) {
      if (!type->HasConditions() ||
          !type->CanBeNode()) {
        continue;
      }

      for (const auto &cond : type->GetConditions()) {
        if ((cond.types & TypeInfo::typeNode)==0) {
          continue;
        }

        if (cond.condition->Evaluate(tagMap)) {
          return type;
        }
      }
    }

    return typeInfoIgnore;
  }

  bool TypeConfig::GetWayAreaType(const TagMap& tagMap,
                                  TypeInfoRef& wayType,
                                  TypeInfoRef& areaType) const
  {
    wayType=typeInfoIgnore;
    areaType=typeInfoIgnore;

    if (tagMap.empty()) {
      return false;
    }

    for (const auto& type : types) {
      if (!((type->CanBeWay() ||
             type->CanBeArea()) &&
             type->HasConditions())) {
        continue;
      }

      for (const auto& cond : type->GetConditions()) {
        if (!((cond.types & TypeInfo::typeWay)!=0 ||
              (cond.types & TypeInfo::typeArea)!=0)) {
          continue;
        }

        if (cond.condition->Evaluate(tagMap)) {
          if (wayType==typeInfoIgnore &&
              (cond.types & TypeInfo::typeWay)!=0) {
            wayType=type;
          }

          if (areaType==typeInfoIgnore &&
              (cond.types & TypeInfo::typeArea)!=0) {
            areaType=type;
          }

          if (wayType!=typeInfoIgnore ||
              areaType!=typeInfoIgnore) {
            return true;
          }
        }
      }
    }

    return false;
  }

  TypeInfoRef TypeConfig::GetRelationType(const TagMap& tagMap) const
  {
    if (tagMap.empty()) {
      return typeInfoIgnore;
    }

    auto relationType=tagMap.find(tagType);

    if (relationType!=tagMap.end() &&
        relationType->second=="multipolygon") {
      for (const auto& type : types) {
        if (!type->HasConditions() ||
            !type->CanBeArea()) {
          continue;
        }

        for (const auto &cond : type->GetConditions()) {
          if ((cond.types & TypeInfo::typeArea)==0) {
            continue;
          }

          if (cond.condition->Evaluate(tagMap)) {
            return type;
          }
        }
      }
    }
    else {
      for (const auto& type : types) {
        if (!type->HasConditions() ||
            !type->CanBeRelation()) {
          continue;
        }

        for (const auto &cond : type->GetConditions()) {
          if ((cond.types & TypeInfo::typeRelation)==0) {
            continue;
          }

          if (cond.condition->Evaluate(tagMap)) {
            return type;
          }
        }
      }
    }

    return typeInfoIgnore;
  }

  /**
   * Loads the type configuration from the given *.ost file.
   *
   * Note:
   * Make sure that you load from a OST file only onto a freshly initialized
   * TypeConfig instance.
   *
   * @param filename
   *    Full filename including path of the OST file
   * @return
   *    True, if there were no errors, else false
   */
  bool TypeConfig::LoadFromOSTFile(const std::string& filename)
  {
    bool success=false;

    try {
      FileOffset fileSize=GetFileSize(filename);
      FILE       *file=fopen(filename.c_str(),"rb");

      if (file==nullptr) {
        log.Error() << "Cannot open file '" << filename << "'";
        return false;
      }

      unsigned char* content=new unsigned char[fileSize];

      if (fread(content,1,fileSize,file)!=(size_t)fileSize) {
        log.Error() << "Cannot load file '" << filename << "'";
        delete [] content;
        fclose(file);
        return false;
      }

      fclose(file);

      ost::Scanner *scanner=new ost::Scanner(content,
                                             fileSize);
      ost::Parser  *parser=new ost::Parser(scanner,
                                           filename,
                                           *this);

      delete [] content;

      parser->Parse();

      success=!parser->errors->hasErrors;

      delete parser;
      delete scanner;
    }
    catch (IOException const& e) {
      log.Error() << e.GetDescription();
    }

    return success;
  }

  /**
   * Returns the file format version of the given database (scanning the
   * "types.dat" file in the given directory) or an IOException.
   *
   * @param directory
   * @return
   */
  uint32_t TypeConfig::GetDatabaseFileFormatVersion(const std::string& directory)
  {
    FileScanner scanner;

    try {
      scanner.Open(AppendFileToDir(directory,
                                   "types.dat"),
                   FileScanner::Sequential,
                   true);

      uint32_t fileFormatVersion=scanner.ReadUInt32();

      scanner.Close();

      return fileFormatVersion;
    }
    catch (const IOException& e) {
      scanner.CloseFailsafe();
     throw e;
    }
  }

  /**
   * Loads the type configuration from the given binary data file.
   *
   * Note:
   * Make sure that you load from file only onto a freshly initialized
   * TypeConfig instance.
   *
   * @param directory
   *    Full path excluding the actual filename of the data file
   *    (filename is always "types.dat")
   * @return
   *    True, if there were no errors, else false
   */
  bool TypeConfig::LoadFromDataFile(const std::string& directory)
  {
    StopClock   timer;
    FileScanner scanner;

    try {
      scanner.Open(AppendFileToDir(directory,
                                   "types.dat"),
                   FileScanner::Sequential,
                   true);

      uint32_t fileFormatVersion=scanner.ReadUInt32();

      if (fileFormatVersion!=FILE_FORMAT_VERSION) {
        log.Error() << "File '" << scanner.GetFilename() << "' does not have the expected format version! Actual " << fileFormatVersion << ", expected: " << FILE_FORMAT_VERSION;
        return false;
      }

      // Features
      uint32_t featureCount=scanner.ReadUInt32Number();

      for (uint32_t f=1; f<=featureCount; f++) {
        std::string featureName=scanner.ReadString();
        uint32_t    descriptionCount=scanner.ReadUInt32Number();
        FeatureRef  feature=GetFeature(featureName);

        for (uint32_t d=1; d<=descriptionCount; d++) {
          std::string languageCode=scanner.ReadString();
          std::string description=scanner.ReadString();

          if (feature) {
            feature->AddDescription(languageCode,
                                    description);
          }
        }
      }

      // Types

      uint32_t typeCount=scanner.ReadUInt32Number();

      for (uint32_t i=1; i<=typeCount; i++) {
        TypeInfoRef typeInfo=TypeInfo::Read(scanner);

        // Type Features

        uint32_t typeFeatureCount=scanner.ReadUInt32Number();

        for (uint32_t f=0; f<typeFeatureCount; f++) {
          std::string featureName=scanner.ReadString();

          FeatureRef feature=GetFeature(featureName);

          if (!feature) {
            log.Error() << "Feature '" << featureName << "' not found";
            return false;
          }

          typeInfo->AddFeature(feature);
        }

        // Groups

        uint32_t groupCount=scanner.ReadUInt32Number();

        for (uint32_t g=0; g<groupCount; g++) {
          std::string groupName=scanner.ReadString();

          typeInfo->AddGroup(groupName);
        }

        // Descriptions

        uint32_t descriptionCount=scanner.ReadUInt32Number();

        for (uint32_t d=1; d<=descriptionCount; d++) {
          std::string languageCode=scanner.ReadString();
          std::string description=scanner.ReadString();

          typeInfo->AddDescription(languageCode,
                                   description);
        }

        RegisterType(typeInfo);
      }

      scanner.Close();
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }

    timer.Stop();

    log.Debug() << "Opening TypeConfig: " << timer.ResultString();

    return true;
  }

  /**
   * Store the part of the TypeConfig information to a data file,
   * which is necessary to review later on when reading and
   * evaluation an import.
   *
   * @param directory
   *    Directory the data file should be written to
   * @return
   *    True, if there were no errors, else false
   */
  bool TypeConfig::StoreToDataFile(const std::string& directory) const
  {
    FileWriter writer;

    try {
      writer.Open(AppendFileToDir(directory,"types.dat"));

      writer.Write(FILE_FORMAT_VERSION);

      uint32_t typeCount=0;
      uint32_t featureCount=0;

      for (const auto& type: GetTypes()) {
        if (!type->IsInternal()) {
          typeCount++;
        }
      }

      for (const auto& feature : GetFeatures()) {
        if (!feature->GetDescriptions().empty()) {
          featureCount++;
        }
      }

      writer.WriteNumber(featureCount);

      for (const auto& feature : GetFeatures()) {
        if (feature->GetDescriptions().empty()) {
          continue;
        }

        writer.Write(feature->GetName());
        writer.WriteNumber((uint32_t)feature->GetDescriptions().size());
        for (const auto& descriptionEntry : feature->GetDescriptions()) {
          writer.Write(descriptionEntry.first);
          writer.Write(descriptionEntry.second);
        }
      }

      writer.WriteNumber(typeCount);

      for (const auto& type : GetTypes()) {
        if (type->IsInternal()) {
          continue;
        }

        writer.Write(type->GetName());
        writer.Write(type->CanBeNode());
        writer.Write(type->CanBeWay());
        writer.Write(type->CanBeArea());
        writer.Write(type->CanBeRelation());
        writer.Write(type->IsPath());
        writer.Write(type->CanRouteFoot());
        writer.Write(type->CanRouteBicycle());
        writer.Write(type->CanRouteCar());
        writer.Write(type->GetIndexAsAddress());
        writer.Write(type->GetIndexAsLocation());
        writer.Write(type->GetIndexAsRegion());
        writer.Write(type->GetIndexAsPOI());
        writer.Write(type->GetOptimizeLowZoom());
        writer.Write(static_cast<uint8_t>(type->GetSpecialType()));
        writer.Write(type->GetPinWay());
        writer.Write(type->GetMergeAreas());
        writer.Write(type->GetIgnoreSeaLand());
        writer.Write(type->GetIgnore());
        writer.Write(type->GetLanes());
        writer.Write(type->GetOnewayLanes());

        writer.WriteNumber((uint32_t)type->GetFeatures().size());
        for (const auto& feature : type->GetFeatures()) {
          writer.Write(feature.GetFeature()->GetName());
        }

        writer.WriteNumber((uint32_t)type->GetGroups().size());
        for (const auto& groupName : type->GetGroups()) {
          writer.Write(groupName);
        }

        writer.WriteNumber((uint32_t)type->GetDescriptions().size());
        for (const auto& descriptionEntry : type->GetDescriptions()) {
          writer.Write(descriptionEntry.first);
          writer.Write(descriptionEntry.second);
        }
      }

      writer.Close();
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      writer.CloseFailsafe();
      return false;
    }

    return true;
  }
}
