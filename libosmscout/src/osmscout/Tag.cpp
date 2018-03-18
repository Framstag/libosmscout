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

#include <osmscout/Tag.h>

#include <osmscout/util/String.h>

#include <osmscout/system/Assert.h>

namespace osmscout {

  TagCondition::~TagCondition()
  {
    // no code
  }

  TagNotCondition::TagNotCondition(const TagConditionRef& condition)
  : condition(condition)
  {
    // no code
  }

  TagBoolCondition::TagBoolCondition(Type type)
  : type(type)
  {
    // no code
  }

  void TagBoolCondition::AddCondition(const TagConditionRef& condition)
  {
    conditions.push_back(condition);
  }

  bool TagBoolCondition::Evaluate(const TagMap& tagMap) const
  {
    switch (type) {
    case boolAnd:
      for (const auto &condition : conditions) {
        if (!condition->Evaluate(tagMap)) {
          return false;
        }
      }

      return true;
    case boolOr:
      for (const auto &condition : conditions) {
        if (condition->Evaluate(tagMap)) {
          return true;
        }
      }

      return false;
    default:
      assert(false);

      return false;
    }
  }

  TagExistsCondition::TagExistsCondition(TagId tag)
  : tag(tag)
  {
    // no code
  }

  TagBinaryCondition::TagBinaryCondition(TagId tag,
                                         BinaryOperator binaryOperator,
                                         const std::string& tagValue)
  : tag(tag),
    binaryOperator(binaryOperator),
    valueType(string),
    tagStringValue(tagValue),
    tagSizeValue(0)
  {
    // no code
  }

  TagBinaryCondition::TagBinaryCondition(TagId tag,
                                         BinaryOperator binaryOperator,
                                         const size_t& tagValue)
  : tag(tag),
    binaryOperator(binaryOperator),
    valueType(sizet),
    tagSizeValue(tagValue)
  {
    // no code
  }

  bool TagBinaryCondition::Evaluate(const TagMap& tagMap) const
  {
    auto t=tagMap.find(tag);

    if (t==tagMap.end()) {
      return false;
    }

    if (valueType==string) {
      switch (binaryOperator) {
      case operatorLess:

        return t->second<tagStringValue;
      case operatorLessEqual:

        return t->second<=tagStringValue;
      case operatorEqual:

        return t->second==tagStringValue;
      case operatorNotEqual:

        return t->second!=tagStringValue;
      case operatorGreaterEqual:

        return t->second>=tagStringValue;
      case operatorGreater:

        return t->second>tagStringValue;
      default:
        assert(false);

        return false;
      }
    }
    else if (valueType==sizet) {
      size_t value;

      if (!StringToNumber(t->second,
                          value)) {
        return false;
      }

      switch (binaryOperator) {
      case operatorLess:

        return value<tagSizeValue;
      case operatorLessEqual:

        return value<=tagSizeValue;
      case operatorEqual:

        return value==tagSizeValue;
      case operatorNotEqual:

        return value!=tagSizeValue;
      case operatorGreaterEqual:

        return value>=tagSizeValue;
      case operatorGreater:

        return value>tagSizeValue;
      default:
        assert(false);

        return false;
      }
    }
    else {
      assert(false);

      return false;
    }
  }

  TagIsInCondition::TagIsInCondition(TagId tag)
  : tag(tag)
  {
    // no code
  }

  void TagIsInCondition::AddTagValue(const std::string& tagValue)
  {
    tagValues.insert(tagValue);
  }

  bool TagIsInCondition::Evaluate(const TagMap& tagMap) const
  {
    auto t=tagMap.find(tag);

    if (t==tagMap.end()) {
      return false;
    }

    return tagValues.find(t->second)!=tagValues.end();
  }

  TagInfo::TagInfo(TagId id,
                   const std::string& name)
  : id(id),
    name(name)
  {
    // no code
  }
}
