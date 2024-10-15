/*
  This source is part of the libosmscout library
  Copyright (C) 2024  Tim Teulings

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

#ifndef OSMSCOUT_DESCRIPTIONSERVICE_H
#define OSMSCOUT_DESCRIPTIONSERVICE_H

#include <osmscout/lib/CoreImportExport.h>

#include <osmscout/system/Compiler.h>

#include <osmscout/Area.h>
#include <osmscout/Node.h>
#include <osmscout/Way.h>

namespace osmscout {
  /**
   * \defgroup Description Description
   *
   * Classes and functions for structured descriptions of objects on the map.
   */

  /**
   * ingroup Description
   *
   * An information set as part of the description
   */
  class OSMSCOUT_API DescriptionEntry CLASS_FINAL
  {
  private:
    std::string sectionKey;
    std::string subsectionKey;
    bool        hasIndex;
    size_t      index;
    std::string labelKey;
    std::string value;

  public:
    DescriptionEntry(const std::string& sectionKey,
                     const std::string& labelKey,
                     const std::string& value);

    DescriptionEntry(const std::string& sectionKey,
                     const std::string& subsectionKey,
                     const std::string& labelKey,
                     const std::string& value);

    DescriptionEntry(const std::string& sectionKey,
                     const std::string& subsectionKey,
                     size_t index,
                     const std::string& labelKey,
                     const std::string& value);

    std::string GetSectionKey() const {
      return sectionKey;
    }

    bool HasSubsection() const {
      return !subsectionKey.empty();
    }

    bool HasIndex() const {
      return hasIndex;
    }

    size_t GetIndex() const {
      return index;
    }

    std::string GetSubsectionKey() const {
      return subsectionKey;
    }

    std::string GetLabelKey() const {
      return labelKey;
    }

    std::string GetValue() const {
      return value;
    }
  };

  /**
   * ingroup Description
   *
   * Class, providing a number of information for an object
   */
  class OSMSCOUT_API ObjectDescription CLASS_FINAL
  {
  private:
    std::list<DescriptionEntry> entries;

  public:
    ObjectDescription();

    void AddEntry(const DescriptionEntry& entry)
    {
      entries.push_back(entry);
    }

    const std::list<DescriptionEntry>& GetEntries() const
    {
      return entries;
    }
  };

  /**
   * ingroup Description
   *
   * Interface to be implemented by the individual processors.
   */
  class OSMSCOUT_API FeatureToDescriptionProcessor
  {
  protected:
    FeatureValue* GetFeatureValue(const FeatureValueBuffer& buffer,
                                  const std::string& featureName) const;
  public:
    FeatureToDescriptionProcessor() = default;
    virtual ~FeatureToDescriptionProcessor();

    virtual void Process(const FeatureValueBuffer& buffer,
                         ObjectDescription& description) = 0;
  };

  /**
   * ingroup Description
   */
  using FeatureToDescriptionProcessorRef = std::shared_ptr<FeatureToDescriptionProcessor>;

  /**
   * ingroup Description
   *
   * General information, likely available for most of the objects
   */
  class GeneralDescriptionProcessor : public FeatureToDescriptionProcessor
  {
  public:
    static const std::string SECTION_NAME_GENERAL;

    static const std::string LABEL_KEY_NAME_TYPE;
    static const std::string LABEL_KEY_NAME_NAME;
    static const std::string LABEL_KEY_NAME_NAME_ALT;
    static const std::string LABEL_KEY_NAME_NAME_SHORT;
    static const std::string LABEL_KEY_NAME_NAME_REF;
    static const std::string LABEL_KEY_NAME_NAME_CONSTRUCTIONYEAR;

  public:
    void Process(const FeatureValueBuffer& buffer,
                 ObjectDescription& description) override;
  };

  /**
   * ingroup Description
   *
   * Geometric information
   */
  class GeometryDescriptionProcessor : public FeatureToDescriptionProcessor
  {
  public:
    static const std::string SECTION_NAME_GEOMETRY;

    static const std::string LABEL_KEY_GEOMETRY_COORDINATE;
    static const std::string LABEL_KEY_GEOMETRY_BOUNDINGBOX;
    static const std::string LABEL_KEY_GEOMETRY_CENTER;
    static const std::string LABEL_KEY_GEOMETRY_CELLLEVEL;
    static const std::string LABEL_KEY_GEOMETRY_LAYER;
    static const std::string LABEL_KEY_GEOMETRY_ISIN;

  public:
    void Process(const FeatureValueBuffer& buffer,
                 ObjectDescription& description) override;
  };

  /**
   * ingroup Description
   *
   * Information regarding the human-defined location of the object
   */
  class LocationDescriptionProcessor : public FeatureToDescriptionProcessor
  {
  public:
    static const std::string SECTION_NAME_LOCATION;

    static const std::string SUBSECTION_NAME_LOCATION_ADMINLEVEL;

    static const std::string LABEL_KEY_LOCATION_ADDRESS;
    static const std::string LABEL_KEY_LOCATION_LOCATION;
    static const std::string LABEL_KEY_LOCATION_POSTALCODE;

    static const std::string LABEL_KEY_LOCATION_ADMINLEVEL_LEVEL;
    static const std::string LABEL_KEY_LOCATION_ADMINLEVEL_ISIN;

  public:
    void Process(const FeatureValueBuffer& buffer,
                 ObjectDescription& description) override;
  };

  /**
   * ingroup Description
   *
   * All information regarding ways, their structure, grade and accessibility,
   */
  class WayDescriptionProcessor : public FeatureToDescriptionProcessor
  {
  public:
    static const std::string SECTION_NAME_WAY;

    static const std::string SUBSECTION_NAME_WAY_LANES;
    static const std::string SUBSECTION_NAME_WAY_SIDEWAYS;
    static const std::string SUBSECTION_NAME_WAY_ACCESS;
    static const std::string SUBSECTION_NAME_WAY_ACCESSRESTRICTED;

    static const std::string LABEL_KEY_WAY_BRIDGE;
    static const std::string LABEL_KEY_WAY_TUNNEL;
    static const std::string LABEL_KEY_WAY_ROUNDABOUT;
    static const std::string LABEL_KEY_WAY_EMBANKMENT;
    static const std::string LABEL_KEY_WAY_MAXSPEED;
    static const std::string LABEL_KEY_WAY_GRADE;
    static const std::string LABEL_KEY_WAY_WIDTH;
    static const std::string LABEL_KEY_WAY_CLOCKWISE;

    static const std::string LABEL_KEY_WAY_LANES_LANES;
    static const std::string LABEL_KEY_WAY_LANES_LANESFORWARD;
    static const std::string LABEL_KEY_WAY_LANES_LANESBACKWARD;
    static const std::string LABEL_KEY_WAY_LANES_TURNFORWARD;
    static const std::string LABEL_KEY_WAY_LANES_TURNBACKWARD;
    static const std::string LABEL_KEY_WAY_LANES_DESTINATIONFORWARD;
    static const std::string LABEL_KEY_WAY_LANES_DESTINATIONBACKWARD;

    static const std::string LABEL_KEY_WAY_SIDEWAYS_CYCLELANE;
    static const std::string LABEL_KEY_WAY_SIDEWAYS_CYCLETRACK;
    static const std::string LABEL_KEY_WAY_SIDEWAYS_WALKTRACK;

    static const std::string LABEL_KEY_WAY_ACCESS_ONEWAY;
    static const std::string LABEL_KEY_WAY_ACCESS_FOOT;
    static const std::string LABEL_KEY_WAY_ACCESS_BICYCLE;
    static const std::string LABEL_KEY_WAY_ACCESS_CAR;

    static const std::string LABEL_KEY_WAY_ACCESSRESTRICTED_FOOT;
    static const std::string LABEL_KEY_WAY_ACCESSRESTRICTED_BICYCLE;
    static const std::string LABEL_KEY_WAY_ACCESSRESTRICTED_CAR;

  private:
    void HandlesLanesFeature(const FeatureValueBuffer &buffer, ObjectDescription &description);
    void HandleSidewayFeature(const FeatureValueBuffer &buffer, ObjectDescription &description);
    void HandleAccessFeature(const FeatureValueBuffer &buffer, ObjectDescription &description);
    void HandelAccessRestricted(const FeatureValueBuffer &buffer, ObjectDescription &description);

  public:
    void Process(const FeatureValueBuffer& buffer,
                 ObjectDescription& description) override;
  };

  /**
   * ingroup Description
   *
   * Information regarding following a defined route
   */
  class RoutingDescriptionProcessor : public FeatureToDescriptionProcessor
  {
  public:
    static const std::string SECTION_NAME_ROUTING;

    static const std::string LABEL_KEY_ROUTING_FROM;
    static const std::string LABEL_KEY_ROUTING_TO;
    static const std::string LABEL_KEY_ROUTING_DESTINATION;

  public:
    void Process(const FeatureValueBuffer& buffer,
                 ObjectDescription& description) override;
  };

  /**
   * ingroup Description
   *
   * Information regarding companies involved in the object
   */
  class CommercialDescriptionProcessor : public FeatureToDescriptionProcessor
  {
  public:
    static const std::string SECTION_NAME_COMMERCIAL;

    static const std::string LABEL_KEY_COMMERCIAL_BRAND;
    static const std::string LABEL_KEY_COMMERCIAL_OPERATOR;
    static const std::string LABEL_KEY_COMMERCIAL_NETWORK;

  public:
    void Process(const FeatureValueBuffer& buffer,
                 ObjectDescription& description) override;
  };

  /**
   * ingroup Description
   *
   * Information regarding payment
   */
  class PaymentDescriptionProcessor : public FeatureToDescriptionProcessor
  {
  public:
    static const std::string SECTION_NAME_PAYMENT;

    static const std::string SUBSECTION_NAME_PAYMENT_FEE;

    static const std::string LABEL_KEY_PAYMENT_FEE_VALUE;
    static const std::string LABEL_KEY_PAYMENT_FEE_CONDITION;

  public:
    void Process(const FeatureValueBuffer& buffer,
                 ObjectDescription& description) override;
  };

  /**
   * ingroup Description
   *
   * Information regarding charging of cars, bikes and similar
   */
  class ChargingStationDescriptionProcessor : public FeatureToDescriptionProcessor
  {
  public:
    static const std::string SECTION_NAME_CHARGINGSTATION;

    static const std::string SUBSECTION_NAME_CHARGINGSTATION_SOCKET;

    static const std::string LABEL_KEY_CHARGINGSTATION_SOCKET_TYPE;
    static const std::string LABEL_KEY_CHARGINGSTATION_SOCKET_CAPACITY;
    static const std::string LABEL_KEY_CHARGINGSTATION_SOCKET_OUTPUT;

  public:
    void Process(const FeatureValueBuffer& buffer,
                 ObjectDescription& description) override;
  };

  /**
   * ingroup Description
   *
   * Information regarding staying at the object
   */
  class PresenceDescriptionProcessor : public FeatureToDescriptionProcessor
  {
  public:
    static const std::string SECTION_NAME_PRESENCE;

    static const std::string SUBSECTION_NAME_PRESENCE_MAXSTAY;

    static const std::string LABEL_KEY_PRESENCE_MAXSTAY_CONDITION;
    static const std::string LABEL_KEY_PRESENCE_MAXSTAY_VALUE;

    static const std::string LABEL_KEY_PRESENCE_OPENINGHOURS;

  public:
    void Process(const FeatureValueBuffer& buffer,
                 ObjectDescription& description) override;
  };

  /**
   * ingroup Description
   *
   * Contact information
   */
  class ContactDescriptionProcessor : public FeatureToDescriptionProcessor
  {
  public:
    static const std::string SECTION_NAME_CONTACT;

    static const std::string LABEL_KEY_CONTACT_PHONE;
    static const std::string LABEL_KEY_CONTACT_WEBSIZE;

  public:
    void Process(const FeatureValueBuffer& buffer,
                 ObjectDescription& description) override;
  };

  /**
   * Service,to get a structured description of an object on the map (Area, Way or Node).
   * Explicitly designed, to offer information within map popup dialogs or similar.
   *
   * Information is return as an ObjectDescription object.
   *
   * The ObjectDescription is more or less a list of DescriptionEntry instances.
   *
   * Each DescriptionEntry holds the following information:
   * * A Section name
   * * An optional subsection name
   * * An optional index, in case where a subsection has multiple instances
   * * a name of a key
   * * a value (always a string)
   *
   * Section, subsection and key names should be defined, that they represent english
   * names and could be directly used in the UI. However, they could also be used as key
   * into a localisation service.
   *
   * Information is gathered by a list of processors, inheriting the FeatureToDescriptionProcessor
   * interface.
   *
   * The processors extract information mainly from the FeatureValueBuffer instance of the object.
   */
  class OSMSCOUT_API DescriptionService CLASS_FINAL
  {
  public:
    static const std::string SECTION_NAME_ID;

    static const std::string LABEL_KEY_ID_KIND;
    static const std::string LABEL_KEY_ID_ID;

  private:
    std::list<FeatureToDescriptionProcessorRef> featureProcessors;

    void GetDescription(const FeatureValueBuffer& buffer,
                        ObjectDescription& description) const;

  public:
    DescriptionService();

    ObjectDescription GetDescription(const FeatureValueBuffer& buffer) const;

    ObjectDescription GetDescription(const Area& area) const;
    ObjectDescription GetDescription(const Way& way) const;
    ObjectDescription GetDescription(const Node& node) const;
  };

  using DescriptionServiceRef = std::shared_ptr<DescriptionService>;
}

#endif // OSMSCOUT_DESCRIPTIONSERVICE_H