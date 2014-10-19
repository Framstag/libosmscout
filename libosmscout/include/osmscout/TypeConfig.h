#ifndef OSMSCOUT_TYPECONFIG_H
#define OSMSCOUT_TYPECONFIG_H

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

#include <limits>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/ObjectRef.h>
#include <osmscout/Tag.h>
#include <osmscout/Types.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/HashMap.h>
#include <osmscout/util/HashSet.h>
#include <osmscout/util/Progress.h>
#include <osmscout/util/Reference.h>
#include <iostream>
namespace osmscout {

  /**
   * \ingroup type
   *
   * Magic constant for an unresolved and to be ignored object type.
   * Object having typeIgnore as type should be handled like
   * they do not have a type at all.
   */
  static const TypeId typeIgnore      = 0;

  // Forward declaration of classes TypeConfig and TypeInfo because
  // of circular dependency between them and Feature
  class FeatureValueBuffer;
  class FeatureInstance;
  class TypeConfig;
  class TypeInfo;

  class OSMSCOUT_API FeatureValue
  {
  public:
    FeatureValue();
    virtual ~FeatureValue();

    inline virtual std::string GetLabel() const
    {
      return "";
    }

    virtual bool Read(FileScanner& scanner);
    virtual bool Write(FileWriter& writer);

    virtual FeatureValue& operator=(const FeatureValue& other) = 0;
    virtual bool operator==(const FeatureValue& other) const = 0;

    virtual inline bool operator!=(const FeatureValue& other) const
    {
      return !(*this==other);
    }
  };

  /**
   * A feature combines one or multiple tags  to build information attribute for a type.
   *
   * The class "Feature" is the abstract base class for a concrete feature implementation
   * like "NameFeature" or "AccessFeature".
   *
   * A feature could just be an alias for one tag (like "name") but it could also combine
   * a number of attributes (e.g. access and all its variations).
   */
  class OSMSCOUT_API Feature : public Referencable
  {
  public:
    Feature();
    virtual ~Feature();

    /**
     * Does further initialization based on the current TypeConfig. For example
     * it registers Tags (and stores their TagId) for further processing.
     */
    virtual void Initialize(TypeConfig& typeConfig) = 0;

    /**
     * Returns the name of the feature
     */
    virtual std::string GetName() const = 0;

    virtual size_t GetValueSize() const = 0;

    inline virtual bool HasValue() const
    {
      return GetValueSize()>0;
    }

    inline virtual bool HasLabel() const
    {
      return false;
    }

    virtual FeatureValue* AllocateValue(void* buffer);

    virtual void Parse(Progress& progress,
                       const TypeConfig& typeConfig,
                       const FeatureInstance& feature,
                       const ObjectOSMRef& object,
                       const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                       FeatureValueBuffer& buffer) const = 0;
  };

  typedef Ref<Feature> FeatureRef;

  class OSMSCOUT_API FeatureInstance
  {
  private:
    FeatureRef     feature; //<! The feature we are an instance of
    const TypeInfo *type;   //<! The type we are assigned to (we are no Ref type to avoid circular references)
    size_t         index;   //<! The index we have in the list of features
    size_t         offset;  //<! Our offset into the value buffer for our data

  public:
    FeatureInstance(const FeatureRef& feature,
                    const TypeInfo* type,
                    size_t index,
                    size_t offset);

    inline FeatureRef GetFeature() const
    {
      return feature;
    }

    inline const TypeInfo* GetType() const
    {
      return type;
    }

    inline size_t GetIndex() const
    {
      return index;
    }

    inline size_t GetOffset() const
    {
      return offset;
    }
  };

  /**
   * \ingroup type
   *
   *  Detailed information about one object type
   *
   *  \see TypeConfig
   */
  class OSMSCOUT_API TypeInfo : public Referencable
  {
  public:
    static const unsigned char typeNode     = 1 << 0;
    static const unsigned char typeWay      = 1 << 1;
    static const unsigned char typeArea     = 1 << 2;
    static const unsigned char typeRelation = 1 << 3;

  public:
    /**
     * \ingroup type
     *
     * A type can have a number of conditions that allow
     * to identify the type of an object based on its
     * tag values.
     */
    struct TypeCondition
    {
      unsigned char    types;     //<! Bitset of types the condition can be applied to
      TagConditionRef  condition; //<! The root condition
    };

  private:
    TypeId                               nodeId;
    TypeId                               wayId;
    TypeId                               areaId;
    std::string                          name;
    size_t                               index;

    std::list<TypeCondition>             conditions;
    OSMSCOUT_HASHMAP<std::string,size_t> nameToFeatureMap;
    std::vector<FeatureInstance>         features;
    size_t                               valueBufferSize;

    bool                                 canBeNode;
    bool                                 canBeWay;
    bool                                 canBeArea;
    bool                                 canBeRelation;
    bool                                 isPath;
    bool                                 canRouteFoot;
    bool                                 canRouteBicycle;
    bool                                 canRouteCar;
    bool                                 indexAsAddress;
    bool                                 indexAsLocation;
    bool                                 indexAsRegion;
    bool                                 indexAsPOI;
    bool                                 optimizeLowZoom;
    bool                                 multipolygon;
    bool                                 pinWay;
    bool                                 ignoreSeaLand;
    bool                                 ignore;

  private:
    TypeInfo(const TypeInfo& other);

  public:
    TypeInfo();
    virtual ~TypeInfo();

    /**
     * Set the id of this type
     */
    TypeInfo& SetNodeId(TypeId id);

    /**
     * Set the id of this type
     */
    TypeInfo& SetWayId(TypeId id);

    /**
     * Set the id of this type
     */
    TypeInfo& SetAreaId(TypeId id);

    /**
     * Set the index of this type. The index is assured to in the interval [0..GetTypeCount()[
     */
    TypeInfo& SetIndex(size_t index);

    /**
     * The the name of this type
     */
    TypeInfo& SetType(const std::string& name);

    TypeInfo& AddCondition(unsigned char types,
                           TagCondition* condition);

    /**
     * Add a feature to this type
     */
    TypeInfo& AddFeature(const FeatureRef& feature);

    inline bool HasFeatures()
    {
      return !features.empty();
    }

    /**
     * Returns true, if the feature with the given name has already been
     * assigned to this type.
     */
    bool HasFeature(const std::string& featureName) const;

    /**
     * Return the feature with the given name
     */
    bool GetFeature(const std::string& name,
                    size_t& index) const;

    /**
     * Return the feature at the given index
     */
    inline const FeatureInstance& GetFeature(size_t idx) const
    {
      return features[idx];
    }

    /**
     * Return the list of features assigned to this type
     */
    inline const std::vector<FeatureInstance>& GetFeatures() const
    {
      return features;
    }

    /**
     * Returns the number of features of the asisgned type
     */
    inline size_t GetFeatureCount() const
    {
      return features.size();
    }

    /**
     * Returns the (rounded) number of bytes required for storing the feature mask
     */
    inline size_t GetFeatureBytes() const
    {
      size_t size=features.size();

      if (size%8==0) {
        return size/8;
      }
      else {
        return size/8+1;
      }
    }

    /**
     * Returns the size of the buffer required to store all FeatureValues of this type into
     */
    inline size_t GetFeatureValueBufferSize() const
    {
      return valueBufferSize;
    }

    /**
     * Returns the unique id of this type. You should not use the type id as an index.

     */
    inline TypeId GetNodeId() const
    {
      return nodeId;
    }

    /**
     * Returns the unique id of this type. You should not use the type id as an index.

     */
    inline TypeId GetWayId() const
    {
      return wayId;
    }

    /**
     * Returns the unique id of this type. You should not use the type id as an index.

     */
    inline TypeId GetAreaId() const
    {
      return areaId;
    }

    /**
     * Returns the index of this type. The index is assured to in the interval [0..GetTypeCount()[
     */
    inline size_t GetIndex() const
    {
      return index;
    }

    /**
     * The name of the given type
     */
    inline std::string GetName() const
    {
      return name;
    }

    /**
     * Returns true, if there are any conditions bound to the type. If the conditions
     * are met for a given object, the object is in turn of the given type.
     * to
     */
    inline bool HasConditions() const
    {
      return conditions.size()>0;
    }

    /**
     * Returns the list of conditions for the given type.
     */
    inline const std::list<TypeCondition>& GetConditions() const
    {
      return conditions;
    }

    /**
     * If set to 'true', a node can be of this type.
     */
    inline TypeInfo& CanBeNode(bool canBeNode)
    {
      this->canBeNode=canBeNode;

      return *this;
    }

    inline bool CanBeNode() const
    {
      return canBeNode;
    }

    /**
     * If set to 'true', a way can be of this type.
     */
    inline TypeInfo& CanBeWay(bool canBeWay)
    {
      this->canBeWay=canBeWay;

      return *this;
    }

    inline bool CanBeWay() const
    {
      return canBeWay;
    }

    /**
     * If set to 'true', an area can be of this type.
     */
    inline TypeInfo& CanBeArea(bool canBeArea)
    {
      this->canBeArea=canBeArea;

      return *this;
    }

    inline bool CanBeArea() const
    {
      return canBeArea;
    }

    /**
     * If set to 'true', a relation can be of this type.
     */
    inline TypeInfo& CanBeRelation(bool canBeRelation)
    {
      this->canBeRelation=canBeRelation;

      return *this;
    }

    inline bool CanBeRelation() const
    {
      return canBeRelation;
    }

    /**
     * If set to 'true', a node can be of this type.
     */
    inline TypeInfo& SetIsPath(bool isPath)
    {
      this->isPath=isPath;

      return *this;
    }

    inline bool IsPath() const
    {
      return isPath;
    }

    /**
     * If set to 'true', an object of this type can be traveled by feet by default.
     */
    inline TypeInfo& CanRouteFoot(bool canBeRoute)
    {
      this->canRouteFoot=canBeRoute;

      return *this;
    }

    inline TypeInfo& CanRouteBicycle(bool canBeRoute)
    {
      this->canRouteBicycle=canBeRoute;

      return *this;
    }

    /**
     * If set to 'true', an object of this type can be traveled by car by default.
     */
    inline TypeInfo& CanRouteCar(bool canBeRoute)
    {
      this->canRouteCar=canBeRoute;

      return *this;
    }

    inline bool CanRoute() const
    {
      return canRouteFoot || canRouteBicycle || canRouteCar;
    }

    /**
     * If set to 'true', an object of this type can be traveled by the given vehicle by default.
     */
    inline bool CanRoute(Vehicle vehicle) const
    {
      switch (vehicle)
      {
      case vehicleFoot:
        return canRouteFoot;
      case vehicleBicycle:
        return canRouteBicycle;
      case vehicleCar:
        return canRouteCar;
      }

      return false;
    }

    inline bool CanRouteFoot() const
    {
      return canRouteFoot;
    }

    inline bool CanRouteBicycle() const
    {
      return canRouteBicycle;
    }

    inline bool CanRouteCar() const
    {
      return canRouteCar;
    }

    uint8_t GetDefaultAccess() const;

    /**
     * Set, if an object of this type should be indexed as an address.
     */
    inline TypeInfo& SetIndexAsAddress(bool indexAsAddress)
    {
      this->indexAsAddress=indexAsAddress;

      return *this;
    }

    inline bool GetIndexAsAddress() const
    {
      return indexAsAddress;
    }

    /**
     * Set, if an object of this type should be indexed as a location.
     */
    inline TypeInfo& SetIndexAsLocation(bool indexAsLocation)
    {
      this->indexAsLocation=indexAsLocation;

      return *this;
    }

    inline bool GetIndexAsLocation() const
    {
      return indexAsLocation;
    }

    /**
     * Set, if an object of this type should be indexed as a region.
     */
    inline TypeInfo& SetIndexAsRegion(bool indexAsRegion)
    {
      this->indexAsRegion=indexAsRegion;

      return *this;
    }

    inline bool GetIndexAsRegion() const
    {
      return indexAsRegion;
    }

    /**
     * Set, if an object of this type should be indexed as a POI.
     */
    inline TypeInfo& SetIndexAsPOI(bool indexAsPOI)
    {
      this->indexAsPOI=indexAsPOI;

      return *this;
    }

    inline bool GetIndexAsPOI() const
    {
      return indexAsPOI;
    }

    /**
     * Set, if an object of this type should be optimized for low zoom.
     */
    inline TypeInfo& SetOptimizeLowZoom(bool optimize)
    {
      this->optimizeLowZoom=optimize;

      return *this;
    }

    inline bool GetOptimizeLowZoom() const
    {
      return optimizeLowZoom;
    }

    /**
     * If set to 'true', an object is handled as multipolygon even though it may not have
     * type=multipolygon set explicitly.
     */
    inline TypeInfo& SetMultipolygon(bool multipolygon)
    {
      this->multipolygon=multipolygon;

      return *this;
    }

    inline bool GetMultipolygon() const
    {
      return multipolygon;
    }

    inline TypeInfo& SetPinWay(bool pinWay)
    {
      this->pinWay=pinWay;

      return *this;
    }

    inline bool GetPinWay() const
    {
      return pinWay;
    }

    /**
     * Set, if an object of this type should be ignored for land/sea calculation.
     */
    inline TypeInfo& SetIgnoreSeaLand(bool ignoreSeaLand)
    {
      this->ignoreSeaLand=ignoreSeaLand;

      return *this;
    }

    inline bool GetIgnoreSeaLand() const
    {
      return ignoreSeaLand;
    }

    /**
     * If set to true, an object of this typoe should be ignored (not exported for renderng, routing,
     * location indexing or other services).
     */
    inline TypeInfo& SetIgnore(bool ignore)
    {
      this->ignore=ignore;

      return *this;
    }

    inline bool GetIgnore() const
    {
      return ignore;
    }
  };

  typedef Ref<TypeInfo> TypeInfoRef;

  class OSMSCOUT_API TypeInfoSetConstIterator : public std::iterator<std::input_iterator_tag, const TypeInfoRef>
  {
  private:
    std::vector<TypeInfoRef>::const_iterator iterCurrent;
    std::vector<TypeInfoRef>::const_iterator iterEnd;

  public:
    TypeInfoSetConstIterator(const std::vector<TypeInfoRef>::const_iterator& iterCurrent,
                             const std::vector<TypeInfoRef>::const_iterator& iterEnd)
    : iterCurrent(iterCurrent),
      iterEnd(iterEnd)
    {
      while (this->iterCurrent!=this->iterEnd &&
            this->iterCurrent->Invalid()) {
        ++this->iterCurrent;
      }
    }

    TypeInfoSetConstIterator(const TypeInfoSetConstIterator& other)
    : iterCurrent(other.iterCurrent),
      iterEnd(other.iterEnd)
    {
      // no code
    }

    TypeInfoSetConstIterator& operator++()
    {
      ++iterCurrent;

      while (iterCurrent!=iterEnd &&
            iterCurrent->Invalid()) {
        ++iterCurrent;
      }

      return *this;
    }
    TypeInfoSetConstIterator operator++(int)
     {
      TypeInfoSetConstIterator tmp(*this);

      operator++();

      return tmp;
     }

    bool operator==(const TypeInfoSetConstIterator& other)
    {
      return iterCurrent==other.iterCurrent;
    }

    bool operator!=(const TypeInfoSetConstIterator& other)
    {
      return iterCurrent!=other.iterCurrent;
    }

    const TypeInfoRef& operator*()
    {
      return *iterCurrent;
    }
  };

  class OSMSCOUT_API TypeInfoSet
  {
  private:
    std::vector<TypeInfoRef> types;
    size_t                   count;

  public:
    TypeInfoSet();
    TypeInfoSet(const TypeConfig& typeConfig);
    TypeInfoSet(const TypeInfoSet& other);
    TypeInfoSet(const std::vector<TypeInfoRef>& types);

    void Adapt(const TypeConfig& typeConfig);

    void Clear()
    {
      types.clear();
      count=0;
    }

    void Set(const TypeInfoRef& type);
    void Set(const std::vector<TypeInfoRef>& types);

    void Remove(const TypeInfoRef& type);
    void Remove(const TypeInfoSet& otherTypes);

    bool IsSet(const TypeInfoRef& type) const
    {
      assert(type.Valid());

      return type->GetIndex()<types.size() &&
             types[type->GetIndex()].Valid();
    }

    inline bool Empty() const
    {
      return count==0;
    }

    inline size_t Size() const
    {
      return count;
    }

    TypeInfoSet& operator=(const TypeInfoSet& other)
    {
      if (&other!=this) {
        this->types=other.types;
        this->count=other.count;
      }

      return *this;
    }

    inline TypeInfoSetConstIterator begin() const
    {
      return TypeInfoSetConstIterator(types.begin(),
                                      types.end());
    }

    inline TypeInfoSetConstIterator end() const
    {
      return TypeInfoSetConstIterator(types.end(),
                                      types.end());
    }
  };

  class OSMSCOUT_API FeatureValueBuffer
  {
  private:
    TypeInfoRef type;
    uint8_t     *featureBits;
    char        *featureValueBuffer;

  private:
    void DeleteData();
    void AllocateData();

  public:
    FeatureValueBuffer();
    FeatureValueBuffer(const FeatureValueBuffer& other);
    virtual ~FeatureValueBuffer();

    void Set(const FeatureValueBuffer& other);

    void SetType(const TypeInfoRef& type);

    inline TypeInfoRef GetType() const
    {
      return type;
    }

    inline size_t GetFeatureCount() const
    {
      return type->GetFeatureCount();
    }

    inline FeatureInstance GetFeature(size_t idx) const
    {
      return type->GetFeature(idx);
    }

    inline bool HasValue(size_t idx) const
    {
      return featureBits[idx/8] & (1 << idx%8);
    }

    inline FeatureValue* GetValue(size_t idx) const
    {
      return static_cast<FeatureValue*>(static_cast<void*>(&featureValueBuffer[type->GetFeature(idx).GetOffset()]));
    }

    FeatureValue* AllocateValue(size_t idx);
    void FreeValue(size_t idx);

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const ObjectOSMRef& object,
               const OSMSCOUT_HASHMAP<TagId,std::string>& tags);

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;

    FeatureValueBuffer& operator=(const FeatureValueBuffer& other);
    bool operator==(const FeatureValueBuffer& other) const;
    bool operator!=(const FeatureValueBuffer& other) const;
  };

  /**
   * \ingroup type
   *
   * The TypeConfig class holds information about object types
   * defined by a database instance.
   */
  class OSMSCOUT_API TypeConfig : public Referencable
  {
  private:
    std::vector<TagInfo>                      tags;
    std::vector<TypeInfoRef>                  types;
    std::vector<TypeInfoRef>                  nodeTypes;
    std::vector<TypeInfoRef>                  wayTypes;
    std::vector<TypeInfoRef>                  areaTypes;
    std::vector<FeatureRef>                   features;

    TagId                                     nextTagId;

    OSMSCOUT_HASHMAP<std::string,TagId>       stringToTagMap;
    OSMSCOUT_HASHMAP<std::string,TypeInfoRef> nameToTypeMap;
    OSMSCOUT_HASHMAP<TagId,uint32_t>          nameTagIdToPrioMap;
    OSMSCOUT_HASHMAP<TagId,uint32_t>          nameAltTagIdToPrioMap;
    OSMSCOUT_HASHMAP<std::string,uint8_t>     nameToMaxSpeedMap;

    OSMSCOUT_HASHMAP<std::string,size_t>      surfaceToGradeMap;

    OSMSCOUT_HASHMAP<std::string,FeatureRef>  nameToFeatureMap;

    FeatureRef                                featureName;
    FeatureRef                                featureRef;
    FeatureRef                                featureLocation;
    FeatureRef                                featureAddress;
    FeatureRef                                featureAccess;
    FeatureRef                                featureLayer;
    FeatureRef                                featureWidth;
    FeatureRef                                featureMaxSpeed;
    FeatureRef                                featureGrade;
    FeatureRef                                featureBridge;
    FeatureRef                                featureTunnel;
    FeatureRef                                featureRoundabout;

  public:
    TypeInfoRef                               typeInfoIgnore;

    TypeInfoRef                               typeInfoTileLand;
    TypeInfoRef                               typeInfoTileSea;
    TypeInfoRef                               typeInfoTileCoast;
    TypeInfoRef                               typeInfoTileUnknown;
    TypeInfoRef                               typeInfoTileCoastline;

    // Internal use (only available during preprocessing)
    TagId                                     tagArea;
    TagId                                     tagNatural;
    TagId                                     tagType;
    TagId                                     tagRestriction;

  public:
    TypeConfig();
    virtual ~TypeConfig();

    /**
     * Methods for dealing with tags
     */
    //@{
    TagId RegisterTag(const std::string& tagName);

    TagId RegisterNameTag(const std::string& tagName,
                          uint32_t priority);
    TagId RegisterNameAltTag(const std::string& tagName,
                             uint32_t priority);

    TagId GetTagId(const char* name) const;

    bool IsNameTag(TagId tag,
                   uint32_t& priority) const;
    bool IsNameAltTag(TagId tag,
                      uint32_t& priority) const;
    //@}

    /**
     * Methods for dealing with features. A feature is a attribute set based on parsed tags.
     * Features can get assigned to a type.
     */
    //@{
    void RegisterFeature(const FeatureRef& feature);

    /**
     * Return the feature with the given name or an invalid reference
     * if no feature with the given name is registered.
     */
    FeatureRef GetFeature(const std::string& name) const;

    /**
     * Return all features registered
     */
    inline const std::vector<FeatureRef>& GetFeatures() const
    {
      return features;
    }
    //@}

    /**
     * Methods for dealing with types.
     */
    //@{
    TypeInfoRef RegisterType(const TypeInfoRef& typeInfo);

    /**
     * Return an array of the types available
     */
    inline const std::vector<TypeInfoRef>& GetTypes() const
    {
      return types;
    }

    /**
     * Returns an array of the (ignore=false) node types available
     */
    inline const std::vector<TypeInfoRef>& GetNodeTypes() const
    {
      return nodeTypes;
    }

    /**
     * Returns an array of (ignore=false) the way types available
     */
    inline const std::vector<TypeInfoRef>& GetWayTypes() const
    {
      return wayTypes;
    }

    /**
     * Returns an array of the (ignore=false) area types available
     */
    inline const std::vector<TypeInfoRef>& GetAreaTypes() const
    {
      return areaTypes;
    }

    /**
     * Returns the number of types available. The index of a type is guaranteed to be in the interval
     * [0..GetTypeCount()[
     */
    inline size_t GetTypeCount() const
    {
      return types.size();
    }

    /**
     * Return the highest used type id.
     */
    TypeId GetMaxTypeId() const;

    /**
     * Returns the type definition for the given type id
     */
    inline const TypeInfoRef GetTypeInfo(size_t index) const
    {
      assert(index<types.size());

      return types[index];
    }

    /**
     * Returns the type definition for the given type id
     */
    inline const TypeInfoRef GetNodeTypeInfo(TypeId id) const
    {
      assert(id<=nodeTypes.size());

      if (id==typeIgnore) {
        return typeInfoIgnore;
      }
      else {
        return nodeTypes[id-1];
      }
    }

    /**
     * Returns the type definition for the given type id
     */
    inline const TypeInfoRef GetWayTypeInfo(TypeId id) const
    {
      assert(id<=wayTypes.size());

      if (id==typeIgnore) {
        return typeInfoIgnore;
      }
      else {
        return wayTypes[id-1];
      }
    }

    /**
     * Returns the type definition for the given type id
     */
    inline const TypeInfoRef GetAreaTypeInfo(TypeId id) const
    {
      if (!(id<=areaTypes.size())) {
        std::cout << "assert(" << id << "<" << areaTypes.size() << ")" << std::endl;
      }

      assert(id<=areaTypes.size());

      if (id==typeIgnore) {
        return typeInfoIgnore;
      }
      else {
        return areaTypes[id-1];
      }
    }

    /**
     * Returns the type definition for the given type name. If there is no
     * type definition for the given name and invalid reference is returned.
     */
    const TypeInfoRef GetTypeInfo(const std::string& name) const;

    /**
     * Return a node type (or an invalid reference if no type got detected)
     * based on the given map of tag and tag values. The method iterates over all
     * node type definitions, evaluates their conditions and returns the first matching
     * type.
     */
    TypeInfoRef GetNodeType(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const;

    /**
     * Return a way/area type (or an invalid reference if no type got detected)
     * based on the given map of tag and tag values. The method iterates over all
     * way/area type definitions, evaluates their conditions and returns the first matching
     * type.
     */
    bool GetWayAreaType(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap,
                        TypeInfoRef& wayType,
                        TypeInfoRef& areaType) const;

    /**
     * Return a relation type (or an invalid reference if no type got detected)
     * based on the given map of tag and tag values. The method iterates over all
     * relation type definitions, evaluates their conditions and returns the first matching
     * type.
     */
    TypeInfoRef GetRelationType(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const;
    //@}

    /**
     * Methods for dealing with mappings for surfaces and surface grades.
     */
    //@{
    void RegisterSurfaceToGradeMapping(const std::string& surface,
                                       size_t grade);
    bool GetGradeForSurface(const std::string& surface,
                            size_t& grade) const;
    //@}

    /**
     * Methods for dealing with mappings for surfaces and surface grades.
     */
    //@{
    void RegisterMaxSpeedAlias(const std::string& alias,
                               uint8_t maxSpeed);
    bool GetMaxSpeedFromAlias(const std::string& alias,
                              uint8_t& maxSpeed) const;
    //@}


    /**
     * Methods for loading/storing of type information from/to files.
     */
    //@{
    bool LoadFromOSTFile(const std::string& filename);
    bool LoadFromDataFile(const std::string& directory);
    bool StoreToDataFile(const std::string& directory) const;
    //@}
  };


  //! \ingroup type
  //! Reference counted reference to a TypeConfig instance
  typedef Ref<TypeConfig> TypeConfigRef;

  /**
   * \defgroup type Object type related data structures and services
   */
}

#endif
