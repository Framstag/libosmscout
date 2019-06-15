package net.sf.libosmscout;

import net.sf.libosmscout.osmscout.*;

public class LocationLookup {

  static {
    System.loadLibrary("osmscout_binding_java");
  }

  private static String getMatchQuality(LocationSearchResult.MatchQuality quality) {
		if (quality==LocationSearchResult.MatchQuality.match) {
			return "=";
		}
		else {
			return "~";
		}
	}

	private static String getAdminRegion(LocationSearchResult.Entry entry) {
  	AdminRegion adminRegion = entry.getAdminRegion();
		StringBuilder builder = new StringBuilder();

		builder.append(getMatchQuality(entry.getAdminRegionMatchQuality()));
		builder.append(" ");
		builder.append("Region (");
		if (!adminRegion.getAliasName().isEmpty()) {
			builder.append(adminRegion.getAliasName());
		}
		else {
			builder.append(adminRegion.getName());
		}
		builder.append(")");

		return builder.toString();
	}

	private static String getPostalArea(LocationSearchResult.Entry entry) {
		PostalArea postalArea = entry.getPostalArea();
		StringBuilder builder = new StringBuilder();

		builder.append(getMatchQuality(entry.getPostalAreaMatchQuality()));
		builder.append(" ");
		builder.append("Postal Area (");
		builder.append(postalArea.getName());
		builder.append(")");

		return builder.toString();
	}

	private static String getLocation(LocationSearchResult.Entry entry) {
		Location location = entry.getLocation();
		StringBuilder builder = new StringBuilder();

		builder.append(getMatchQuality(entry.getLocationMatchQuality()));
		builder.append(" ");
		builder.append("Location (");
		builder.append(location.getName());
		builder.append(")");

		return builder.toString();
	}

	private static String getAddress(LocationSearchResult.Entry entry) {
  	Address address = entry.getAddress();
		StringBuilder builder = new StringBuilder();

		builder.append(getMatchQuality(entry.getAddressMatchQuality()));
		builder.append(" ");
		builder.append("Address (");
		builder.append(address.getName());
		builder.append(")");

		return builder.toString();
	}

	private static String getObject(ObjectFileRef object) {
		StringBuilder builder = new StringBuilder();

		builder.append(object.GetTypeName());
		builder.append(" ");
		builder.append(object.GetFileOffset());

		return builder.toString();
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
		LocationStringSearchParameter searchParameter = new LocationStringSearchParameter(searchPattern);
    LocationSearchResult searchResult = new LocationSearchResult();

    System.out.println("Searching for '"+searchPattern+"'...");

    if (!locationService.SearchForLocationByString(searchParameter,searchResult)) {
			System.err.println("Error while searching for location");
			database.Close();
			return;
		}

    System.out.println("Done.");

    for (LocationSearchResult.Entry entry : searchResult.getResults()) {
			if (entry.getAdminRegion() != null & entry.getLocation()!=null & entry.getAddress()!=null) {
				System.out.print(getLocation(entry));
				System.out.print(" ");
				System.out.print(getAddress(entry));
				System.out.print(" ");
				System.out.print(getPostalArea(entry));
				System.out.print(" ");
				System.out.print(getAdminRegion(entry));
				System.out.println();

				// TODO: Dump AdminRegion hierarchy

				System.out.print("  - ");
				System.out.print(getObject(entry.getAddress().getObject()));
				System.out.println();
			}
			else if (entry.getAdminRegion() != null & entry.getLocation()!=null) {
				System.out.print(getLocation(entry));
				System.out.print(" ");
				System.out.print(getPostalArea(entry));
				System.out.print(" ");
				System.out.print(getAdminRegion(entry));
				System.out.println();

				// TODO: Dump AdminRegion hierarchy

				for (ObjectFileRef object : entry.getLocation().getObjects()) {
					System.out.print("  - ");
					System.out.print(getObject(object));
					System.out.println();
				}
			}
			else if (entry.getAdminRegion() != null & entry.getPoi()!=null) {
				System.out.print(getAdminRegion(entry));
				System.out.println();

				// TODO: Dump AdminRegion hierarchy
				// TODO Dump object
			}
			else if (entry.getAdminRegion()!=null) {
				System.out.print(getAdminRegion(entry));
				System.out.println();

				// TODO: Dump AdminRegion hierarchy

				System.out.print("  - ");
				if (entry.getAdminRegion().getAliasObject().Valid()) {
					System.out.print(getObject(entry.getAdminRegion().getAliasObject()));
				}
				else {
					System.out.print(getObject(entry.getAdminRegion().getObject()));
				}
				System.out.println();
			}
		}

    database.Close();
	}

}
