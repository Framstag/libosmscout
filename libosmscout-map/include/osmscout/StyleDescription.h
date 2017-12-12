#ifndef OSMSCOUT_STYLEDESCRIPTION_H
#define OSMSCOUT_STYLEDESCRIPTION_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Tim Teulings

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

#include <memory>
#include <unordered_map>
#include <vector>

#include <osmscout/private/MapImportExport.h>

#include <osmscout/system/Compiler.h>

#include <osmscout/util/Color.h>
#include <osmscout/util/Magnification.h>

namespace osmscout {

  class Symbol;
  typedef std::shared_ptr<Symbol> SymbolRef;

  class LabelProvider;
  typedef std::shared_ptr<LabelProvider> LabelProviderRef;

  /**
   * \ingroup Stylesheet
   *
   * Interface class that offers a medium generic interface for styles classes. The
   * interface defines methods for setting index attributes to a given value.
   *
   * Used by the style sheet parser. The parser uses the StyleDescriptor to get te attribute name,
   * type and index. Attribute values are written back to the style instance using the index.
   */
  class OSMSCOUT_MAP_API Style
  {
  public:
    virtual ~Style();

    virtual void SetBoolValue(int attribute, bool value);
    virtual void SetStringValue(int attribute, const std::string& value);
    virtual void SetColorValue(int attribute, const Color& value);
    virtual void SetMagnificationValue(int attribute, const Magnification& value);
    virtual void SetDoubleValue(int attribute, double value);
    virtual void SetDoubleArrayValue(int attribute, const std::vector<double>& value);
    virtual void SetSymbolValue(int attribute, const SymbolRef& value);
    virtual void SetIntValue(int attribute, int value);
    virtual void SetUIntValue(int attribute, size_t value);
    virtual void SetLabelValue(int attribute, const LabelProviderRef& value);
  };

  // Probem under Mac OS X
  #undef TYPE_BOOL

  /**
   * \ingroup Stylesheet
   *
   * Enumeration of different style sheet attribute value types
   */
  enum class StyleAttributeType
  {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_COLOR,
    TYPE_MAGNIFICATION,
    TYPE_ENUM,
    TYPE_DISPLAY_SIZE,
    TYPE_UDISPLAY_SIZE,
    TYPE_MAP_SIZE,
    TYPE_UMAP_SIZE,
    TYPE_DOUBLE,
    TYPE_UDOUBLE,
    TYPE_UDOUBLE_ARRAY,
    TYPE_INT,
    TYPE_UINT,
    TYPE_LABEL,
    TYPE_SYMBOL
  };

  /**
   * \ingroup Stylesheet
   *
   * Base class for all attribute metadata
   */
  class OSMSCOUT_MAP_API StyleAttributeDescriptor
  {
  private:
    StyleAttributeType type;         //!< Type if the attribute
    std::string        name;         //!< Name of the attribute
    int                attribute;    //!< The id of the attribute to set

  protected:
    StyleAttributeDescriptor(StyleAttributeType type,
                             const std::string& name,
                             int attribute);

  public:
    virtual ~StyleAttributeDescriptor();

    inline std::string GetName() const
    {
      return name;
    }

    inline StyleAttributeType GetType() const
    {
      return type;
    }

    inline int GetAttribute() const
    {
      return attribute;
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a 'VOID' attribute. This attribute type is only used internally.
   */
  class OSMSCOUT_MAP_API StyleVoidAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleVoidAttributeDescriptor()
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_VOID,
                                 "",
                                 -1)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a bool attribute value
   */
  class OSMSCOUT_MAP_API StyleBoolAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleBoolAttributeDescriptor(const std::string& name,
                                 int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_BOOL,
                                 name,
                                 attribute)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a string attribute value
   */
  class OSMSCOUT_MAP_API StyleStringAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleStringAttributeDescriptor(const std::string& name,
                                   int attribute)
    : StyleAttributeDescriptor(StyleAttributeType::TYPE_STRING,
                               name,
                               attribute)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a color attribute value
   */
  class OSMSCOUT_MAP_API StyleColorAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleColorAttributeDescriptor(const std::string& name,
                                  int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_COLOR,
                                 name,
                                 attribute)
    {
      // no code
    }
  };


  /**
   * \ingroup Stylesheet
   *
   * Describes a magnification attribute value
   */
  class OSMSCOUT_MAP_API StyleMagnificationAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleMagnificationAttributeDescriptor(const std::string& name,
                                          int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_MAGNIFICATION,
                                 name,
                                 attribute)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a enumeration attribute value. The base class has to get derived for defining
   * an actual enum type.
   */
  class OSMSCOUT_MAP_API StyleEnumAttributeDescriptor : public StyleAttributeDescriptor
  {
  public:
    typedef std::unordered_map<std::string,int> EnumNameValueMap;

  private:
    EnumNameValueMap enumMap;

  protected:
    StyleEnumAttributeDescriptor(const std::string& name,
                                 int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_ENUM,
                                 name,
                                 attribute)
    {
      // no code
    }

  public:
    void AddEnumValue(const std::string& name,
                     int value)
    {
      enumMap.insert(std::make_pair(name,value));
    }

    int GetEnumValue(const std::string& name) const
    {
      EnumNameValueMap::const_iterator entry=enumMap.find(name);

      if (entry!=enumMap.end()) {
        return entry->second;
      }
      else {
        return -1;
      }
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a display size (using 'mm' unit) attribute value
   */
  class OSMSCOUT_MAP_API StyleDisplayAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleDisplayAttributeDescriptor(const std::string& name,
                                    int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_DISPLAY_SIZE,
                                 name,
                                 attribute)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a unsigned display size (using 'mm' unit) attribute value
   */
  class OSMSCOUT_MAP_API StyleUDisplayAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleUDisplayAttributeDescriptor(const std::string& name,
                                     int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_UDISPLAY_SIZE,
                                 name,
                                 attribute)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a map size (using 'm' unit) attribute value
   */
  class OSMSCOUT_MAP_API StyleMapAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleMapAttributeDescriptor(const std::string& name,
                                int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_MAP_SIZE,
                                 name,
                                 attribute)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a unsigned map size (using 'm' unit) attribute value
   */
  class OSMSCOUT_MAP_API StyleUMapAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleUMapAttributeDescriptor(const std::string& name,
                                 int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_UMAP_SIZE,
                                 name,
                                 attribute)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a unitless double attribute value
   */
  class OSMSCOUT_MAP_API StyleDoubleAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleDoubleAttributeDescriptor(const std::string& name,
                                   int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_DOUBLE,
                                 name,
                                 attribute)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a unitless unsigned double attribute value
   */
  class OSMSCOUT_MAP_API StyleUDoubleAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleUDoubleAttributeDescriptor(const std::string& name,
                                    int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_UDOUBLE,
                                 name,
                                 attribute)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a unitless array of unsigned double attribute value
   */
  class OSMSCOUT_MAP_API StyleUDoubleArrayAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleUDoubleArrayAttributeDescriptor(const std::string& name,
                                         int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_UDOUBLE_ARRAY,
                                 name,
                                 attribute)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a unitless int attribute value
   */
  class OSMSCOUT_MAP_API StyleIntAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleIntAttributeDescriptor(const std::string& name,
                                int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_INT,
                                 name,
                                 attribute)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a unitless unsigned int attribute value
   */
  class OSMSCOUT_MAP_API StyleUIntAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleUIntAttributeDescriptor(const std::string& name,
                                 int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_UINT,
                                 name,
                                 attribute)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a label attribute value
   */
  class OSMSCOUT_MAP_API StyleLabelAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleLabelAttributeDescriptor(const std::string& name,
                                  int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_LABEL,
                                 name,
                                 attribute)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Describes a symbol attribute value
   */
  class OSMSCOUT_MAP_API StyleSymbolAttributeDescriptor CLASS_FINAL : public StyleAttributeDescriptor
  {
  public:
    StyleSymbolAttributeDescriptor(const std::string& name,
                                   int attribute)
      : StyleAttributeDescriptor(StyleAttributeType::TYPE_SYMBOL,
                                 name,
                                 attribute)
    {
      // no code
    }
  };


  typedef std::shared_ptr<StyleAttributeDescriptor> StyleAttributeDescriptorRef;

  /**
   * \ingroup Stylesheet
   *
   * Holds Meta information and technical description of a style. It currently holds
   * a list of parameters and their types. It also allows to assign type safe values
   * to a given style object.
   */
  class OSMSCOUT_MAP_API StyleDescriptor
  {
  private:
    std::unordered_map<std::string,StyleAttributeDescriptorRef> attributeMap;

  protected:
    void AddAttribute(const StyleAttributeDescriptorRef& attribute);

  public:
    StyleAttributeDescriptorRef GetAttribute(const std::string& name) const
    {
      auto result=attributeMap.find(name);

      if (result!=attributeMap.end())  {
        return result->second;
      }
      else {
        return NULL;
      }
    }
  };

  typedef std::shared_ptr<StyleDescriptor> StyleDescriptorRef;
}

#endif
