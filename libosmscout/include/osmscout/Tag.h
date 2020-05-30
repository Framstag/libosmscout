#ifndef OSMSCOUT_TAG_H
#define OSMSCOUT_TAG_H

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

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <osmscout/CoreImportExport.h>

#include <osmscout/util/Parsing.h>

#include <osmscout/system/Compiler.h>
#include <osmscout/system/OSMScoutTypes.h>

namespace osmscout {

  using TagId = uint16_t;

  using TagMap = std::unordered_map<TagId, std::string>;

  /**
   * \ingroup type
   *
   * Magic constant for an unresolved and to be ignored tag
   */
  static const TagId tagIgnore = 0;

  /**
   * \ingroup type
   *
   * Abstract base class for all tag based conditions
   */
  class OSMSCOUT_API TagCondition
  {
  public:
    virtual ~TagCondition() = default;

    virtual bool Evaluate(const TagMap& tagMap) const = 0;
  };

  /**
   * \ingroup type
   *
   * Reference counted reference to a tag condition
   */
  using TagConditionRef = std::shared_ptr<TagCondition>;

  /**
   * \ingroup type
   *
   * Negates the result of the given child condition
   */
  class OSMSCOUT_API TagNotCondition : public TagCondition
  {
  private:
    TagConditionRef condition;

  public:
    explicit TagNotCondition(const TagConditionRef& condition);

    inline bool Evaluate(const TagMap& tagMap) const override
    {
      return !condition->Evaluate(tagMap);
    }
  };

  /**
   * \ingroup type
   *
   * Allows a boolean and/or condition between a number of
   * child conditions.
   */
  class OSMSCOUT_API TagBoolCondition : public TagCondition
  {
  public:
    enum Type {
      boolAnd,
      boolOr
    };

  private:
    std::list<TagConditionRef> conditions;
    Type                       type;

  public:
    explicit TagBoolCondition(Type type);

    void AddCondition(const TagConditionRef& condition);

    bool Evaluate(const TagMap& tagMap) const override;
  };

  /**
   * \ingroup type
   *
   * Reference counted reference to a tag condition
   */
  using TagBoolConditionRef = std::shared_ptr<TagBoolCondition>;

  /**
   * \ingroup type
   *
   * Returns true, if the given tag exists for an object
   */
  class OSMSCOUT_API TagExistsCondition : public TagCondition
  {
  private:
    TagId tag;

  public:
    explicit TagExistsCondition(TagId tag);

    inline bool Evaluate(const TagMap& tagMap) const override
    {
      return tagMap.find(tag)!=tagMap.end();
    }
  };

  /**
   * \ingroup type
   *
   * Returns true, if the value of the given tag fulfills the given
   * boolean condition in regard to the comparison value.
   */
  class OSMSCOUT_API TagBinaryCondition : public TagCondition
  {
  private:
    enum ValueType {
      string,
      sizet
    };

  private:
    TagId          tag;
    BinaryOperator binaryOperator;
    ValueType      valueType;
    std::string    tagStringValue;
    size_t         tagSizeValue;

  public:
    TagBinaryCondition(TagId tag,
                       BinaryOperator binaryOperator,
                       const std::string& tagValue);
    TagBinaryCondition(TagId tag,
                       BinaryOperator binaryOperator,
                       const size_t& tagValue);

    bool Evaluate(const TagMap& tagMap) const override;
  };

  /**
   * \ingroup type
   *
   * Returns true, if the tag value of the given is one of the
   * given values.
   */
  class OSMSCOUT_API TagIsInCondition : public TagCondition
  {
  private:
    TagId                           tag;
    std::unordered_set<std::string> tagValues;

  public:
    explicit TagIsInCondition(TagId tag);

    void AddTagValue(const std::string& tagValue);

    bool Evaluate(const TagMap& tagMap) const override;
  };

  /**
   * \ingroup type
   *
   * Reference counted reference to a tag condition
   */
  using TagIsInConditionRef = std::shared_ptr<TagIsInCondition>;

  /**
   * \ingroup type
   *
   * Information about a tag definition
   */
  class OSMSCOUT_API TagInfo
  {
  private:
    TagId       id;
    std::string name;

  public:
    TagInfo(TagId id,
            const std::string& name);

    inline std::string GetName() const
    {
      return name;
    }

    /**
     * Returns the unique id of this tag
     */
    inline TagId GetId() const
    {
      return id;
    }
  };

  class OSMSCOUT_API TagRegistry CLASS_FINAL
  {
  private:
    // Tags

    std::vector<TagInfo>                        tags;

    TagId                                       nextTagId;

    std::unordered_map<std::string,TagId>       stringToTagMap;
    std::unordered_map<TagId,uint32_t>          nameTagIdToPrioMap;
    std::unordered_map<TagId,uint32_t>          nameAltTagIdToPrioMap;
    std::unordered_map<std::string,uint8_t>     nameToMaxSpeedMap;

    std::unordered_map<std::string,size_t>      surfaceToGradeMap;

  public:
    TagRegistry();
    ~TagRegistry();

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

    /**
     * Methods for dealing with mappings for surfaces and surface grades.
     */
    //@{
    void RegisterSurfaceToGradeMapping(const std::string& surface,
                                       size_t grade);
    /**
     * \note surface has multiple values often ("asphalt;ground;gravel")
     * use first that is matching to some grade
     */
    bool GetGradeForSurface(const std::string& surfaceValue,
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
  };
}

#endif
