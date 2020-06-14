#ifndef OSMSCOUT_FEATURE_READER_H
#define OSMSCOUT_FEATURE_READER_H

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

#include <limits>
#include <vector>

#include <osmscout/TypeFeature.h>
#include <osmscout/TypeFeatures.h>
#include <osmscout/TypeConfig.h>

namespace osmscout {

  /**
   * Helper template class for easy access to flag-like Features.
   *
   * Each type may have stored the feature in request at a different index. The FeatureReader
   * caches the index for each type once in the constructor and later on allows access to the feature
   * in O(1) - without iterating of all feature(values) of an object.
   */
  template<class F>
  class FeatureReader
  {
  private:
    std::vector<size_t> lookupTable;

  public:
    explicit FeatureReader(const TypeConfig& typeConfig);

    /**
     * Returns the index of the Feature/FeatureValue within the given FeatureValueBuffer.
     *
     * @param buffer
     *    The FeatureValueBuffer instance
     * @param index
     *    The index
     * @return
     *    true, if there is a valid index 8because the type has such feature), else false
     */
    bool GetIndex(const FeatureValueBuffer& buffer,
                  size_t& index) const;

    /**
     * Returns true, if the feature is set for the given FeatureValueBuffer
     * @param buffer
     *    The FeatureValueBuffer instance
     * @return
     *    true if set, else false
     */
    bool IsSet(const FeatureValueBuffer& buffer) const;
  };

  template<class F>
  FeatureReader<F>::FeatureReader(const TypeConfig& typeConfig)
  {
    FeatureRef feature=typeConfig.GetFeature(F::NAME);

    lookupTable.resize(typeConfig.GetTypeCount(),
                       std::numeric_limits<size_t>::max());

    for (const auto &type : typeConfig.GetTypes()) {
      size_t index;

      if (type->GetFeature(F::NAME,
                           index)) {
        lookupTable[type->GetIndex()]=index;
      }
    }
  }

  template<class F>
  bool FeatureReader<F>::GetIndex(const FeatureValueBuffer& buffer,
                                  size_t& index) const
  {
    index=lookupTable[buffer.GetType()->GetIndex()];

    return index!=std::numeric_limits<size_t>::max();
  }

  template<class F>
  bool FeatureReader<F>::IsSet(const FeatureValueBuffer& buffer) const
  {
    size_t index=lookupTable[buffer.GetType()->GetIndex()];

    if (index!=std::numeric_limits<size_t>::max()) {
      return buffer.HasFeature(index);
    }

    return false;
  }

  using AccessRestrictedFeatureReader = FeatureReader<AccessRestrictedFeature>;
  using BridgeFeatureReader           = FeatureReader<BridgeFeature>;
  using TunnelFeatureReader           = FeatureReader<TunnelFeature>;
  using EmbankmentFeatureReader       = FeatureReader<EmbankmentFeature>;
  using RoundaboutFeatureReader       = FeatureReader<RoundaboutFeature>;

  /**
   * Variant of FeatureReader that is not type set and thus can easier get used
   * in cases where runtime dynamics are required and features are referenced
   * by name and not by type.
   */
  class OSMSCOUT_API DynamicFeatureReader CLASS_FINAL
  {
  private:
    std::string         featureName;
    std::vector<size_t> lookupTable;

  public:
    DynamicFeatureReader(const TypeConfig& typeConfig,
                         const Feature& feature);

    inline std::string GetFeatureName() const
    {
      return featureName;
    }

    bool IsSet(const FeatureValueBuffer& buffer) const;

    FeatureValue* GetValue(const FeatureValueBuffer& buffer) const;
  };

  /**
   * Helper template class for easy access to the value of a certain feature for objects of any type.
   *
   * Each type may have stored the feature in request at a different index. The FeatureValueReader
   * caches the index for each type once in the constructor and later on allows access to the feature value
   * in O(1) - without iterating of all feature(values) of an object.
   */
  template<class F, class V>
  class FeatureValueReader
  {
  private:
    std::vector<size_t> lookupTable;

    static_assert(std::is_base_of<Feature, F>::value, "F have to be subtype of Feature");
    static_assert(std::is_base_of<FeatureValue, V>::value, "V have to be subtype of FeatureValue");

  public:
    explicit FeatureValueReader(const TypeConfig& typeConfig);

    /**
     * Returns the index of the Feature/FeatureValue within the given FeatureValueBuffer.
     *
     * @param buffer
     *    The FeatureValueBuffer instance
     * @param index
     *    The index
     * @return
     *    true, if there is a valid index (because the type has such feature), else false
     */
    bool GetIndex(const FeatureValueBuffer& buffer,
                  size_t& index) const;

    /**
     * Returns the FeatureValue for the given FeatureValueBuffer
     * @param buffer
     *    The FeatureValueBuffer instance
     * @return
     *    A pointer to an instance if the Type and the instance do have the feature and its value is not nullptr,
     *    else nullptr
     */
    V* GetValue(const FeatureValueBuffer& buffer) const;

    /**
     * Returns the FeatureValue for the given FeatureValueBuffer or a defaultValue, if the feature is not set
     * @param buffer
     *    The FeatureValueBuffer instance
     * @return
     *    Either the value from the FeatureValueBuffer or the defaultValue
     */
    V GetValue(const FeatureValueBuffer& buffer, const V& defaultValue) const;
  };

  template<class F, class V>
  FeatureValueReader<F,V>::FeatureValueReader(const TypeConfig& typeConfig)
  {
    FeatureRef feature=typeConfig.GetFeature(F::NAME);

    assert(feature->HasValue());

    lookupTable.resize(typeConfig.GetTypeCount(),
                       std::numeric_limits<size_t>::max());

    for (const auto &type : typeConfig.GetTypes()) {
      size_t index;

      if (type->GetFeature(F::NAME,
                           index)) {
        lookupTable[type->GetIndex()]=index;
      }
    }
  }

  template<class F, class V>
  bool FeatureValueReader<F,V>::GetIndex(const FeatureValueBuffer& buffer,
                                         size_t& index) const
  {
    assert(buffer.GetType()->GetIndex() < lookupTable.size());
    index=lookupTable[buffer.GetType()->GetIndex()];

    return index!=std::numeric_limits<size_t>::max();
  }

  template<class F, class V>
  V* FeatureValueReader<F,V>::GetValue(const FeatureValueBuffer& buffer) const
  {
    assert(buffer.GetType()->GetIndex()<lookupTable.size());
    size_t index=lookupTable[buffer.GetType()->GetIndex()];

    if (index!=std::numeric_limits<size_t>::max() &&
        buffer.HasFeature(index)) {
      FeatureValue* val=buffer.GetValue(index);
      // Object returned from Feature::AllocateValue and V have to be the same type!
      // But it cannot be tested in compile-time, lets do it in runtime assert at least.
      assert(val==nullptr || dynamic_cast<V*>(val)!=nullptr);
      return static_cast<V*>(val);
    }

    return nullptr;
  }

  template<class F, class V>
  V FeatureValueReader<F,V>::GetValue(const FeatureValueBuffer& buffer, const V& defaultValue) const
  {
    assert(buffer.GetType()->GetIndex() < lookupTable.size());
    size_t index=lookupTable[buffer.GetType()->GetIndex()];

    if (index!=std::numeric_limits<size_t>::max() &&
        buffer.HasFeature(index)) {
      FeatureValue *val = buffer.GetValue(index);
      // Object returned from Feature::AllocateValue and V have to be the same type!
      // But it cannot be tested in compile-time, lets do it in runtime assert at least.
      assert(val == nullptr || dynamic_cast<V*>(val) != nullptr);
      return *static_cast<V*>(val);
    }

    return defaultValue;
  }

  using NameFeatureValueReader             = FeatureValueReader<NameFeature, NameFeatureValue>;
  using NameAltFeatureValueReader          = FeatureValueReader<NameAltFeature, NameAltFeatureValue>;
  using NameShortFeatureValueReader        = FeatureValueReader<NameShortFeature, NameShortFeatureValue>;
  using RefFeatureValueReader              = FeatureValueReader<RefFeature, RefFeatureValue>;
  using LocationFeatureValueReader         = FeatureValueReader<LocationFeature, LocationFeatureValue>;
  using AddressFeatureValueReader          = FeatureValueReader<AddressFeature, AddressFeatureValue>;
  using AccessFeatureValueReader           = FeatureValueReader<AccessFeature, AccessFeatureValue>;
  using AccessRestrictedFeatureValueReader = FeatureValueReader<AccessRestrictedFeature, AccessRestrictedFeatureValue>;
  using LayerFeatureValueReader            = FeatureValueReader<LayerFeature, LayerFeatureValue>;
  using WidthFeatureValueReader            = FeatureValueReader<WidthFeature, WidthFeatureValue>;
  using MaxSpeedFeatureValueReader         = FeatureValueReader<MaxSpeedFeature, MaxSpeedFeatureValue>;
  using GradeFeatureValueReader            = FeatureValueReader<GradeFeature, GradeFeatureValue>;
  using AdminLevelFeatureValueReader       = FeatureValueReader<AdminLevelFeature, AdminLevelFeatureValue>;
  using PostalCodeFeatureValueReader       = FeatureValueReader<PostalCodeFeature, PostalCodeFeatureValue>;
  using IsInFeatureValueReader             = FeatureValueReader<IsInFeature, IsInFeatureValue>;
  using DestinationFeatureValueReader      = FeatureValueReader<DestinationFeature, DestinationFeatureValue>;
  using ConstructionYearFeatureValueReader = FeatureValueReader<ConstructionYearFeature, ConstructionYearFeatureValue>;
  using LanesFeatureValueReader            = FeatureValueReader<LanesFeature, LanesFeatureValue>;
  using EleFeatureValueReader              = FeatureValueReader<EleFeature, EleFeatureValue>;
  using OperatorFeatureValueReader         = FeatureValueReader<OperatorFeature, OperatorFeatureValue>;
  using NetworkFeatureValueReader          = FeatureValueReader<NetworkFeature, NetworkFeatureValue>;
  using FromToFeatureValueReader           = FeatureValueReader<FromToFeature, FromToFeatureValue>;
  using ColorFeatureValueReader            = FeatureValueReader<ColorFeature, ColorFeatureValue>;

  template <class F, class V>
  class FeatureLabelReader
  {
  private:
    std::vector<size_t> lookupTable;

  public:
    explicit FeatureLabelReader(const TypeConfig& typeConfig);

    /**
     * Returns the label of the given object
     * @param buffer
     *    The FeatureValueBuffer instance
     * @return
     *    The label, if the given feature has a value and a label  or a empty string
     */
    std::string GetLabel(const FeatureValueBuffer& buffer) const;
  };

  template<class F, class V>
  FeatureLabelReader<F,V>::FeatureLabelReader(const TypeConfig& typeConfig)
  {
    FeatureRef feature=typeConfig.GetFeature(F::NAME);

    assert(feature->HasLabel());

    lookupTable.resize(typeConfig.GetTypeCount(),
                       std::numeric_limits<size_t>::max());

    for (const auto &type : typeConfig.GetTypes()) {
      size_t index;

      if (type->GetFeature(F::NAME,
                           index)) {
        lookupTable[type->GetIndex()]=index;
      }
    }
  }

  template<class F, class V>
  std::string FeatureLabelReader<F,V>::GetLabel(const FeatureValueBuffer& buffer) const
  {
    size_t index=lookupTable[buffer.GetType()->GetIndex()];

    if (index!=std::numeric_limits<size_t>::max() &&
        buffer.HasFeature(index)) {
      V* value=dynamic_cast<V*>(buffer.GetValue(index));

      if (value!=nullptr) {
        return value->GetLabel(Locale(), 0);
      }
    }

    return "";
  }

  using NameFeatureLabelReader = FeatureLabelReader<NameFeature, NameFeatureValue>;
  using RefFeatureLabelReader  = FeatureLabelReader<RefFeature, RefFeatureValue>;

  /**
   * \defgroup type Object type related data structures and services
   */
}

#endif
