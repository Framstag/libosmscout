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

import osm.scout.GeoPos;

public class GeoBox {
	
	private double mLonMin;
	private double mLonMax;
	private double mLatMin;
	private double mLatMax;
	
	public GeoBox(double lonMin, double lonMax, double latMin, double latMax) {
		
		mLonMin=Math.min(lonMin, lonMax);
		mLonMax=Math.max(lonMin, lonMax);
		mLatMin=Math.min(latMin, latMax);
		mLatMax=Math.max(latMin, latMax);
	}
	
	public GeoBox(GeoPos posTopLeft, GeoPos posBotRight) {
		
		mLonMin=Math.min(posTopLeft.getLon(), posBotRight.getLon());
		mLonMax=Math.max(posTopLeft.getLon(), posBotRight.getLon());
		mLatMin=Math.min(posTopLeft.getLat(), posBotRight.getLat());
		mLatMax=Math.max(posTopLeft.getLat(), posBotRight.getLat());
	}
	
	public GeoBox(GeoBox box) {
		
		mLonMin=box.getLonMin();
		mLonMax=box.getLonMax();
		mLatMin=box.getLatMin();
		mLatMax=box.getLatMax();
	}
	
	public double getLonMin() {
		return mLonMin;
	}
	
	public double getLonMax() {
		return mLonMax;
	}
	
	public double getLatMin() {
		return mLatMin;
	}
	
	public double getLatMax() {
		return mLatMax;
	}
	
	public double getWidth() {
		return Math.abs(mLonMax-mLonMin);
	}
	
	public double getHeight() {
		return Math.abs(mLatMax-mLatMin);
	}
	
	public GeoPos getCenter() {
		return new GeoPos((mLonMax+mLonMin)/2.0, (mLatMax+mLatMin)/2.0);
	}
	
	public boolean geoIsIn(GeoPos pos) {
		return geoIsIn(pos.getLon(), pos.getLat());
	}
	
	public boolean geoIsIn(double lon, double lat) {
		
		if ((lon<mLonMin) || (lon>mLonMax) || (lat<mLatMin) || (lat>mLatMax)) {
			return false;
		}
		else
			return true;
	}
	
	public void scale(double scaleFactor) {
		
		GeoPos center=getCenter();
		double width=getWidth();
		double height=getHeight();
		
		width*=scaleFactor;
		height*=scaleFactor;
		
		mLonMin=center.getLon()-width/2.0;
		mLonMax=center.getLon()+width/2.0;
		
		mLatMin=center.getLat()-height/2.0;
		mLatMax=center.getLat()-height/2.0;
	}
}
