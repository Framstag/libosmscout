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

import java.util.Vector;

public class MapParameter {

	private int mJniMapParameterIndex;

	public MapParameter() {

		mJniMapParameterIndex=jniConstructor();
	}

	public MapParameter(int jniMapParameterIndex) {

		mJniMapParameterIndex=jniMapParameterIndex;
	}

	protected void finalize() throws Throwable {

		try {
			jniDestructor(mJniMapParameterIndex);
		} finally {
			super.finalize();
		}
	}

	public int getJniObjectIndex() {
		return mJniMapParameterIndex;
	}

	public void setIconPaths(Vector<String> iconPaths) {

		String[] pathsArray = new String[iconPaths.size()];

		for(int i=0; i<iconPaths.size(); i++) {
			
			pathsArray[i]=iconPaths.elementAt(i);
		}

		jniSetIconPaths(mJniMapParameterIndex, pathsArray);
	}

	public void setPatternPaths(Vector<String> patternPaths) {

		String[] pathsArray = new String[patternPaths.size()];

		for(int i=0; i<patternPaths.size(); i++) {
			
			pathsArray[i]=patternPaths.elementAt(i);
		}

		jniSetPatternPaths(mJniMapParameterIndex, pathsArray);
	}
	
	public void setRenderSeaLand(boolean render) {
		
		jniSetRenderSeaLand(mJniMapParameterIndex, render);
	}

	public boolean getRenderSeaLand() {
		
		return jniGetRenderSeaLand(mJniMapParameterIndex);
	}

	// Native methods
	private native int jniConstructor();

	private native void jniDestructor(int jniMapParameterIndex);

	private native void jniSetIconPaths(int jniMapParameterIndex,
											String[] pathsArray);

	private native void jniSetPatternPaths(int jniMapParameterIndex,
											String[] pathsArray);
	
	private native void jniSetRenderSeaLand(int jniMapParameterIndex,
											boolean render);
	
	private native boolean jniGetRenderSeaLand(int jniMapParameterIndex);
}
