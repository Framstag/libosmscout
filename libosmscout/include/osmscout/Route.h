#ifndef OSMSCOUT_ROUTE_H
#define OSMSCOUT_ROUTE_H

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
#include <string>
#include <vector>

#include <osmscout/Path.h>

#include <osmscout/util/HashMap.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  /**
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

  public:
    /*
     * Base class of all descriptions.
     */
    class OSMSCOUT_API Description : public Referencable
    {
    public:
      virtual ~Description();

      virtual std::string GetDebugString() const = 0;
    };

    typedef Ref<Description> DescriptionRef;

    /**
     * Start of the route
     */
    class OSMSCOUT_API StartDescription : public Description
    {
    private:
      std::string description;

    public:
      StartDescription(const std::string& description);

      std::string GetDebugString() const;

      std::string GetDescription() const;
    };

    typedef Ref<StartDescription> StartDescriptionRef;

    /**
     * Start of the route
     */
    class OSMSCOUT_API TargetDescription : public Description
    {
    private:
      std::string description;

    public:
      TargetDescription(const std::string& description);

      std::string GetDebugString() const;

      std::string GetDescription() const;
    };

    typedef Ref<TargetDescription> TargetDescriptionRef;

    /**
     * Something has a name. A name consists of a name and a optional alphanumeric
     * reference (LIke B1 or A40).
     */
    class OSMSCOUT_API NameDescription : public Description
    {
    private:
      std::string name;
      std::string ref;

    public:
      NameDescription(const std::string& name,
                      const std::string& ref);

      std::string GetDebugString() const;

      bool HasName() const;

      std::string GetName() const;
      std::string GetRef() const;

      std::string GetDescription() const;
    };

    typedef Ref<NameDescription> NameDescriptionRef;

    /**
     * Something has a name. A name consists of a name and a optional alphanumeric
     * reference (Like B1 or A40).
     */
    class OSMSCOUT_API NameChangedDescription : public Description
    {
      NameDescriptionRef originDescription;
      NameDescriptionRef targetDescription;

    public:
      NameChangedDescription(NameDescription* originDescription,
                             NameDescription* targetDescription);

      std::string GetDebugString() const;

      inline const NameDescriptionRef& GetOriginDesccription() const
      {
        return originDescription;
      }

      inline const NameDescriptionRef& GetTargetDesccription() const
      {
        return targetDescription;
      }
    };

    typedef Ref<NameChangedDescription> NameChangedDescriptionRef;

    /**
     * List the names of allways, that are crossing the current node.
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
                              NameDescription* originDescription,
                              NameDescription* targetDescription);

      void AddDescription(NameDescription* description);

      std::string GetDebugString() const;

      inline size_t GetExitCount() const
      {
        return exitCount;
      }

      inline bool HasMultipleExits() const
      {
        return exitCount>1;
      }

      inline const NameDescriptionRef& GetOriginDesccription() const
      {
        return originDescription;
      }

      inline const NameDescriptionRef& GetTargetDesccription() const
      {
        return targetDescription;
      }

      inline const std::list<NameDescriptionRef>& GetDescriptions() const
      {
        return descriptions;
      }
    };

    typedef Ref<CrossingWaysDescription> CrossingWaysDescriptionRef;

    /**
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

      std::string GetDebugString() const;

      inline double GetTurnAngle() const
      {
        return turnAngle;
      }

      inline double GetCurveAngle() const
      {
        return curveAngle;
      }

      inline Move GetTurn() const
      {
        return turn;
      }

      inline Move GetCurve() const
      {
        return curve;
      }
    };

    typedef Ref<DirectionDescription> DirectionDescriptionRef;

    /**
     * Signals an explicit turn
     */
    class OSMSCOUT_API TurnDescription : public Description
    {
    public:
      TurnDescription();

      std::string GetDebugString() const;
    };

    typedef Ref<TurnDescription> TurnDescriptionRef;

    /**
     * Signals entering a roundabout
     */
    class OSMSCOUT_API RoundaboutEnterDescription : public Description
    {
    public:
      RoundaboutEnterDescription();

      std::string GetDebugString() const;
    };

    typedef Ref<RoundaboutEnterDescription> RoundaboutEnterDescriptionRef;

    /**
     * Signals leaving a roundabout
     */
    class OSMSCOUT_API RoundaboutLeaveDescription : public Description
    {
    private:
      size_t exitCount;

    public:
      RoundaboutLeaveDescription(size_t exitCount);

      std::string GetDebugString() const;

      inline size_t GetExitCount() const
      {
        return exitCount;
      }
    };

    typedef Ref<RoundaboutLeaveDescription> RoundaboutLeaveDescriptionRef;

    /**
     * Signals entering a motorway
     */
    class OSMSCOUT_API MotorwayEnterDescription : public Description
    {
    private:
      NameDescriptionRef toDescription;

    public:
      MotorwayEnterDescription(NameDescription* toDescription);

      std::string GetDebugString() const;

      inline const NameDescriptionRef& GetToDescription() const
      {
        return toDescription;
      }
    };

    typedef Ref<MotorwayEnterDescription> MotorwayEnterDescriptionRef;

    /**
     * Signals changing a motorway
     */
    class OSMSCOUT_API MotorwayChangeDescription : public Description
    {
    private:
      NameDescriptionRef fromDescription;
      NameDescriptionRef toDescription;

    public:
      MotorwayChangeDescription(NameDescription* fromDescription,
                                NameDescription* toDescription);

      std::string GetDebugString() const;

      inline const NameDescriptionRef& GetFromDescription() const
      {
        return fromDescription;
      }

      inline const NameDescriptionRef& GetToDescription() const
      {
        return toDescription;
      }
    };

    typedef Ref<MotorwayChangeDescription> MotorwayChangeDescriptionRef;

    /**
     * Signals leaving a motorway
     */
    class OSMSCOUT_API MotorwayLeaveDescription : public Description
    {
    private:
      NameDescriptionRef fromDescription;

    public:
      MotorwayLeaveDescription(NameDescription* fromDescription);

      std::string GetDebugString() const;

      inline const NameDescriptionRef& GetFromDescription() const
      {
        return fromDescription;
      }
    };

    typedef Ref<MotorwayLeaveDescription> MotorwayLeaveDescriptionRef;

    class OSMSCOUT_API Node
    {
    private:
      Id                                           currentNodeId;
      std::vector<Path>                            paths;
      Id                                           pathWayId;
      Id                                           targetNodeId;
      double                                       distance;
      double                                       time;
      OSMSCOUT_HASHMAP<std::string,DescriptionRef> descriptionMap;
      std::list<DescriptionRef>                    descriptions;

    public:
      Node(Id currentNodeId,
           const std::vector<Path>& paths,
           Id pathWayId,
           Id targetNodeId);

      inline Id GetCurrentNodeId() const
      {
        return currentNodeId;
      }

      inline const std::vector<Path>& GetPaths() const
      {
        return paths;
      }

      inline const std::list<DescriptionRef>& GetDescriptions() const
      {
        return descriptions;
      }

      inline bool HasPathWay() const
      {
        return pathWayId!=0;
      }

      inline Id GetPathWayId() const
      {
        return pathWayId;
      }

      inline Id GetTargetNodeId() const
      {
        return targetNodeId;
      }

      /**
       * Distance from the start of the route in km.
       */
      inline double GetDistance() const
      {
        return distance;
      }

      /**
       * Time from the start of the route in h.
       */
      inline double GetTime() const
      {
        return time;
      }

      bool HasDescription(const char* name) const;
      Description* GetDescription(const char* name) const;

      void SetDistance(double distance);
      void SetTime(double time);

      void AddDescription(const char* name, Description* description);
    };

  private:
    std::list<Node> nodes;

  public:
    RouteDescription();
    virtual ~RouteDescription();

    void Clear();

    void AddNode(Id currentNodeId,
                 const std::vector<Path>& paths,
                 Id pathWayId,
                 Id targetNodeId);
  
    inline std::list<Node>& Nodes()
    {
      return nodes;
    }
  };
}

#endif
