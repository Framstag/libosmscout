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

public abstract class Projection {
	
	protected double   mLon=0.0;             //! Longitude coordinate of the center of the image
	protected double   mLat=0.0;             //! Latitude coordinate of the center of the image
	protected double   mMagnification=1.0;   //! Current magnification
	protected int      mWidth=0;         	 //! Width of image
	protected int      mHeight=0;            //! Height of image
	
	protected GeoBox   mBoundaries=null;     //! Projection boundaries
	
	public double getLon() {
		return mLon;
	}
	
	public double getLat() {
		return mLat;
	}
	
	public double getMagnification() {
		return mMagnification;
	}
	
	public int getWidth() {
		return mWidth;
	}
	
	public int getHeight() {
		return mHeight;
	}
	
	public abstract GeoBox getBoundaries();
	
	public abstract boolean set(double lon, double lat, double magnification,
                     int width, int height);
	
	public boolean setPos(double lon, double lat) {
		return set(lon, lat, mMagnification, mWidth, mHeight);
	}
	
	public boolean setPos(GeoPos pos) {
		return setPos(pos.getLon(), pos.getLat());
	}
	
	public boolean setPos(GeoPos pos, double magnification) {
		return set(pos.getLon(), pos.getLat(), magnification, mWidth, mHeight);
	}
	
	public boolean setMagnification(double magnification) {
		return set(mLon, mLat, magnification, mWidth, mHeight);
	}
	
	public boolean setSize(int width, int height) {
		return set(mLon, mLat, mMagnification, width, height);
	}
	
	public abstract GeoPos pixelToGeo(double x, double y);
	
	public abstract Point geoToPixel(double lon, double lat);
	
	public Point geoToPixel(GeoPos pos) {
		return geoToPixel(pos.getLon(), pos.getLat());
	}
}
