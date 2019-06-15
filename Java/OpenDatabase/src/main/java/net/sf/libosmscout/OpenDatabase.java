package net.sf.libosmscout;

import net.sf.libosmscout.osmscout.Database;
import net.sf.libosmscout.osmscout.DatabaseParameter;
import net.sf.libosmscout.osmscout.GeoBox;

public class OpenDatabase
{
  static {
    System.loadLibrary("osmscout_binding_java");
  }

  public static void main(String[] args)
  {
    if (args.length!=1) {
      System.err.println("OpenDatabase <map directory>");
      return;
    }
  
    String mapDirectory=args[0];
    
    DatabaseParameter parameter=new DatabaseParameter();
    Database database = new Database(parameter);

    System.out.println("Opening database '"+mapDirectory+"'...");
    
    boolean result=database.Open(mapDirectory);
    
    System.out.println("Database opened: " + result);
    
    if (result) {
      GeoBox boundingBox = new GeoBox();
      
      if (database.GetBoundingBox(boundingBox)) {
        System.out.println("BoundingBox: " + boundingBox.GetDisplayText());
      }
    
      System.out.println("Closing database...");
      database.Close();
      System.out.println("Database closed.");
    }
  }
}
