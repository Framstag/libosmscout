package net.sf.libosmscout;

import net.sf.libosmscout.osmscout.CarSpeedMap;
import net.sf.libosmscout.osmscout.Database;
import net.sf.libosmscout.osmscout.DatabaseParameter;
import net.sf.libosmscout.osmscout.FastestPathRoutingProfile;
import net.sf.libosmscout.osmscout.ObjectFileRef;
import net.sf.libosmscout.osmscout.RefType;
import net.sf.libosmscout.osmscout.RouteData;
import net.sf.libosmscout.osmscout.RouteDescription;
import net.sf.libosmscout.osmscout.RoutePostprocessor;
import net.sf.libosmscout.osmscout.RouterParameter;
import net.sf.libosmscout.osmscout.RoutingService;
import net.sf.libosmscout.osmscout.SWIGTYPE_p_size_t;
import net.sf.libosmscout.osmscout.Vehicle;

public class Routing {

	static {
		System.loadLibrary("osmscoutjava");
	}

	private static CarSpeedMap getCarSpeedMap() {
		CarSpeedMap map = new CarSpeedMap();

		map.set("highway_motorway", 110.0);
		map.set("highway_motorway_trunk", 100.0);
		map.set("highway_motorway_primary", 70.0);
		map.set("highway_motorway_link", 60.0);
		map.set("highway_motorway_junction", 60.0);
		map.set("highway_trunk", 100.0);
		map.set("highway_trunk_link", 60.0);
		map.set("highway_primary", 70.0);
		map.set("highway_primary_link", 60.0);
		map.set("highway_secondary", 60.0);
		map.set("highway_secondary_link", 50.0);
		map.set("highway_tertiary_link", 55.0);
		map.set("highway_tertiary", 55.0);
		map.set("highway_unclassified", 50.0);
		map.set("highway_road", 50.0);
		map.set("highway_residential", 40.0);
		map.set("highway_roundabout", 40.0);
		map.set("highway_living_street", 10.0);
		map.set("highway_service", 30.0);

		return map;
	}

	public static void main(String[] args) {
		if (args.length != 1) {
			System.err.println("Routing <map directory>");
			return;
		}

		String mapDirectory = args[0];

		// TODO: Argument parsing

		String routingFilenameBase = RoutingService.getDEFAULT_FILENAME_BASE();

		double startLat = 51.5717798;
		double startLon = 7.4587852;

		double targetLat = 50.6890143;
		double targetLon = 7.1360549;

		Vehicle vehicle = Vehicle.vehicleCar;

		CarSpeedMap carSpeedMap = getCarSpeedMap();

		DatabaseParameter parameter = new DatabaseParameter();
		Database database = new Database(parameter);

		System.out.println("Opening database '" + mapDirectory + "'...");

		if (!database.Open(mapDirectory)) {
			System.err.println("Cannot open database '" + mapDirectory + "'");
			return;
		}

		FastestPathRoutingProfile routingProfile = new FastestPathRoutingProfile(database.GetTypeConfig());
		RouterParameter routingParameter = new RouterParameter();

		routingParameter.SetDebugPerformance(true);

		RoutingService router = new RoutingService(database, routingParameter, routingFilenameBase);

		if (!router.Open()) {
			System.err.println("Cannot open router");
			database.Close();
			return;
		}

		// Car does not work, because of std::map missing
		routingProfile.ParametrizeForCar(database.GetTypeConfig(), carSpeedMap, 160.0);

		ObjectFileRef startObject = new ObjectFileRef();
		int[] startNodeIndex = new int[1];

		if (!router.GetClosestRoutableNode(startLat, startLon, vehicle, 1000, startObject, startNodeIndex)) {
			System.err.println("Error while searching for routing node for start");
			database.Close();
			return;
		}

		if (startObject.Invalid() || startObject.GetType() == RefType.refNode) {
			System.err.println("Cannot find routing node for start");
			database.Close();
			return;
		}

		System.out.println("Route node for start found...");
		
		ObjectFileRef targetObject = new ObjectFileRef();
		int[] targetNodeIndex = new int[1];

		if (!router.GetClosestRoutableNode(targetLat, targetLon, vehicle, 1000, targetObject, targetNodeIndex)) {
			System.err.println("Error while searching for routing node for target");
			database.Close();
			return;
		}

		if (targetObject.Invalid() || targetObject.GetType() == RefType.refNode) {
			System.err.println("Cannot find routing node for target");
			database.Close();
			return;
		}

		System.out.println("Route node for target found...");
		
		RouteData data = new RouteData();

		if (!router.CalculateRoute(routingProfile, startObject, startNodeIndex[0], targetObject, targetNodeIndex[0],
				data)) {
			System.err.println("There was an error while calculating the route!");
			router.Close();
			return;
		}

		if (data.IsEmpty()) {
			System.out.println("No Route found!");

			router.Close();

			return;
		}

		System.out.println("Route found...");

		RouteDescription description = new RouteDescription(); 
		
		router.TransformRouteDataToRouteDescription(data, description);
		
		RoutePostprocessor postprocessor = new RoutePostprocessor();
		
		//postprocessor.PostprocessRouteDescription(description, profile, database, processors)
		
		database.Close();
	}
}
