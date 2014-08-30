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
#include <set>
#include <string>

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/util/HashMap.h>
#include <osmscout/util/Parser.h>
#include <osmscout/util/Reference.h>

#include <osmscout/system/Types.h>

namespace osmscout {

  typedef uint16_t TagId;

  /**
   * \ingroup type
   *
   * Magic constant for an unresolved and to be ignored tag
   */
  static const TagId tagIgnore        = 0;

  /**
   * \ingroup type
   *
   * Abstract base class for all tag based conditions
   */
  class OSMSCOUT_API TagCondition : public Referencable
  {
  public:
    virtual ~TagCondition();

    virtual bool Evaluate(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const = 0;
  };

  /**
   * \ingroup type
   *
   * Reference counted reference to a tag condition
   */
  typedef Ref<TagCondition> TagConditionRef;

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
    TagNotCondition(TagCondition* condition);

    bool Evaluate(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const;
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
    TagBoolCondition(Type type);

    void AddCondition(TagCondition* condition);

    bool Evaluate(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const;
  };

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
    TagExistsCondition(TagId tag);

    bool Evaluate(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const;
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

    bool Evaluate(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const;
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
    TagId                 tag;
    std::set<std::string> tagValues;

  public:
    TagIsInCondition(TagId tag);

    void AddTagValue(const std::string& tagValue);

    bool Evaluate(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const;
  };

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
    TagInfo();
    TagInfo(const std::string& name);

    TagInfo& SetId(TagId id);

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
}

#endif
