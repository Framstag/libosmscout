#ifndef OSMSCOUT_TYPEFEATURE_H
#define OSMSCOUT_TYPEFEATURE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2018  Tim Teulings

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
#include <string>
#include <unordered_map>
#include <vector>

#include <osmscout/lib/CoreImportExport.h>

#include <osmscout/util/Locale.h>
#include <osmscout/util/TagErrorReporter.h>

#include <osmscout/io/FileScanner.h>
#include <osmscout/io/FileWriter.h>

#include <osmscout/system/Assert.h>

namespace osmscout {

  class OSMSCOUT_API FeatureValue
  {
  public:
    FeatureValue() = default;
    FeatureValue(const FeatureValue& featureValue) = default;

    virtual ~FeatureValue() = default;

    virtual FeatureValue& operator=(const FeatureValue& other); //NOSONAR

    virtual std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const
    {
      return "";
    }

    virtual bool IsFlagSet(size_t /*flagIndex*/) const
    {
      assert(false);

      return false;
    }

    /**
     * Read the value of the Feature from the FileScanner
     *
     * @throws IOException
     */
    virtual void Read(FileScanner& scanner) = 0;

    /**
     * Write the FeatureValue to disk.
     *
     * @throws IOException.
     */
    virtual void Write(FileWriter& writer) = 0;

    virtual bool operator==(const FeatureValue& other) const = 0; // NOSONAR

    virtual bool operator!=(const FeatureValue& other) const // NOSONAR
    {
      return !(*this==other);
    }
  };

  // Forward declaration of classes because
  // of circular dependency between them and Feature
  class FeatureValueBuffer;
  class FeatureInstance;

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
    std::unordered_map<std::string,size_t>      flags;
    std::unordered_map<std::string,std::string> descriptions; //!< Map of descriptions for given language codes

  protected:
    void RegisterLabel(size_t index,
                       const std::string& labelName);

    void RegisterFlag(size_t index,
                      const std::string& flagName);

  public:
    Feature() = default;

    virtual ~Feature() = default;

    /**
     * Does further initialization based on the current TagRegistry. For example
     * it registers Tags (and stores their TagId) for further processing.
     */
    virtual void Initialize(TagRegistry& tagRegistry) = 0;

    void AddDescription(const std::string& languageCode,
                        const std::string& description);

    /**
     * Returns the name of the feature
     */
    virtual std::string GetName() const = 0;

    /**
     * If feature have value object, this method returns
     * alignment requirements of the value type (alignof( type-id )).
     */
    virtual size_t GetValueAlignment() const
    {
      return 0;
    }

    /**
     * A feature, if set for an object, can hold a value. If there is no value object,
     * this method returns 0, else it returns the C++ size of the value object.
     */
    virtual size_t GetValueSize() const
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
    virtual size_t GetFeatureBitCount() const
    {
      return 0;
    }

    /**
     * Returns 'true' if the feature has an value object.
     */
    virtual bool HasValue() const
    {
      return GetValueSize()>0;
    }

    /**
     * Returns 'true' if the feature provides labels.
     */
    virtual bool HasLabel() const
    {
      return !labels.empty();
    }

    /**
     * Returns 'true' if the feature provides flags.
     */
    virtual bool HasFlags() const
    {
      return !flags.empty();
    }

    /**
     * Returns the index of the label with the given name. Method returns 'true'
     * if the feature has labels and a label with the given name exists. Else
     * 'false' is returned.
     */
    bool GetLabelIndex(const std::string& labelName,
                       size_t& index) const;

    /**
     * Returns the index of the feature flag with the given name. Method returns 'true'
     * if the feature has the named flag. Else
     * 'false' is returned.
     */
    bool GetFlagIndex(const std::string& flagName,
                      size_t& index) const;

    std::string GetDescription(const std::string& languageCode) const;

    const std::unordered_map<std::string,std::string>& GetDescriptions() const
    {
      return descriptions;
    };

    virtual FeatureValue* AllocateValue(void* buffer);

    virtual void Parse(TagErrorReporter& reporter,
                       const TagRegistry& tagRegistry,
                       const FeatureInstance& feature,
                       const ObjectOSMRef& object,
                       const TagMap& tags,
                       FeatureValueBuffer& buffer) const = 0;
  };

  using FeatureRef = std::shared_ptr<Feature>;

  // Forward declaration of TypeInfo
  class TypeInfo;

  /**
   * An instantiation of a feature for a certain type.
   */
  class OSMSCOUT_API FeatureInstance CLASS_FINAL
  {
  private:
    FeatureRef    feature;    //!< The feature we are an instance of
    const TypeInfo* type;      //!< The type we are assigned to (we are no Ref type to avoid circular references)
    size_t        featureBit; //!< index of the bit that signals that the feature is available
    size_t        index;      //!< The index we have in the list of features
    size_t        offset;     //!< Our offset into the value buffer for our data

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
    FeatureRef GetFeature() const
    {
      return feature;
    }

    /**
     * Return a pointer back tot he type we are assigned to.
     */
    const TypeInfo* GetType() const
    {
      return type;
    }

    /**
     * return the index of this feature within the list of features of the type.
     */
    size_t GetFeatureBit() const
    {
      return featureBit;
    }

    /**
     * return the index of this feature within the list of features of the type.
     */
    size_t GetIndex() const
    {
      return index;
    }

    /**
     * Return the file offset within the feature value buffer for the value of this feature.
     */
    size_t GetOffset() const
    {
      return offset;
    }
  };
}

#endif
