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
	
	public static int lon2tilex(double lon, long zoom) {
		
		return (int) Math.floor((lon+180.0)/360.0*Math.pow(2.0, zoom));
	}
	
	public static int lat2tiley(double lat, long zoom) {
		
		return (int) Math.floor((1.0-Math.log(Math.tan(lat*Math.PI/180.0)+1.0/Math.cos(lat*Math.PI/180.0))/Math.PI)/2.0*Math.pow(2.0, zoom));
	}

	public static double tilex2long(long x, long zoom) {
		
		return x/Math.pow(2.0, zoom)*360.0-180.0;
	}
	
	public static double tiley2lat(long y, long zoom) {
		
		double n =Math.PI-2.0*Math.PI*y/Math.pow(2.0, zoom);
		
		return 180.0/Math.PI*Math.atan(0.5*(Math.exp(n)-Math.exp(-n)));
	}
}
