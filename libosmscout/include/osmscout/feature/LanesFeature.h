#ifndef OSMSCOUT_FEATURE_LANES_FEATURE_H
#define OSMSCOUT_FEATURE_LANES_FEATURE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2014  Tim Teulings

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
#include <osmscout/TypeFeature.h>

#include <osmscout/FeatureReader.h>

#include <osmscout/util/LaneTurn.h>

namespace osmscout {

  class OSMSCOUT_API LanesFeatureValue : public FeatureValue
  {
  private:

    uint8_t     lanes=0;              //!< First two bits reserved, 3 bit for number of lanes in each direction
    std::vector<LaneTurn> turnForward;
    std::vector<LaneTurn> turnBackward;
    std::string destinationForward;
    std::string destinationBackward;

  public:
    LanesFeatureValue() = default;

    explicit LanesFeatureValue(uint8_t lanes)
      : lanes(lanes)
    {
      // no code
    }

    void SetLanes(uint8_t forwardLanes,
                  uint8_t backwardLanes)
    {
      this->lanes=((forwardLanes & uint8_t(0x7u)) << 2) |
                  ((backwardLanes & uint8_t(0x7u)) << 5);
    }

    bool HasSingleLane() const
    {
      return GetLanes()==0;
    }

    uint8_t GetForwardLanes() const
    {
      return (lanes >> 2) & (uint8_t)0x07;
    }

    uint8_t GetBackwardLanes() const
    {
      return (lanes >> 5) & (uint8_t)0x07;
    }

    uint8_t GetLanes() const;

    void SetTurnLanes(const std::vector<LaneTurn>& turnForward,
                      const std::vector<LaneTurn>& turnBackward)
    {
      this->turnForward=turnForward;
      this->turnBackward=turnBackward;
    }

    std::vector<LaneTurn> GetTurnForward() const
    {
      return turnForward;
    }

    std::vector<LaneTurn> GetTurnBackward() const
    {
      return turnBackward;
    }

    std::string GetDestinationForward() const
    {
      return destinationForward;
    }

    std::string GetDestinationBackward() const
    {
      return destinationBackward;
    }

    void SetDestinationLanes(const std::string_view& destinationForward,
                             const std::string_view& destinationBackward)
    {
      this->destinationForward=destinationForward;
      this->destinationBackward=destinationBackward;
    }

    std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      if (HasSingleLane()) {
        return "1";
      }

      return std::to_string(GetForwardLanes()) + " " + std::to_string(GetBackwardLanes());
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    LanesFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API LanesFeature : public Feature
  {
  private:
    TagId tagOneway=0;
    TagId tagLanes=0;
    TagId tagLanesForward=0;
    TagId tagLanesBackward=0;
    TagId tagTurnLanes=0;
    TagId tagTurnLanesForward=0;
    TagId tagTurnLanesBackward=0;
    TagId tagDestinationLanes=0;
    TagId tagDestinationLanesForward=0;
    TagId tagDestinationLanesBackward=0;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "name" label */
    static const char* const NAME_LABEL;

    /** Index of the 'name' label */
    static const size_t      NAME_LABEL_INDEX;

  public:
    LanesFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueAlignment() const override;
    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  using LanesFeatureValueReader = FeatureValueReader<LanesFeature, LanesFeatureValue>;

}

#endif
