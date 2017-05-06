#ifndef OSMSCOUT_LABELPROVIDER_H
#define OSMSCOUT_LABELPROVIDER_H

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

#include <vector>

#include <osmscout/private/MapImportExport.h>

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeatures.h>

#include <osmscout/MapParameter.h>

namespace osmscout {

  /**
   * \ingroup Stylesheet
   *
   * Interface one must implement to provider a label for the map.
   */
  class OSMSCOUT_MAP_API LabelProvider
  {
  public:
    virtual ~LabelProvider();

    /**
     * Returns the label based on the given feature value buffer
     *
     * @param buffer
     *    The FeatureValueBuffer instance
     * @return
     *    The label, if the given feature has a value and a label or a empty string
     */
    virtual std::string GetLabel(const MapParameter& parameter,
                                 const FeatureValueBuffer& buffer) const = 0;

    /**
     * Returns the name of the label provider as it must get stated in the style sheet
     */
    virtual std::string GetName() const = 0;
  };

  typedef std::shared_ptr<LabelProvider> LabelProviderRef;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API LabelProviderFactory
  {
  public:
    virtual ~LabelProviderFactory();

    virtual LabelProviderRef Create(const TypeConfig& typeConfig) const = 0;
  };

  typedef std::shared_ptr<LabelProviderFactory> LabelProviderFactoryRef;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API INameLabelProviderFactory : public LabelProviderFactory
  {
  private:
    class INameLabelProvider : public LabelProvider
    {
    private:
      std::vector<size_t> nameLookupTable;
      std::vector<size_t> nameAltLookupTable;

    public:
      INameLabelProvider(const TypeConfig& typeConfig);

      std::string GetLabel(const MapParameter& parameter,
                           const FeatureValueBuffer& buffer) const;

      inline std::string GetName() const
      {
        return "IName";
      }
    };

    private:
      mutable LabelProviderRef instance;

    public:
      LabelProviderRef Create(const TypeConfig& typeConfig) const;
  };

  /**
   * \ingroup Stylesheet
   *
   * Generates a label based on a given feature name and label name.
   *
   * Example:
   *   Give me the label "inMeter" of the Ele-Feature.
   */
  class OSMSCOUT_MAP_API DynamicFeatureLabelReader : public LabelProvider
  {
  private:
    std::vector<size_t> lookupTable;
    std::string         featureName;
    std::string         labelName;
    size_t              labelIndex;

  public:
    /**
     * Assigns a label to the reader
     *
     * @param typeConfig
     *   Reference to the current type configuration
     * @param featureName
     *   Name of the feature which must be valid and must support labels
     * @param labelIndex
     *   The index of the labels to use (a feature might support multiple labels)
     */
    DynamicFeatureLabelReader(const TypeConfig& typeConfig,
                              const std::string& featureName,
                              const std::string& labelName);

    std::string GetLabel(const MapParameter& parameter,
                         const FeatureValueBuffer& buffer) const;

    inline std::string GetName() const
    {
      return featureName + "." + labelName;
    }
  };
}

#endif
