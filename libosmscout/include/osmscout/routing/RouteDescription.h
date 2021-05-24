#ifndef OSMSCOUT_ROUTE_DESCRIPTION_H
#define OSMSCOUT_ROUTE_DESCRIPTION_H

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
#include <vector>
#include <map>

#include <osmscout/ObjectRef.h>
#include <osmscout/Path.h>
#include <osmscout/GeoCoord.h>
#include <osmscout/routing/DBFileOffset.h>
#include <osmscout/util/Distance.h>
#include <osmscout/util/Time.h>

namespace osmscout {

  /**
   * \ingroup Routing
   * Description of a route, enhanced with information that are required to
   * give a human textual (or narrative) drive instructions;
   *
   * A route consists of nodes. A Node can be the crossing point of a number of
   * ways and is a route decision point (where the driver possibly has the change ways)
   * that requires some potential action by the driver.
   *
   * For each node you can pass a number of descriptions. For the way from the current node
   * to the next node also a number of descriptions can get retrieved.
   *
   * Descriptions are typed and must derive from class Description..
   */
  class OSMSCOUT_API RouteDescription
  {
  public:
    /** Constant for a description of the start node (StartDescription) */
    static const char* const NODE_START_DESC;
    /** Constant for a description of the target node (TargetDescription) */
    static const char* const NODE_TARGET_DESC;
    /** Constant for a description of name of the way (NameDescription) */
    static const char* const WAY_NAME_DESC;
    /** Constant for a description of a change of way name (NameChangedDescription) */
    static const char* const WAY_NAME_CHANGED_DESC;
    /** Constant for a description of list of way name crossing a node (CrossingWaysDescription) */
    static const char* const CROSSING_WAYS_DESC;
    /** Constant for a description of drive direction (DirectionDescription) */
    static const char* const DIRECTION_DESC;
    /** Constant for a description of an explicit turn (TurnDescription) */
    static const char* const TURN_DESC;
    /** Constant for a description of entering a roundabout  (RoundaboutEnterDescription) */
    static const char* const ROUNDABOUT_ENTER_DESC;
    /** Constant for a description of leaving a roundabout (RoundaboutLeaveDescription) */
    static const char* const ROUNDABOUT_LEAVE_DESC;
    /** Constant for a description of entering a motorway (MotorwayEnterDescription) */
    static const char* const MOTORWAY_ENTER_DESC;
    /** Constant for a description of changing a motorway (MotorwayChangeDescription) */
    static const char* const MOTORWAY_CHANGE_DESC;
    /** Constant for a description of leaving a motorway (MotorwayLeaveDescription) */
    static const char* const MOTORWAY_LEAVE_DESC;
    /** Constant for a description of node describing a motorway junction */
    static const char* const MOTORWAY_JUNCTION_DESC;
    /** Constant for a description of a destination to choose at a junction */
    static const char* const CROSSING_DESTINATION_DESC;
    /** Constant for a description of the maximum speed for the given way */
    static const char* const WAY_MAXSPEED_DESC;
    /** Constant for a description of type name of the way (TypeNameDescription) */
    static const char* const WAY_TYPE_NAME_DESC;
    /** Constant for a description of pois at the route (POIAtRouteDescription) */
    static const char* const POI_AT_ROUTE_DESC;
    /** Constant for a description of route lanes (LaneDescription) */
    static const char* const LANES_DESC;
    /** Constant for a description of suggested route lanes (SuggestedLaneDescription) */
    static const char* const SUGGESTED_LANES_DESC;

  public:
    /**
     * \ingroup Routing
     * Base class of all descriptions.
     */
    class OSMSCOUT_API Description
    {
    public:
      virtual ~Description() = default;

      virtual std::string GetDebugString() const = 0;
    };

    using DescriptionRef = std::shared_ptr<Description>;

    /**
     * \ingroup Routing
     * Start of the route
     */
    class OSMSCOUT_API StartDescription : public Description
    {
    private:
      std::string description;

    public:
      explicit StartDescription(const std::string& description);

      std::string GetDebugString() const override;

      std::string GetDescription() const;
    };

    using StartDescriptionRef = std::shared_ptr<StartDescription>;

    /**
     * \ingroup Routing
     * Target of the route
     */
    class OSMSCOUT_API TargetDescription : public Description
    {
    private:
      std::string description;

    public:
      explicit TargetDescription(const std::string& description);

      std::string GetDebugString() const override;

      std::string GetDescription() const;
    };

    using TargetDescriptionRef = std::shared_ptr<TargetDescription>;

    /**
     * \ingroup Routing
     * Something has a name. A name consists of a name and a optional alphanumeric
     * reference (Like B1 or A40).
     */
    class OSMSCOUT_API NameDescription : public Description
    {
    private:
      std::string name;
      std::string ref;

    public:
      explicit NameDescription(const std::string& name);

      NameDescription(const std::string& name,
                      const std::string& ref);

      std::string GetDebugString() const override;

      bool HasName() const;

      std::string GetName() const;
      std::string GetRef() const;

      std::string GetDescription() const;
    };

    using NameDescriptionRef = std::shared_ptr<NameDescription>;

    /**
     * \ingroup Routing
     * Something has a name. A name consists of a name and a optional alphanumeric
     * reference (Like B1 or A40).
     */
    class OSMSCOUT_API NameChangedDescription : public Description
    {
      NameDescriptionRef originDescription;
      NameDescriptionRef targetDescription;

    public:
      NameChangedDescription(const NameDescriptionRef& originDescription,
                             const NameDescriptionRef& targetDescription);

      std::string GetDebugString() const override;

      NameDescriptionRef GetOriginDescription() const
      {
        return originDescription;
      }

      NameDescriptionRef GetTargetDescription() const
      {
        return targetDescription;
      }
    };

    using NameChangedDescriptionRef = std::shared_ptr<NameChangedDescription>;

    /**
     * \ingroup Routing
     * List the names of all ways, that are crossing the current node.
     */
    class OSMSCOUT_API CrossingWaysDescription : public Description
    {
    private:
      size_t                        exitCount;
      NameDescriptionRef            originDescription;
      NameDescriptionRef            targetDescription;
      std::list<NameDescriptionRef> descriptions;

    public:
      CrossingWaysDescription(size_t exitCount,
                              const NameDescriptionRef& originDescription,
                              const NameDescriptionRef& targetDescription);

      void AddDescription(const NameDescriptionRef& description);

      std::string GetDebugString() const override;

      size_t GetExitCount() const
      {
        return exitCount;
      }

      bool HasMultipleExits() const
      {
        return exitCount>1;
      }

      NameDescriptionRef GetOriginDesccription() const
      {
        return originDescription;
      }

      NameDescriptionRef GetTargetDesccription() const
      {
        return targetDescription;
      }

      const std::list<NameDescriptionRef>& GetDescriptions() const
      {
        return descriptions;
      }
    };

    using CrossingWaysDescriptionRef = std::shared_ptr<CrossingWaysDescription>;

    /**
     * \ingroup Routing
     * Describes the turn and the curve while getting from the previous node to the next node via the current node.
     *
     * The turn is the angle between the incoming way (previous node and current node)
     * and the outgoing way (current node and next node) at the given node.
     *
     * The curve is a heuristic measurement that not only take the next node of the target way into
     * account (which could only the start of a slight curve) but tries to determine the last node
     * of the curve and this gives a better description of the curve the vehicle needs to take.
     */
    class OSMSCOUT_API DirectionDescription : public Description
    {
    public:
      enum Move {
        sharpLeft,
        left,
        slightlyLeft,
        straightOn,
        slightlyRight,
        right,
        sharpRight
      };

    private:
      double turnAngle;
      double curveAngle;
      Move   turn;
      Move   curve;

    private:
      Move ConvertAngleToMove(double angle) const;
      std::string ConvertMoveToString(Move move) const;

    public:
      DirectionDescription(double turnAngle,
                      double curveAngle);

      std::string GetDebugString() const override;

      double GetTurnAngle() const
      {
        return turnAngle;
      }

      double GetCurveAngle() const
      {
        return curveAngle;
      }

      Move GetTurn() const
      {
        return turn;
      }

      Move GetCurve() const
      {
        return curve;
      }
    };

    using DirectionDescriptionRef = std::shared_ptr<DirectionDescription>;

    /**
     * \ingroup Routing
     * Signals an explicit turn
     */
    class OSMSCOUT_API TurnDescription : public Description
    {
    public:
      std::string GetDebugString() const override;
    };

    using TurnDescriptionRef = std::shared_ptr<TurnDescription>;

    /**
     * \ingroup Routing
     * Signals entering a roundabout
     */
    class OSMSCOUT_API RoundaboutEnterDescription : public Description
    {
    private:
      bool clockwise;

    public:
      explicit RoundaboutEnterDescription(bool clockwise);

      std::string GetDebugString() const override;

      bool IsClockwise() const
      {
        return clockwise;
      }
    };

    using RoundaboutEnterDescriptionRef = std::shared_ptr<RoundaboutEnterDescription>;

    /**
     * \ingroup Routing
     * Signals leaving a roundabout
     */
    class OSMSCOUT_API RoundaboutLeaveDescription : public Description
    {
    private:
      size_t exitCount;
      bool clockwise;

    public:
      RoundaboutLeaveDescription(size_t exitCount, bool clockwise);

      std::string GetDebugString() const override;

      size_t GetExitCount() const
      {
        return exitCount;
      }

      bool IsClockwise() const
      {
        return clockwise;
      }
    };

    using RoundaboutLeaveDescriptionRef = std::shared_ptr<RoundaboutLeaveDescription>;

    /**
     * \ingroup Routing
     * Signals entering a motorway
     */
    class OSMSCOUT_API MotorwayEnterDescription : public Description
    {
    private:
      NameDescriptionRef toDescription;

    public:
      explicit MotorwayEnterDescription(const NameDescriptionRef& toDescription);

      std::string GetDebugString() const override;

      NameDescriptionRef GetToDescription() const
      {
        return toDescription;
      }
    };

    using MotorwayEnterDescriptionRef = std::shared_ptr<MotorwayEnterDescription>;

    /**
     * \ingroup Routing
     * Signals changing a motorway
     */
    class OSMSCOUT_API MotorwayChangeDescription : public Description
    {
    private:
      NameDescriptionRef fromDescription;
      NameDescriptionRef toDescription;

    public:
      MotorwayChangeDescription(const NameDescriptionRef& fromDescription,
                                const NameDescriptionRef& toDescription);

      std::string GetDebugString() const override;

      NameDescriptionRef GetFromDescription() const
      {
        return fromDescription;
      }

      NameDescriptionRef GetToDescription() const
      {
        return toDescription;
      }
    };

    using MotorwayChangeDescriptionRef = std::shared_ptr<MotorwayChangeDescription>;

    /**
     * \ingroup Routing
     * Signals leaving a motorway
     */
    class OSMSCOUT_API MotorwayLeaveDescription : public Description
    {
    private:
      NameDescriptionRef fromDescription;

    public:
      explicit MotorwayLeaveDescription(const NameDescriptionRef& fromDescription);

      std::string GetDebugString() const override;

      NameDescriptionRef GetFromDescription() const
      {
        return fromDescription;
      }
    };

    using MotorwayLeaveDescriptionRef = std::shared_ptr<MotorwayLeaveDescription>;

    /**
     * \ingroup Routing
     * A motorway junction
     */
    class OSMSCOUT_API MotorwayJunctionDescription : public Description
    {
    private:
      NameDescriptionRef junctionDescription;

    public:
      explicit MotorwayJunctionDescription(const NameDescriptionRef& junctionDescription);

      std::string GetDebugString() const override;

      NameDescriptionRef GetJunctionDescription() const
      {
        return junctionDescription;
      }
    };

    using MotorwayJunctionDescriptionRef = std::shared_ptr<MotorwayJunctionDescription>;

    /**
     * \ingroup Routing
     * Destination of the route
     */
    class OSMSCOUT_API DestinationDescription : public Description
    {
    private:
      std::string description;

    public:
      explicit DestinationDescription(const std::string& description);

      std::string GetDebugString() const override;

      std::string GetDescription() const;
    };

    using DestinationDescriptionRef = std::shared_ptr<DestinationDescription>;

    /**
     * \ingroup Routing
     * A motorway junction
     */
    class OSMSCOUT_API MaxSpeedDescription : public RouteDescription::Description
    {
    private:
      uint8_t maxSpeed;

    public:
      explicit MaxSpeedDescription(uint8_t speed);

      std::string GetDebugString() const override;

      uint8_t GetMaxSpeed() const
      {
        return maxSpeed;
      }
    };

    using MaxSpeedDescriptionRef = std::shared_ptr<MaxSpeedDescription>;

    /**
     * \ingroup Routing
     * Something has a type name. This is the name of the type of the way used.
     */
    class OSMSCOUT_API TypeNameDescription : public Description
    {
    private:
      std::string name;

    public:
      explicit TypeNameDescription(const std::string& name);

      std::string GetDebugString() const override;

      bool HasName() const;

      std::string GetName() const;

      std::string GetDescription() const;
    };

    using TypeNameDescriptionRef = std::shared_ptr<TypeNameDescription>;

    /**
     * \ingroup Routing
     * A motorway junction
     */
    class OSMSCOUT_API POIAtRouteDescription : public RouteDescription::Description
    {
    private:
      DatabaseId         databaseId;
      ObjectFileRef      object;
      NameDescriptionRef name;
      Distance           distance;

    public:
      explicit POIAtRouteDescription(DatabaseId databaseId,
                                     const ObjectFileRef& object,
                                     const NameDescriptionRef& name,
                                     const Distance& distance);

      std::string GetDebugString() const override;

      DatabaseId GetDatabaseId() const
      {
        return databaseId;
      }

      ObjectFileRef GetObject() const
      {
        return object;
      }

      NameDescriptionRef GetName() const
      {
        return name;
      }

      Distance GetDistance() const
      {
        return distance;
      }
    };

    using POIAtRouteDescriptionRef = std::shared_ptr<POIAtRouteDescription>;

    /**
     * \ingroup Routing
     * A route lane
     */
    class OSMSCOUT_API LaneDescription : public RouteDescription::Description
    {
    private:
      bool oneway{false};
      uint8_t laneCount{1}; //!< in our direction, not sum on way

      /**
       * turns in lanes from left one (drivers view)
       * vector size may be less than laneCount, even empty
       *
       * usual variants:
       *    left, slight_left, merge_to_left,
       *    through;left, through;slight_left, through;sharp_left,
       *    through,
       *    through;right, through;slight_right, through;sharp_right,
       *    right, slight_right, merge_to_right
       */
      std::vector<std::string> laneTurns;

    public:
      LaneDescription(bool oneway,
                      uint8_t laneCount,
                      const std::vector<std::string> &laneTurns);

      std::string GetDebugString() const override;

      bool IsOneway() const
      {
        return oneway;
      }

      uint8_t GetLaneCount() const
      {
        return laneCount;
      }

      const std::vector<std::string>& GetLaneTurns() const
      {
        return laneTurns;
      }

      bool operator==(const LaneDescription &o) const;
      bool operator!=(const LaneDescription &o) const;
    };

    using LaneDescriptionRef = std::shared_ptr<LaneDescription>;

    /**
     * \ingroup Routing
     *
     * A suggested route lanes. It specifies range of lanes <from, to> that drive
     * should use. Lanes are counted from left (just route direction, not opposite direction),
     * left-most lane has index 0, both indexes are inclusive.
     */
    class OSMSCOUT_API SuggestedLaneDescription : public RouteDescription::Description
    {
    private:
      uint8_t from = uint8_t(-1); //!< left-most suggested lane, inclusive
      uint8_t to = uint8_t(-1); //!< right-most suggested lane, inclusive

    public:
      SuggestedLaneDescription(uint8_t from, uint8_t to);

      std::string GetDebugString() const override;

      uint8_t GetFrom() const
      {
        return from;
      }

      uint8_t GetTo() const
      {
        return to;
      }
    };

    using SuggestedLaneDescriptionRef = std::shared_ptr<SuggestedLaneDescription>;

    /**
     * \ingroup Routing
     */
    class OSMSCOUT_API Node
    {
    private:
      DatabaseId                                     database; //!< database id of objects and pathObject
      size_t                                         currentNodeIndex; //!< current node index of pathObject
      std::vector<ObjectFileRef>                     objects; //!< list of objects intersecting this node. Is empty when node belongs to pathObject only
      ObjectFileRef                                  pathObject; //!< object used for traveling from this node. Is invalid for last node
      size_t                                         targetNodeIndex; //!< target node index of pathObject
      Distance                                       distance; //!< distance from route start
      Duration                                       time; //!< time from route start
      GeoCoord                                       location; //!< geographic coordinate of node
      std::unordered_map<std::string,DescriptionRef> descriptionMap;
      std::list<DescriptionRef>                      descriptions;

    public:
      Node(DatabaseId database,
           size_t currentNodeIndex,
           const std::vector<ObjectFileRef>& objects,
           const ObjectFileRef& pathObject,
           size_t targetNodeIndex);

      size_t GetCurrentNodeIndex() const
      {
        return currentNodeIndex;
      }

      /**
       * Return the objects that intersect at the current node index.
       */
      const std::vector<ObjectFileRef>& GetObjects() const
      {
        return objects;
      }

      /**
       * Return a list of descriptions attached to the current node
       */
      const std::list<DescriptionRef>& GetDescriptions() const
      {
        return descriptions;
      }

      /**
       * There exists a object/path from the current node to the next node
       * in the route.
       */
      bool HasPathObject() const
      {
        return pathObject.Valid();
      }

      DatabaseId GetDatabaseId() const
      {
        return database;
      }

      DBFileOffset GetDBFileOffset() const
      {
        return DBFileOffset(GetDatabaseId(),GetPathObject().GetFileOffset());
      }

      /**
       * Return the path object that connects the current node to the next node.
       */
      ObjectFileRef GetPathObject() const
      {
        return pathObject;
      }

      /**
       * The the index of the target node on the path that is the next node on the route.
       */
      size_t GetTargetNodeIndex() const
      {
        return targetNodeIndex;
      }

      /**
       * Distance from the start of the route.
       */
      Distance GetDistance() const
      {
        return distance;
      }

      /**
       * Time from the start of the route in h.
       */
      Duration GetTime() const
      {
        return time;
      }

      /**
       * Location (latitude,longitude) of the node
       */
      GeoCoord GetLocation() const
      {
        return location;
      }

      bool HasDescription(const char* name) const;
      DescriptionRef GetDescription(const char* name) const;

      void SetDistance(Distance distance);
      void SetTime(const Timestamp::duration &time);
      void SetLocation(const GeoCoord &coord);

      void AddDescription(const char* name,
                          const DescriptionRef& description);
    };

    using NodeIterator = std::list<RouteDescription::Node>::const_iterator;

  private:
    std::list<Node> nodes;
    std::map<DatabaseId, std::string> databaseMapping;

  public:
    RouteDescription() = default;
    virtual ~RouteDescription() = default;

    void SetDatabaseMapping(std::map<DatabaseId, std::string> databaseMapping);

    std::map<DatabaseId, std::string> GetDatabaseMapping() const;

    void Clear();

    bool Empty() const;

    void AddNode(DatabaseId database,
                 size_t currentNodeIndex,
                 const std::vector<ObjectFileRef>& objects,
                 const ObjectFileRef& pathObject,
                 size_t targetNodeIndex);

    std::list<Node>& Nodes()
    {
      return nodes;
    }

    const std::list<Node>& Nodes() const
    {
      return nodes;
    }
  };

  using RouteDescriptionRef = std::shared_ptr<RouteDescription>;
}

#endif
