package net.sf.libosmscout;

import net.sf.libosmscout.osmscout.*;

public class Routing {

	static {
		System.loadLibrary("osmscout_binding_java");
	}

	private static CarSpeedMap getCarSpeedMap() {
		CarSpeedMap map = new CarSpeedMap();

		map.put("highway_motorway", 110.0);
		map.put("highway_motorway_trunk", 100.0);
		map.put("highway_motorway_primary", 70.0);
		map.put("highway_motorway_link", 60.0);
		map.put("highway_motorway_junction", 60.0);
		map.put("highway_trunk", 100.0);
		map.put("highway_trunk_link", 60.0);
		map.put("highway_primary", 70.0);
		map.put("highway_primary_link", 60.0);
		map.put("highway_secondary", 60.0);
		map.put("highway_secondary_link", 50.0);
		map.put("highway_tertiary_link", 55.0);
		map.put("highway_tertiary", 55.0);
		map.put("highway_unclassified", 50.0);
		map.put("highway_road", 50.0);
		map.put("highway_residential", 40.0);
		map.put("highway_roundabout", 40.0);
		map.put("highway_living_street", 10.0);
		map.put("highway_service", 30.0);

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

		GeoCoord start = new GeoCoord(51.5717798,7.4587852);
		GeoCoord target = new GeoCoord(50.6890143,7.1360549);

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

		if (vehicle == Vehicle.vehicleCar) {
				routingProfile.ParametrizeForCar(database.GetTypeConfig(),carSpeedMap,160.0);
		}

		RouterParameter routerParameter = new RouterParameter();

		routerParameter.SetDebugPerformance(true);

		RoutingParameter routingParameter = new RoutingParameter();

		SimpleRoutingService router = new SimpleRoutingService(database, routerParameter, routingFilenameBase);

		if (!router.Open()) {
			System.err.println("Cannot open router");
			database.Close();
			return;
		}

		// Car does not work, because of std::map missing
		routingProfile.ParametrizeForCar(database.GetTypeConfig(), carSpeedMap, 160.0);

		RoutePositionResult routeStart = router.GetClosestRoutableNode(start, routingProfile, libosmscout.Meters(1000));

		if (!routeStart.IsValid()) {
			System.err.println("Error while searching for routing node for start");
			database.Close();
			return;
		}

		if (routeStart.GetRoutePosition().GetObjectFileRef().GetType() == RefType.refNode) {
			System.err.println("Cannot find routing node for start");
			database.Close();
			return;
		}

		System.out.println("Route node for start found...");
		
		RoutePositionResult routeTarget = router.GetClosestRoutableNode(target, routingProfile, libosmscout.Meters(1000));

		if (!routeTarget.IsValid()) {
			System.err.println("Error while searching for routing node for target");
			database.Close();
			return;
		}

		if (routeTarget.GetRoutePosition().GetObjectFileRef().GetType() == RefType.refNode) {
			System.err.println("Cannot find routing node for target");
			database.Close();
			return;
		}

		System.out.println("Route node for target found...");

		System.out.println("Calculating route...");

		RoutingResult routingResult = router.CalculateRoute(routingProfile, routeStart.GetRoutePosition(), routeTarget.GetRoutePosition(),
			routingParameter);

		if (!routingResult.Success()) {
			System.err.println("There was an error while calculating the route!");
			router.Close();
			return;
		}

		System.out.println("Route found...");

		System.out.println("Generate route description...");

		RouteDescriptionResult routeDescriptionResult = router.TransformRouteDataToRouteDescription(routingResult.GetRoute());

		if (!routeDescriptionResult.Success()) {
			System.err.println("There was an error during generation of the route description!");
			router.Close();
			return;
		}

		RoutePostprocessor postprocessor = new RoutePostprocessor();
		
		//postprocessor.PostprocessRouteDescription(routeDescriptionResult.GetDescription(), profile, database, processors)
		
		database.Close();
	}
}
