package com.framstag.libosmscout.client;

/**
 * A language-agnostic record of one manoeuvre of a route.
 * <p>
 * Bridged from C++ {@code osmscout::client::RouteStep} via JNI. A step carries every description
 * the route description attaches to its route node, the identity of that node and its position
 * along the route.
 * <p>
 * <strong>All values are untranslated map data</strong>: no sentences, no translated words, no
 * units and no number formatting. Every label a user reads is produced by the client that displays
 * the step, in its own language and unit system.
 * <p>
 * A description that the route does not carry for the node is absent: the sub-record is
 * {@code null} instead of an empty or zero-valued placeholder. A value that can be absent on its
 * own (a max speed, the remaining distance of a live step, the turn of a node without a turn
 * description) is a boxed type and {@code null} when unknown.
 * <p>
 * Live guidance extends the same record with {@link #remainingDistance}, {@link #remainingTime}
 * and {@link #nextNext}; a step of a published route list leaves them {@code null}.
 */
public class RouteStep {

    /** The manoeuvre a step describes, derived by the producer for icons and row selection. */
    public enum Kind {
        /** No manoeuvre kind was derived. */
        UNKNOWN,
        /** The route's start. */
        START,
        /** The route's destination. */
        TARGET,
        /** A turn. */
        TURN,
        /** Entering a roundabout. */
        ROUNDABOUT_ENTER,
        /** Leaving a roundabout. */
        ROUNDABOUT_LEAVE,
        /** Entering a motorway. */
        MOTORWAY_ENTER,
        /** Changing to another motorway. */
        MOTORWAY_CHANGE,
        /** Leaving a motorway. */
        MOTORWAY_LEAVE,
        /** The way's name changes. */
        NAME_CHANGE,
        /** A node that only carries facts (a max speed, lanes, a POI, a via section). */
        INFORMATIVE
    }

    /** A name with its optional alphanumeric reference, like {@code B1} or {@code A40}. */
    public static class Name {
        /** The name as it exists in the map data, without quotes. */
        public String name = "";
        /** The reference as it exists in the map data, without brackets. */
        public String ref = "";

        /** Default constructor. */
        public Name() {
        }

        /**
         * Construct a name.
         *
         * @param name the name, may be empty
         * @param ref  the reference, may be empty
         */
        public Name(String name, String ref) {
            this.name = name != null ? name : "";
            this.ref = ref != null ? ref : "";
        }
    }

    /** The way's name changes from {@link #from} to {@link #to} at this node. */
    public static class NameChanged {
        /** The name of the way before the change. */
        public Name from = new Name();
        /** The name of the way after the change. */
        public Name to = new Name();
    }

    /** The ways that cross the route at this node. */
    public static class CrossingWays {
        /** Number of exits at this crossing node. */
        public long exitCount;
        /** Name of the way the route comes from, {@code null} when unknown. */
        public Name origin;
        /** Name of the way the route turns into, {@code null} when unknown. */
        public Name target;
        /** Names of the other ways crossing here, empty when there are none. */
        public Name[] others = new Name[0];
    }

    /** The turn and the curve while getting from the previous node to the next node. */
    public static class Direction {
        /** Angle of the turn in degrees. */
        public double turnAngle;
        /** Angle of the curve in degrees. */
        public double curveAngle;
        /** Turn direction of the angle. */
        public TurnType turn = TurnType.STRAIGHT_ON;
        /** Direction of the curve. */
        public TurnType curve = TurnType.STRAIGHT_ON;
    }

    /** Entering a roundabout. */
    public static class RoundaboutEnter {
        /** Whether the roundabout's traffic circulates clockwise. */
        public boolean clockwise;
    }

    /** Leaving a roundabout. */
    public static class RoundaboutLeave {
        /** 1-based ordinal, in circulation order, of the exit taken. */
        public long exitCountTaken;
        /** Whether the roundabout's traffic circulates clockwise. */
        public boolean clockwise;
    }

    /** Entering a motorway. */
    public static class MotorwayEnter {
        /** The motorway entered. */
        public Name to = new Name();
    }

    /** Changing to another motorway. */
    public static class MotorwayChange {
        /** The motorway left. */
        public Name from = new Name();
        /** The motorway entered. */
        public Name to = new Name();
        /** Direction of the change. */
        public TurnType move = TurnType.STRAIGHT_ON;
    }

    /** Leaving a motorway. */
    public static class MotorwayLeave {
        /** The motorway left. */
        public Name from = new Name();
        /** Direction of the leave. */
        public TurnType move = TurnType.STRAIGHT_ON;
    }

    /** The lanes of the way, from the driver's view. */
    public static class Lane {
        /** Whether the way is one-way. */
        public boolean oneway;
        /** Number of lanes in the direction of travel, not the sum of both directions. */
        public int count;
        /** Turns of the lanes, from the left, own direction; may be shorter than {@link #count}. */
        public LaneTurn[] turns = new LaneTurn[0];
    }

    /** Lanes suggested for the manoeuvre ahead. */
    public static class SuggestedLane {
        /** Left-most suggested lane, inclusive, counted from the left. */
        public int from;
        /** Right-most suggested lane, inclusive. */
        public int to;
        /** Turn of the suggested lanes. */
        public LaneTurn turn = LaneTurn.NONE;
    }

    /** A point of interest on the route. */
    public static class Poi {
        /** Database the object belongs to. */
        public long databaseId;
        /** File offset of the object. */
        public long objectOffset;
        /** Type of the object (1 node, 2 area, 3 way). */
        public int objectType;
        /** Name of the POI, {@code null} when unknown. */
        public Name name;
        /** Distance of the POI from the route start in meters. */
        public double distance;
    }

    /** A section of the route between two via points. */
    public static class Via {
        /** Number of the section, starting at 1. */
        public int sectionNumber;
        /** Number of route nodes of the section. */
        public int nodeCount;
    }

    /** The manoeuvre kind, derived for icons and row selection. */
    public Kind kind = Kind.UNKNOWN;

    /** Database of the node and of its path object. */
    public long databaseId;
    /** Node index of {@link #pathObjectOffset} this step's node is. */
    public long currentNodeIndex;
    /** File offset of the object travelled along from this node. */
    public long pathObjectOffset;
    /** Object type of the path object (1 node, 2 area, 3 way). */
    public int pathObjectType;
    /** Target node index of the path object. */
    public long targetNodeIndex;

    /** Latitude of the node in degrees. */
    public double lat;
    /** Longitude of the node in degrees. */
    public double lon;
    /** Distance of the node from the route start in meters. */
    public double distanceFromStart;
    /** Time of the node from the route start in seconds. */
    public double timeFromStart;

    /** File offsets of the objects that intersect the node. */
    public long[] objectOffsets = new long[0];
    /** Object types of those objects, index-aligned with {@link #objectOffsets}. */
    public int[] objectTypes = new int[0];

    /** Distance of the leg that ends at this step in meters. */
    public double legDistance;
    /** Time of that leg in seconds. */
    public double legTime;

    /** Name of the way at this node, {@code null} when absent. */
    public Name name;
    /** A name change at this node, {@code null} when absent. */
    public NameChanged nameChanged;
    /** Type name of the way (for example {@code highway_secondary}), {@code null} when absent. */
    public String typeName;
    /** The ways crossing the node, {@code null} when absent. */
    public CrossingWays crossingWays;
    /** Turn and curve, {@code null} when absent. */
    public Direction direction;
    /** An explicit turn, {@code null} when absent. */
    public TurnType turn;
    /** Entering a roundabout, {@code null} when absent. */
    public RoundaboutEnter roundaboutEnter;
    /** Leaving a roundabout, {@code null} when absent. */
    public RoundaboutLeave roundaboutLeave;
    /** Entering a motorway, {@code null} when absent. */
    public MotorwayEnter motorwayEnter;
    /** Changing motorways, {@code null} when absent. */
    public MotorwayChange motorwayChange;
    /** Leaving a motorway, {@code null} when absent. */
    public MotorwayLeave motorwayLeave;
    /** The motorway junction with its name and exit reference, {@code null} when absent. */
    public Name junction;
    /**
     * The destination sign text as the raw tag, semicolon separated, {@code null} when absent.
     * Splitting it is the client's job.
     */
    public String destination;
    /** Maximum speed in km/h, {@code null} when unknown. */
    public Integer maxSpeed;
    /** The lanes of the way, {@code null} when absent. */
    public Lane lanes;
    /** Suggested lanes, {@code null} when absent. */
    public SuggestedLane suggestedLanes;
    /** A POI on the route, {@code null} when absent. */
    public Poi poi;
    /** A via section at this node, {@code null} when absent. */
    public Via via;
    /** The start label, {@code null} when absent. */
    public String start;
    /** The destination label, {@code null} when absent. */
    public String target;

    /** Remaining distance to this manoeuvre in meters; live guidance only, else {@code null}. */
    public Double remainingDistance;
    /** Remaining time to this manoeuvre in seconds; live guidance only, else {@code null}. */
    public Double remainingTime;
    /** The following manoeuvre when it is close enough to be announced, else {@code null}. */
    public RouteStep nextNext;

    /** Default constructor. */
    public RouteStep() {
    }
}
