/*
  MercatorProjection - a test program for libosmscout
  Copyright (C) 2023 Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "osmscout/projection/MercatorProjection.h"

#include <TestMain.h>

osmscout::GeoCoord defaultCenter(51.51241,
                                 7.46525);
double             defaultAngle =0.0;
double             defaultDpi   =96.0;
double             defaultWidth =800;
double             defaultHeight=480;

TEST_CASE("GetCenter() should return the center")
{
  osmscout::MercatorProjection projection;

  projection.Set(defaultCenter,
                 defaultAngle,
                 osmscout::Magnification(osmscout::Magnification::magClose),
                 defaultDpi,
                 defaultWidth,
                 defaultHeight);

  REQUIRE(projection.GetCenter() == defaultCenter);
}

TEST_CASE("GetWidth() should return width")
{
  osmscout::MercatorProjection projection;
  osmscout::GeoCoord center(51.51241,
                            7.46525);
  projection.Set(defaultCenter,
                 defaultAngle,
                 osmscout::Magnification(osmscout::Magnification::magClose),
                 defaultDpi,
                 defaultWidth,
                 defaultHeight);

  REQUIRE(projection.GetWidth()==defaultWidth);
}

TEST_CASE("GetHeight() should return height")
{
  osmscout::MercatorProjection projection;

  projection.Set(defaultCenter,
                 defaultAngle,
                 osmscout::Magnification(osmscout::Magnification::magClose),
                 defaultDpi,
                 defaultWidth,
                 defaultHeight);

  REQUIRE(projection.GetHeight()==defaultHeight);
}

TEST_CASE("CanBatch() should return false")
{
  osmscout::MercatorProjection projection;

  REQUIRE_FALSE(projection.CanBatch());
}

TEST_CASE("Uninitialised projection is not valid")
{
  osmscout::MercatorProjection projection;

  REQUIRE_FALSE(projection.IsValid());
}

TEST_CASE("Correctly initialised projection is valid")
{
  osmscout::MercatorProjection projection;

  projection.Set(defaultCenter,
                 defaultAngle,
                 osmscout::Magnification(osmscout::Magnification::magClose),
                 defaultDpi,
                 defaultWidth,
                 defaultHeight);

  REQUIRE(projection.IsValid());
}

TEST_CASE("Is validFor() for europe")
{
  osmscout::MercatorProjection projection;

  projection.Set(defaultCenter,
                 defaultAngle,
                 osmscout::Magnification(osmscout::Magnification::magClose),
                 defaultDpi,
                 defaultWidth,
                 defaultHeight);

  REQUIRE(projection.IsValidFor(defaultCenter));
}

TEST_CASE("Is not validFor() north pole")
{
  osmscout::MercatorProjection projection;

  projection.Set(defaultCenter,
                 defaultAngle,
                 osmscout::Magnification(osmscout::Magnification::magClose),
                 defaultDpi,
                 defaultWidth,
                 defaultHeight);

  REQUIRE_FALSE(projection.IsValidFor(osmscout::GeoCoord(90.0,0.0)));
}

TEST_CASE("Is not validFor() south pole")
{
  osmscout::MercatorProjection projection;

  projection.Set(defaultCenter,
                 defaultAngle,
                 osmscout::Magnification(osmscout::Magnification::magClose),
                 defaultDpi,
                 defaultWidth,
                 defaultHeight);

  REQUIRE_FALSE(projection.IsValidFor(osmscout::GeoCoord(-90.0,0.0)));
}

TEST_CASE("GetPixelSize()")
{
  osmscout::MercatorProjection projection;

  projection.Set(defaultCenter,
    defaultAngle,
    osmscout::Magnification(osmscout::Magnification::magClose),
    defaultDpi,
    defaultWidth,
    defaultHeight);

  REQUIRE(projection.GetPixelSize()==Approx(5.9462763732));
}

TEST_CASE("GetMeterInMM()")
{
  osmscout::MercatorProjection projection;

  projection.Set(defaultCenter,
    defaultAngle,
    osmscout::Magnification(osmscout::Magnification::magClose),
    defaultDpi,
    defaultWidth,
    defaultHeight);

  REQUIRE(projection.GetMeterInMM()==Approx(0.0444956334));
}

TEST_CASE("GetMeterInPixel()")
{
  osmscout::MercatorProjection projection;

  projection.Set(defaultCenter,
    defaultAngle,
    osmscout::Magnification(osmscout::Magnification::magClose),
    defaultDpi,
    defaultWidth,
    defaultHeight);

  REQUIRE(projection.GetMeterInPixel()==Approx(0.1681724725));
}

TEST_CASE("ConvertWidthToPixel()")
{
  osmscout::MercatorProjection projection;

  projection.Set(defaultCenter,
    defaultAngle,
    osmscout::Magnification(osmscout::Magnification::magClose),
    defaultDpi,
    defaultWidth,
    defaultHeight);

  REQUIRE(projection.ConvertWidthToPixel(100.0)==Approx(377.9527559055));
}

TEST_CASE("ConvertPixelToWidth()")
{
  osmscout::MercatorProjection projection;

  projection.Set(defaultCenter,
    defaultAngle,
    osmscout::Magnification(osmscout::Magnification::magClose),
    defaultDpi,
    defaultWidth,
    defaultHeight);

  REQUIRE(projection.ConvertPixelToWidth(377.9527559055)==Approx(100.0));
}

TEST_CASE("PixelToGeo()")
{
  osmscout::MercatorProjection projection;

  projection.Set(defaultCenter,
    defaultAngle,
    osmscout::Magnification(osmscout::Magnification::magClose),
    defaultDpi,
    defaultWidth,
    defaultHeight);

  osmscout::GeoCoord result;

  REQUIRE(projection.PixelToGeo(projection.GetWidth()/2.0,
                                projection.GetHeight()/2.0,
                                result));

  REQUIRE(result==osmscout::GeoCoord(51.51241,7.46525));
}

TEST_CASE("GeoToPixel()")
{
  osmscout::MercatorProjection projection;

  projection.Set(defaultCenter,
    defaultAngle,
    osmscout::Magnification(osmscout::Magnification::magClose),
    defaultDpi,
    defaultWidth,
    defaultHeight);

  osmscout::Vertex2D result;

  REQUIRE(projection.GeoToPixel(osmscout::GeoCoord(51.51241,7.46525),
                                result));

  REQUIRE(result==osmscout::Vertex2D(defaultWidth/2.0,
                                     defaultHeight/2.0));
}

TEST_CASE("GetDimensions()")
{
  osmscout::MercatorProjection projection;
  osmscout::GeoBox expectedBox(osmscout::GeoCoord(51.49959,7.43092),
                               osmscout::GeoCoord(51.52523,7.49958));

  projection.Set(defaultCenter,
                 defaultAngle,
                 osmscout::Magnification(osmscout::Magnification::magClose),
                 defaultDpi,
                 defaultWidth,
                 defaultHeight);

  REQUIRE(projection.GetDimensions().GetDisplayText()==expectedBox.GetDisplayText());
}

TEST_CASE("GetDimensions() with rotation")
{
  osmscout::MercatorProjection projection;
  osmscout::GeoBox expectedBox(osmscout::GeoCoord(51.49061,7.42522),
                               osmscout::GeoCoord(51.53420,7.50528));

  projection.Set(defaultCenter,
                 0.524, // 30°
                 osmscout::Magnification(osmscout::Magnification::magClose),
                 defaultDpi,
                 defaultWidth,
                 defaultHeight);

  REQUIRE(projection.GetDimensions().GetDisplayText()==expectedBox.GetDisplayText());
}
