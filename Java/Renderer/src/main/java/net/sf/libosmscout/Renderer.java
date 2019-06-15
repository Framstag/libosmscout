package net.sf.libosmscout;

import net.sf.libosmscout.osmscout.*;

public class Renderer {

	static {
		System.loadLibrary("osmscout_binding_java");
		System.loadLibrary("osmscout_map_binding_java");
	}

	public static void main(String[] args) {
		if (args.length != 1) {
			System.err.println("Renderer <map directory>");
			return;
		}

		String mapDirectory = args[0];

		// TODO: Argument parsing

		DatabaseParameter parameter = new DatabaseParameter();
		Database database = new Database(parameter);

		System.out.println("Opening database '" + mapDirectory + "'...");

		if (!database.Open(mapDirectory)) {
			System.err.println("Cannot open database '" + mapDirectory + "'");
			return;
		}

		database.Close();
	}
}
