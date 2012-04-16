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
	
	public Database() {		
		jniConstructor();
	}
	
	protected void finalize() throws Throwable {		
		try {			
			jniDestructor();
		}
		finally {			
			super.finalize();
		}
	}
	
	public boolean open(String path) {
		return jniOpen(path);
	}
	
	public boolean isOpen() {
		return jniIsOpen();
	}
	
	public GeoBox getBoundingBox() {
		return jniGetBoundingBox();
	}
	
	public MapData getObjects(Projection projection) {
		return jniGetObjects();
	}
	
	public Node getNode(long id) {
		return jniGetNode(id);
	}
	
	public boolean getMatchingAdminRegions(String name, Vector<AdminRegion> regions,
			int limit, Bool limitReached, boolean startWith) {

		regions.clear();
		
		AdminRegion[] regionArray=jniGetMatchingAdminRegions(name, limit, limitReached, startWith);
		
		if (regionArray==null)
			return false;
		
		regions.addAll(Arrays.asList(regionArray));
		
		return true;
	}
	
	// Native methods
	
	private native void jniConstructor();
	private native void jniDestructor();
	
	private native boolean jniOpen(String path);
	private native boolean jniIsOpen();
	private native GeoBox jniGetBoundingBox();
	
	private native AdminRegion[] jniGetMatchingAdminRegions(String name, int limit,
			Bool limitReached, boolean startWith);
	
	private native Node jniGetNode(long id);
	
	private native MapData jniGetObjects();
}
