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
#include <memory>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/ObjectRef.h>
#include <osmscout/Tag.h>
#include <osmscout/Types.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Progress.h>
#include <osmscout/util/TagErrorReporter.h>

#include <osmscout/system/Assert.h>

#include <osmscout/system/Compiler.h>

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
    virtual ~FeatureValue() = default;

    inline virtual std::string GetLabel() const
    {
      return "";
    }

    virtual void Read(FileScanner& scanner);
    virtual void Write(FileWriter& writer);

    virtual FeatureValue& operator=(const FeatureValue& other);
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
  class OSMSCOUT_API Feature
  {
  private:
    std::unordered_map<std::string,size_t>      labels;
    std::unordered_map<std::string,std::string> descriptions; //!< Map of descriptions for given language codes

  protected:
    size_t RegisterLabel(const std::string& labelName,
                         size_t index);

  public:
    Feature();
    virtual ~Feature() = default;

    /**
     * Does further initialization based on the current TypeConfig. For example
     * it registers Tags (and stores their TagId) for further processing.
     */
    virtual void Initialize(TypeConfig& typeConfig) = 0;

    void AddDescription(const std::string& languageCode,
                        const std::string& description);

    /**
     * Returns the name of the feature
     */
    virtual std::string GetName() const = 0;

    /**
     * A feature, if set for an object, can hold a value. If there is no value object,
     * this method returns 0, else it returns the C++ size of the value object.
     */
    inline virtual size_t GetValueSize() const
    {
      return 0;
    }

    /**
     * This method returns the number of additional feature bits reserved. If there are
     * additional features bit, 0 is returned.
     *
     * A feature may reserve additional feature bits. Feature bits should be used
     * if a custom value object is too expensive. Space for feature bits is always reserved
     * even if the feature itself is not set for a certain object.
     */
    inline virtual size_t GetFeatureBitCount() const
    {
      return 0;
    }

    /**
     * Returns 'true' if the feature has an value object.
     */
    inline virtual bool HasValue() const
    {
      return GetValueSize()>0;
    }

    /**
     * Returns 'true' if the feature provides labels.
     */
    inline virtual bool HasLabel() const
    {
      return !labels.empty();
    }

    /**
     * Returns the index of the label with the given name. Method returns 'true'
     * if the feature has labels and a label with the given name exists. Else
     * 'false' is returned.
     */
    bool GetLabelIndex(const std::string& labelName,
                       size_t& index) const;

    std::string GetDescription(const std::string& languageCode) const;

    inline const std::unordered_map<std::string,std::string>& GetDescriptions() const
    {
      return descriptions;
    };

    virtual FeatureValue* AllocateValue(void* buffer);

    virtual void Parse(TagErrorReporter& reporter,
                       const TypeConfig& typeConfig,
                       const FeatureInstance& feature,
                       const ObjectOSMRef& object,
                       const TagMap& tags,
                       FeatureValueBuffer& buffer) const = 0;
  };

  typedef std::shared_ptr<Feature> FeatureRef;

  /**
   * An instantiation of a feature for a certain type.
   */
  class OSMSCOUT_API FeatureInstance CLASS_FINAL
  {
  private:
    FeatureRef     feature;    //!< The feature we are an instance of
    const TypeInfo *type;      //!< The type we are assigned to (we are no Ref type to avoid circular references)
    size_t         featureBit; //!< index of the bit that signals that the feature is available
    size_t         index;      //!< The index we have in the list of features
    size_t         offset;     //!< Our offset into the value buffer for our data

  public:
    FeatureInstance();
    FeatureInstance(const FeatureRef& feature,
                    const TypeInfo* type,
                    size_t featureBit,
                    size_t index,
                    size_t offset);

    /**
     * Return the feature itself.
     */
    inline FeatureRef GetFeature() const
    {
      return feature;
    }

    /**
     * Return a pointer back tot he type we are assigned to.
     */
    inline const TypeInfo* GetType() const
    {
      return type;
    }

    /**
     * return the index of this feature within the list of features of the type.
     */
    inline size_t GetFeatureBit() const
    {
      return featureBit;
    }

    /**
     * return the index of this feature within the list of features of the type.
     */
    inline size_t GetIndex() const
    {
      return index;
    }

    /**
     * Return the file offset within the feature value buffer for the value of this feature.
     */
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
  class OSMSCOUT_API TypeInfo CLASS_FINAL
  {
  public:
    static const unsigned char typeNode     = 1 << 0; //!< Condition applies to nodes
    static const unsigned char typeWay      = 1 << 1; //!< Condition applies to ways
    static const unsigned char typeArea     = 1 << 2; //!< Condition applies to areas
    static const unsigned char typeRelation = 1 << 3; //!< Condition applies to releations

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
      unsigned char    types;     //!< Bitset of types the condition can be applied to
      TagConditionRef  condition; //!< The root condition
    };

  private:
    TypeId                                      nodeId;                  //!< Type if in case the object is a node
    TypeId                                      wayId;                   //!< Type if in case the object is a way
    TypeId                                      areaId;                  //!< Type if in case the object is a area
    std::string                                 name;                    //!< Name of the type
    size_t                                      index;                   //!< Internal unique index of the type

    bool                                        internal;                //!< This typ eis only internally used, there is no OSM date for this type

    std::list<TypeCondition>                    conditions;              //!< One of this conditions must be fulfilled for a object to match this type
    std::unordered_map<std::string,size_t>      nameToFeatureMap;
    std::vector<FeatureInstance>                features;                //!< List of feature this type has
    size_t                                      featureMaskBytes;        //!< Size of the feature bitmask in bytes
    size_t                                      specialFeatureMaskBytes; //!< Size of the feature bitmask in bytes
    size_t                                      valueBufferSize;         //!< Size of the value buffer holding values for all feature of the type

    bool                                        canBeNode;               //!< Type can be a node
    bool                                        canBeWay;                //!< Type can be a way
    bool                                        canBeArea;               //!< Type can be a area
    bool                                        canBeRelation;
    bool                                        isPath;                  //!< Type has path characteristics (features like bridges, tunnels, names,...)
    bool                                        canRouteFoot;            //!< Object of this type are by default routable for foot
    bool                                        canRouteBicycle;         //!< Object of this type are by default routable for bicylce
    bool                                        canRouteCar;             //!< Object of this type are by default routable for car
    bool                                        indexAsAddress;          //!< Objects of this type are addressable
    bool                                        indexAsLocation;         //!< Objects of this type are defining a location (e.g. street)
    bool                                        indexAsRegion;           //!< Objects of this type are defining a administrative region (e.g. city, county,...)
    bool                                        indexAsPOI;              //!< Objects of this type are defining a POI
    bool                                        optimizeLowZoom;         //!< Optimize objects of this type for low zoom rendering
    bool                                        multipolygon;
    bool                                        pinWay;                  //!< If there is no way/area information treat this object as way even it the way is closed
    bool                                        mergeAreas;              //!< Areas of this type are merged under certain conditions
    bool                                        ignoreSeaLand;           //!< Ignore objects of this type for sea/land calculation
    bool                                        ignore;                  //!< Ignore objects of this type

    std::unordered_set<std::string>             groups;                  //!< Set of idents that server as categorizing groups
    std::unordered_map<std::string,std::string> descriptions;            //!< Map of descriptions for given language codes

  private:
    TypeInfo(const TypeInfo& other);

  public:
    TypeInfo(const std::string& name);

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
     * Mark this type as internal
     */
    TypeInfo& SetInternal();

    /**
     * The the name of this type
     */
    TypeInfo& SetType(const std::string& name);

    TypeInfo& AddCondition(unsigned char types,
                           const TagConditionRef& condition);

    /**
     * Add a feature to this type
     */
    TypeInfo& AddFeature(const FeatureRef& feature);

    /**
     * Add a categorizing group name to the type.
     */
    TypeInfo& AddGroup(const std::string& groupName);

    TypeInfo& AddDescription(const std::string& languageCode,
                             const std::string& description);

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
    inline size_t GetFeatureMaskBytes() const
    {
      return featureMaskBytes;
    }

    /**
     * Returns the (rounded) number of bytes required for storing the feature mask and one additional
     * general purpose signal byte.
     */
    inline size_t GetSpecialFeatureMaskBytes() const
    {
      return specialFeatureMaskBytes;
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
     * Return true, if this is a internal type, else false
     */
    inline bool IsInternal() const
    {
      return internal;
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
     * Set to true, if "touching" areas of this type should get merged.
     */
    inline TypeInfo& SetMergeAreas(bool mergeAreas)
    {
      this->mergeAreas=mergeAreas;

      return *this;
    }

    inline bool GetMergeAreas() const
    {
      return mergeAreas;
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

    /**
     * Return the set of groups the type is in.
     */
    inline const std::unordered_set<std::string>& GetGroups() const
    {
      return groups;
    }

    inline bool IsInGroup(const std::string& groupName) const
    {
      return groups.find(groupName)!=groups.end();
    }

    inline const std::unordered_map<std::string,std::string>& GetDescriptions() const
    {
      return descriptions;
    };

    std::string GetDescription(const std::string& languageCode) const;
  };

  typedef std::shared_ptr<TypeInfo> TypeInfoRef;

  class OSMSCOUT_API TypeInfoSetConstIterator CLASS_FINAL : public std::iterator<std::input_iterator_tag, const TypeInfoRef>
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
            !*this->iterCurrent) {
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
            !*iterCurrent) {
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

  /**
   * Custom data structure to efficiently handle a set of TypeInfoRef.
   *
   * All operations on the set are O(1) using the fact, that TypeInfo internally
   * have a continuously running index variable (Set may be slower if the
   * internal array was not preinitialized to it maximum size by passing a
   * TypeConfig or another TypeInfoSet in the constructor.
   */
  class OSMSCOUT_API TypeInfoSet CLASS_FINAL
  {
  private:
    std::vector<TypeInfoRef> types;
    size_t                   count;

  public:
    TypeInfoSet();
    TypeInfoSet(const TypeConfig& typeConfig);
    TypeInfoSet(const TypeInfoSet& other);
    TypeInfoSet(TypeInfoSet&& other);
    TypeInfoSet(const std::vector<TypeInfoRef>& types);

    void Adapt(const TypeConfig& typeConfig);

    inline void Clear()
    {
      if (count>0) {
        types.clear();
      }

      count=0;
    }

    void Set(const TypeInfoRef& type);
    void Set(const std::vector<TypeInfoRef>& types);
    void Set(const TypeInfoSet& other);

    void Add(const TypeInfoSet& types);

    void Remove(const TypeInfoRef& type);
    void Remove(const TypeInfoSet& otherTypes);

    void Intersection(const TypeInfoSet& otherTypes);

    inline bool IsSet(const TypeInfoRef& type) const
    {
      assert(type);

      return type->GetIndex()<types.size() &&
             types[type->GetIndex()];
    }

    inline bool Empty() const
    {
      return count==0;
    }

    inline size_t Size() const
    {
      return count;
    }

    bool Intersects(const TypeInfoSet& otherTypes) const;

    inline TypeInfoSet& operator=(const TypeInfoSet& other)
    {
      if (&other!=this) {
        this->types=other.types;
        this->count=other.count;
      }

      return *this;
    }

    bool operator==(const TypeInfoSet& other) const;
    bool operator!=(const TypeInfoSet& other) const;

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

  /**
   * A FeatureValueBuffer is instantiated by an object and holds information
   * about the type of the object, the features and feature values available for the given object.
   */
  class OSMSCOUT_API FeatureValueBuffer CLASS_FINAL
  {
  private:
    TypeInfoRef type;
    uint8_t     *featureBits;
    char        *featureValueBuffer;

  private:
    void DeleteData();
    void AllocateBits();
    void AllocateValueBufferLazy();

    /**
     * Return a raw pointer to the value (as reserved in the internal featureValueBuffer). If the
     * featureValueBuffer doe snot yet exist, it will be created lazy.
     */
    inline FeatureValue* GetValueAndAllocateBuffer(size_t idx)
    {
      AllocateValueBufferLazy();
      return static_cast<FeatureValue*>(static_cast<void*>(&featureValueBuffer[type->GetFeature(idx).GetOffset()]));
    }

  public:
    FeatureValueBuffer();
    FeatureValueBuffer(const FeatureValueBuffer& other);
    ~FeatureValueBuffer();

    /**
     * Deletes the current feature values and assign the type and values
     * of the passed featur evalue buffer.
     *
     * @param other
     *    Other FeatureValueBuffer to make a copy of
     */
    void Set(const FeatureValueBuffer& other);

    /**
     * Maintains the current values and types and copies over
     * all not already set feature values of the passed instance to the current instance.
     *
     * Both FeatureValueBuffers do not have to have the same type. Copying is a based on
     * best effort. The copy opperation is expensive.
     *
     * @param other
     *    instance to copy missing values from.
     *
     * @note Untested!
     *
     */
    void CopyMissingValues(const FeatureValueBuffer& other);

    /**
     * Clears all feature buffer values
     */
    void ClearFeatureValues();

    void SetType(const TypeInfoRef& type);

    inline TypeInfoRef GetType() const
    {
      return type;
    }

    /**
     * Return the numbe rof features defined for this type
     */
    inline size_t GetFeatureCount() const
    {
      return type->GetFeatureCount();
    }

    /**
     * Get a feature description for the feature with the given index ([0..featureCount[)
     */
    inline FeatureInstance GetFeature(size_t idx) const
    {
      return type->GetFeature(idx);
    }

    /**
     * Return true, if the given feature is set (available), else false.
     */
    inline bool HasFeature(size_t idx) const
    {
      size_t featureBit=type->GetFeature(idx).GetFeatureBit();

      return (featureBits[featureBit/8] & (1 << featureBit%8))!=0;
    }

    /**
     * Return a raw pointer to the value (as reserved in the internal featureValueBuffer)
     *
     * Note:
     * Can return NULL value!
     * HasFeature(idx) && GetFeature(idx).GetFeature()->HasValue()
     *  should be called before accessing the value.
     */
    inline FeatureValue* GetValue(size_t idx) const
    {
      if (featureValueBuffer == NULL)
        return NULL;
      return static_cast<FeatureValue*>(static_cast<void*>(&featureValueBuffer[type->GetFeature(idx).GetOffset()]));
    }

    FeatureValue* AllocateValue(size_t idx);
    void FreeValue(size_t idx);

    void Parse(TagErrorReporter& errorReporter,
               const TypeConfig& typeConfig,
               const ObjectOSMRef& object,
               const TagMap& tags);

    void Read(FileScanner& scanner);
    void Read(FileScanner& scanner,
              bool& specialFlag);
    void Read(FileScanner& scanner,
              bool& specialFlag1,
              bool& specialFlag2);
    void Write(FileWriter& writer) const;
    void Write(FileWriter& writer,
               bool specialFlag) const;
    void Write(FileWriter& writer,
               bool specialFlag1,
               bool specialFlag2) const;

    FeatureValueBuffer& operator=(const FeatureValueBuffer& other);
    bool operator==(const FeatureValueBuffer& other) const;
    bool operator!=(const FeatureValueBuffer& other) const;

    template<class T> const T* findValue() const
    {
      for (auto &featureInstance :GetType()->GetFeatures()){
          if (HasFeature(featureInstance.GetIndex())){
            osmscout::FeatureRef feature=featureInstance.GetFeature();
            if (feature->HasValue()){
              osmscout::FeatureValue *value=GetValue(featureInstance.GetIndex());
              const T *v = dynamic_cast<const T*>(value);
              if (v!=NULL){
                return v;
              }
            }
          }
      }
      return NULL;
    }
  };

  typedef std::shared_ptr<FeatureValueBuffer> FeatureValueBufferRef;

  static const uint32_t FILE_FORMAT_VERSION=15;

  /**
   * \ingroup type
   *
   * The TypeConfig class holds information about object types
   * defined by a database instance.
   */
  class OSMSCOUT_API TypeConfig CLASS_FINAL
  {
  public:
    static const char* FILE_TYPES_DAT;
    static const uint32_t MIN_FORMAT_VERSION = FILE_FORMAT_VERSION;
    static const uint32_t MAX_FORMAT_VERSION = FILE_FORMAT_VERSION;

  private:

    // Tags

    std::vector<TagInfo>                        tags;

    TagId                                       nextTagId;

    std::unordered_map<std::string,TagId>       stringToTagMap;
    std::unordered_map<TagId,uint32_t>          nameTagIdToPrioMap;
    std::unordered_map<TagId,uint32_t>          nameAltTagIdToPrioMap;
    std::unordered_map<std::string,uint8_t>     nameToMaxSpeedMap;

    std::unordered_map<std::string,size_t>      surfaceToGradeMap;

    // Types

    std::vector<TypeInfoRef>                    types;
    std::vector<TypeInfoRef>                    nodeTypes;
    std::vector<TypeInfoRef>                    wayTypes;
    std::vector<TypeInfoRef>                    areaTypes;

    uint8_t                                     nodeTypeIdBytes;
    uint8_t                                     wayTypeIdBytes;
    uint8_t                                     areaTypeIdBits;
    uint8_t                                     areaTypeIdBytes;

    std::unordered_map<std::string,TypeInfoRef> nameToTypeMap;

    // Features

    std::vector<FeatureRef>                     features;

    std::unordered_map<std::string,FeatureRef>  nameToFeatureMap;

    FeatureRef                                  featureName;
    FeatureRef                                  featureRef;
    FeatureRef                                  featureLocation;
    FeatureRef                                  featureAddress;
    FeatureRef                                  featurePostalCode;
    FeatureRef                                  featureWebsite;
    FeatureRef                                  featurePhone;
    FeatureRef                                  featureAccess;
    FeatureRef                                  featureAccessRestricted;
    FeatureRef                                  featureLayer;
    FeatureRef                                  featureWidth;
    FeatureRef                                  featureMaxSpeed;
    FeatureRef                                  featureGrade;
    FeatureRef                                  featureBridge;
    FeatureRef                                  featureTunnel;
    FeatureRef                                  featureEmbankment;
    FeatureRef                                  featureRoundabout;

  public:
    // Internal use (only available during preprocessing)
    TagId                                       tagArea;
    TagId                                       tagNatural;
    TagId                                       tagDataPolygon;
    TagId                                       tagType;
    TagId                                       tagRestriction;
    TagId                                       tagJunction;

    TypeInfoRef                                 typeInfoIgnore;
    TypeInfoRef                                 typeInfoTileLand;         //!< Internal type for ground tiles of type "land"
    TypeInfoRef                                 typeInfoTileSea;          //!< Internal type for ground tiles of type "sea"
    TypeInfoRef                                 typeInfoTileCoast;        //!< Internal type for ground tiles of type "coast"
    TypeInfoRef                                 typeInfoTileUnknown;      //!< Internal type for ground tiles of type "unknown"
    TypeInfoRef                                 typeInfoCoastline;        //!< Internal type for coastlines
    TypeInfoRef                                 typeInfoOSMTileBorder;    //!< Internal type for OSM tile borders
    TypeInfoRef                                 typeInfoOSMSubTileBorder; //!< Internal type for OSM tile borders

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
    TagId GetTagId(const std::string& name) const;

    bool IsNameTag(TagId tag,
                   uint32_t& priority) const;
    bool IsNameAltTag(TagId tag,
                      uint32_t& priority) const;
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

    inline uint8_t GetNodeTypeIdBytes() const
    {
      return nodeTypeIdBytes;
    }

    /**
     * Returns an array of (ignore=false) the way types available
     */
    inline const std::vector<TypeInfoRef>& GetWayTypes() const
    {
      return wayTypes;
    }

    inline uint8_t GetWayTypeIdBytes() const
    {
      return wayTypeIdBytes;
    }

    /**
     * Returns an array of the (ignore=false) area types available
     */
    inline const std::vector<TypeInfoRef>& GetAreaTypes() const
    {
      return areaTypes;
    }

    inline uint8_t GetAreaTypeIdBits() const
    {
      return areaTypeIdBits;
    }

    inline uint8_t GetAreaTypeIdBytes() const
    {
      return areaTypeIdBytes;
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
    TypeInfoRef GetNodeType(const TagMap& tagMap) const;

    /**
     * Return a way/area type (or an invalid reference if no type got detected)
     * based on the given map of tag and tag values. The method iterates over all
     * way/area type definitions, evaluates their conditions and returns the first matching
     * type.
     */
    bool GetWayAreaType(const TagMap& tagMap,
                        TypeInfoRef& wayType,
                        TypeInfoRef& areaType) const;

    /**
     * Return a relation type (or an invalid reference if no type got detected)
     * based on the given map of tag and tag values. The method iterates over all
     * relation type definitions, evaluates their conditions and returns the first matching
     * type.
     */
    TypeInfoRef GetRelationType(const TagMap& tagMap) const;
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
  typedef std::shared_ptr<TypeConfig> TypeConfigRef;

  /**
   * \defgroup type Object type related data structures and services
   */
}

#endif
