/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

package osm.scout;

import java.util.Arrays;
import java.util.Vector;

public class Database {
	
	private int mJniDatabaseIndex;
	
	public Database() {		
		mJniDatabaseIndex=jniConstructor();
	}
	
	protected void finalize() throws Throwable {
		
		try {			
			jniDestructor(mJniDatabaseIndex);
		}
		finally {			
			super.finalize();
		}
	}
	
	public boolean open(String path) {
		return jniOpen(mJniDatabaseIndex, path);
	}
	
	public boolean isOpen() {
		return jniIsOpen(mJniDatabaseIndex);
	}
	
	public GeoBox getBoundingBox() {
		return jniGetBoundingBox(mJniDatabaseIndex);
	}
	
	public boolean getMatchingAdminRegions(String name, Vector<AdminRegion> regions,
			int limit, Bool limitReached, boolean startWith) {

		regions.clear();
		
		AdminRegion[] regionArray=jniGetMatchingAdminRegions(mJniDatabaseIndex,
				name, limit, limitReached, startWith);
		
		if (regionArray==null)
			return false;
		
		regions.addAll(Arrays.asList(regionArray));
		
		return true;
	}
	
	public Node getNode(long id) {
		return jniGetNode(mJniDatabaseIndex, id);
	}
	
	public MapData getObjects(ObjectTypeSets objectTypeSets, Projection projection) {
		
		GeoBox bounds=projection.getBoundaries();
		
		return jniGetObjects(mJniDatabaseIndex, objectTypeSets.getJniObjectIndex(),
	            bounds.getLonMin(), bounds.getLatMin(),
	            bounds.getLonMax(), bounds.getLatMax(),
	            projection.getMagnification());
	}
	
	public TypeConfig getTypeConfig() {
		return jniGetTypeConfig(mJniDatabaseIndex);
	}
	
	// Native methods
	
	private native int jniConstructor();
	private native void jniDestructor(int databaseIndex);
	
	private native boolean jniOpen(int databaseIndex, String path);
	private native boolean jniIsOpen(int databaseIndex);
	private native GeoBox jniGetBoundingBox(int databaseIndex);
	
	private native AdminRegion[] jniGetMatchingAdminRegions(int databaseIndex,
			String name, int limit, Bool limitReached, boolean startWith);
	
	private native Node jniGetNode(int databaseIndex, long id);
	
	private native MapData jniGetObjects(
			int databaseIndex, int objectTypeSetsIndex,
            double lonMin, double latMin, double lonMax, double latMax,
            double magnification);
	
	private native TypeConfig jniGetTypeConfig(int databaseIndex);
}
