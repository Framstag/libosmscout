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

import android.graphics.Point;

public class MercatorProjection extends Projection {
	
	private int mJniProjectionIndex;
	
	public MercatorProjection() {
		mJniProjectionIndex=jniConstructor();
	}
	
	protected void finalize() throws Throwable {
		
		try {			
			jniDestructor(mJniProjectionIndex);
		}
		finally {			
			super.finalize();
		}
	}
	
	public int getJniObjectIndex() {
		return mJniProjectionIndex;
	}
	
	public boolean set(double lon, double lat, double magnification,
            int width, int height) {
		
		mLon=lon;
		mLat=lat;
		mMagnification=magnification;
		mWidth=width;
		mHeight=height;
		
		mBoundaries=null;

		return jniSet(mJniProjectionIndex, mLon, mLat, mMagnification,
				             mWidth, mHeight);
	}
	
	public GeoPos pixelToGeo(double x, double y) {
		return jniPixelToGeo(mJniProjectionIndex, x, y);
	}
	
	public Point geoToPixel(double lon, double lat) {
		return jniGeoToPixel(mJniProjectionIndex, lon, lat);
	}
	
	public GeoBox getBoundaries() {
		
		if (mBoundaries==null) {			
			mBoundaries=jniGetBoundaries(mJniProjectionIndex);			
		}
		
		return new GeoBox(mBoundaries);
	}	
	
	// Native methods
	
	private native int jniConstructor();
	private native void jniDestructor(int jniProjectionIndex);
	
	protected native GeoBox jniGetBoundaries(int jniProjectionIndex);
	
	protected native boolean jniSet(int jniProjectionIndex,
			double lon, double lat, double magnification,
			int width, int height);
	
	protected native GeoPos jniPixelToGeo(int jniProjectionIndex,
			                              double x, double y);
	
	protected native Point jniGeoToPixel(int jniProjectionIndex,
			                             double lon, double lat);
}
