package net.sf.libosmscout;

import net.sf.libosmscout.osmscout.Database;
import net.sf.libosmscout.osmscout.DatabaseParameter;
import net.sf.libosmscout.osmscout.LocationSearch;
import net.sf.libosmscout.osmscout.LocationSearchResult;
import net.sf.libosmscout.osmscout.LocationService;

public class LocationLookup {

  static {
    System.loadLibrary("osmscoutjava");
  }
	
	public static void main(String[] args) {
		if (args.length<2) {
			System.err.println("LocationLookup <map directory> address...");
			return;
		}
		
		String mapDirectory = args[0];
		String searchPattern = args[1];
		
		int argIndex=2;
		while (argIndex<args.length) {
			searchPattern = searchPattern + " " + args[argIndex];
			argIndex++;
		}
		
    DatabaseParameter parameter=new DatabaseParameter();
    Database database = new Database(parameter);

    System.out.println("Opening database '"+mapDirectory+"'...");
    
    if (!database.Open(mapDirectory)) {
    	System.err.println("Cannot open database '" + mapDirectory + "'");
    	return;
    }
		
    LocationService locationService = new LocationService(database);
    LocationSearch locationSearch = new LocationSearch();
    LocationSearchResult locationSearchResult = new LocationSearchResult();
    
    if (!locationService.InitializeLocationSearchEntries(searchPattern, locationSearch)) {
    	System.err.println("Error while parsing search pattern");
    	database.Close();
    	return;
    }
    
    System.out.println("Looking up search pattern '" + searchPattern+"'...");
    
    if (!locationService.SearchForLocations(locationSearch, locationSearchResult)) {
    	System.err.println("Error while searching for location");
    	database.Close();
    	return;
    }

    System.out.println("Done.");
    
    //TODO: Currently there is no implementation for std::list<>....
    
    database.Close();
	}

}
