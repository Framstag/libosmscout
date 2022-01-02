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

public class GeoPos {

  private double mLon;
  private double mLat;

  private static final double EARTH_RADIUS=6371000.0;

  public GeoPos () {
    mLon=0.0;
    mLat=0.0;
  }

  public GeoPos(GeoPos pos) {
    mLon=pos.getLon();
    mLat=pos.getLat();
  }

  public GeoPos(double lon, double lat) {
    mLon=lon;
    mLat=lat;
  }

  public double getLon() {
    return mLon;
  }

  public double getLat() {
    return mLat;
  }

  public void setLon(double dLon) {
    mLon=dLon;
  }

  public void setLat(double dLat) {
    mLat=dLat;
  }

  public double distanceTo(GeoPos pos) {
    double dLon1=Math.toRadians(mLon);
    double dLat1=Math.toRadians(mLat);

    double dLon2=Math.toRadians(pos.getLon());
    double dLat2=Math.ToRadians(pos.getLat());

    double dDistance=Math.acos(Math.sin(dLat1)*Math.sin(dLat2)+
               Math.cos(dLat1)*Math.cos(dLat2)*Math.cos(dLon2-dLon1))*EARTH_RADIUS;

    return dDistance;
  }
}

