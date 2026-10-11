/*
  This source is part of the libosmscout-map library
  Copyright (C) 2026  Tim Teulings

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

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#if defined(__WIN32__) || defined(WIN32) || (defined(__APPLE__) && __APPLE__)
  #include <cairo.h>
#else
  #include <cairo/cairo.h>
#endif

#include <osmscout/Area.h>
#include <osmscout/GeoCoord.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>
#include <osmscout/log/Logger.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscout/util/Magnification.h>

#include <osmscoutmap/MapData.h>
#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/Styles.h>

#include <osmscoutmapcairo/MapPainterCairo.h>

/*
 * Tests for the report a pattern-capable backend produces when it cannot serve a pattern fill:
 * a render without a pattern image source has to name the misconfiguration and fall back to the
 * solid fill colour, and a render whose configured directory holds the image has to stay quiet.
 */
namespace {

  constexpr const char  *PatternName="natural_scrub";

  const osmscout::Color FillColor(0.0,1.0,0.0);

// ---------------------------------------------------------------------------
// Capturing logger: records every log line a render produces
// ---------------------------------------------------------------------------
  class CapturingDestination : public osmscout::Logger::Destination
  {
  public:
    std::vector<std::string> lines;
    std::string              line;

    void Print(const std::string& value) override
    {
      line+=value;
    }

    void Print(const std::string_view& value) override
    {
      line+=std::string(value);
    }

    void Print(const char* value) override
    {
      line+=value;
    }

    void Print(bool value) override
    {
      line+=value ? "true" : "false";
    }

    void Print(short value) override
    {
      line+=std::to_string(value);
    }

    void Print(unsigned short value) override
    {
      line+=std::to_string(value);
    }

    void Print(int value) override
    {
      line+=std::to_string(value);
    }

    void Print(unsigned int value) override
    {
      line+=std::to_string(value);
    }

    void Print(long value) override
    {
      line+=std::to_string(value);
    }

    void Print(unsigned long value) override
    {
      line+=std::to_string(value);
    }

    void Print(long long value) override
    {
      line+=std::to_string(value);
    }

    void Print(unsigned long long value) override
    {
      line+=std::to_string(value);
    }

    void PrintLn() override
    {
      lines.push_back(line);
      line.clear();
    }
  };

  class CapturingLogger : public osmscout::Logger
  {
  public:
    CapturingDestination destination;

    Line Log(Level /*level*/) override
    {
      return Line(destination);
    }
  };

  /**
   * Installs a capturing logger for its lifetime and restores the logger that was installed
   * before it.
   */
  class LogCapture
  {
  private:
    osmscout::Log                    previous;
    std::shared_ptr<CapturingLogger> logger;

  public:
    LogCapture()
    : previous(osmscout::log),
      logger(std::make_shared<CapturingLogger>())
    {
      osmscout::log.SetLogger(logger);
    }

    ~LogCapture()
    {
      osmscout::log=previous;
    }

    const std::vector<std::string>& Lines() const
    {
      return logger->destination.lines;
    }

    size_t CountLinesContaining(const std::string& text) const
    {
      return static_cast<size_t>(std::count_if(Lines().begin(),
                                               Lines().end(),
                                               [&text](const std::string& line) {
                                                 return line.find(text)!=std::string::npos;
                                               }));
    }
  };

// ---------------------------------------------------------------------------
// Render target: an in-memory surface the test can sample
// ---------------------------------------------------------------------------
  struct TestSurface
  {
    static constexpr int WIDTH=800;
    static constexpr int HEIGHT=480;

    cairo_surface_t      *surface;
    cairo_t              *context;

    TestSurface()
    {
      surface=cairo_image_surface_create(CAIRO_FORMAT_RGB24,WIDTH,HEIGHT);
      REQUIRE(surface!=nullptr);

      context=cairo_create(surface);
      REQUIRE(context!=nullptr);

      cairo_set_source_rgb(context,1.0,1.0,1.0);
      cairo_paint(context);
    }

    ~TestSurface()
    {
      cairo_destroy(context);
      cairo_surface_destroy(surface);
    }

    unsigned int GetPixel(unsigned int x,unsigned int y) const
    {
      cairo_surface_flush(surface);

      const unsigned char *data=cairo_image_surface_get_data(surface);
      const int           stride=cairo_image_surface_get_stride(surface);
      const unsigned char *pixel=data+y*static_cast<size_t>(stride)+x*4;

      return (static_cast<unsigned int>(pixel[2])<<16) |
             (static_cast<unsigned int>(pixel[1])<<8) |
             static_cast<unsigned int>(pixel[0]);
    }

    /**
     * True if any pixel carries the given fill colour, i.e. the area was drawn with the solid
     * fallback instead of a pattern.
     */
    bool ContainsColor(const osmscout::Color& color) const
    {
      const unsigned int wanted=(static_cast<unsigned int>(color.GetR()*255.0)<<16) |
                                 (static_cast<unsigned int>(color.GetG()*255.0)<<8) |
                                 static_cast<unsigned int>(color.GetB()*255.0);

      for (int y=0; y<HEIGHT; y++) {
        for (int x=0; x<WIDTH; x++) {
          if (GetPixel(static_cast<unsigned int>(x),static_cast<unsigned int>(y))==wanted) {
            return true;
          }
        }
      }

      return false;
    }
  };

// ---------------------------------------------------------------------------
// Synthetic type, style and area
// ---------------------------------------------------------------------------
  struct TestTypes
  {
    osmscout::TypeConfigRef typeConfig;
    osmscout::TypeInfoRef   areaType;
  };

  TestTypes MakeTypes()
  {
    TestTypes types;

    types.typeConfig=std::make_shared<osmscout::TypeConfig>();

    types.areaType=std::make_shared<osmscout::TypeInfo>("test_pattern_area");
    types.areaType->CanBeArea(true);
    types.typeConfig->RegisterType(types.areaType);

    return types;
  }

  osmscout::StyleConfigRef MakeStyles(const TestTypes& types)
  {
    auto                  styleConfig=std::make_shared<osmscout::StyleConfig>(types.typeConfig);

    osmscout::TypeInfoSet areaTypes(*types.typeConfig);

    areaTypes.Set(types.areaType);

    osmscout::StyleFilter areaFilter;

    areaFilter.SetTypes(areaTypes);

    osmscout::FillPartialStyle fillStyle;

    fillStyle.SetColorValue(osmscout::FillStyle::attrFillColor,FillColor);
    fillStyle.SetStringValue(osmscout::FillStyle::attrPattern,PatternName);
    styleConfig->AddAreaFillStyle(areaFilter,fillStyle);

    styleConfig->Postprocess();

    return styleConfig;
  }

  osmscout::AreaRef MakeArea(const TestTypes& types,
                             double latitudeOffset)
  {
    auto                 area=std::make_shared<osmscout::Area>();

    osmscout::Area::Ring ring;

    ring.MarkAsOuterRing();
    ring.SetType(types.areaType);
    ring.nodes.push_back(osmscout::Point(1,osmscout::GeoCoord(50.1070+latitudeOffset,14.4585)));
    ring.nodes.push_back(osmscout::Point(2,osmscout::GeoCoord(50.1070+latitudeOffset,14.4595)));
    ring.nodes.push_back(osmscout::Point(3,osmscout::GeoCoord(50.1075+latitudeOffset,14.4595)));
    ring.nodes.push_back(osmscout::Point(4,osmscout::GeoCoord(50.1075+latitudeOffset,14.4585)));
    ring.center=osmscout::GeoCoord(50.10725+latitudeOffset,14.4590);

    area->rings.push_back(ring);

    return area;
  }

  osmscout::MercatorProjection MakeProjection()
  {
    osmscout::MercatorProjection projection;

    REQUIRE(projection.Set(osmscout::GeoCoord(50.107252570499767,14.459053009732296),
                           0.0,
                           osmscout::Magnification(osmscout::Magnification::magClose),
                           96.0,
                           TestSurface::WIDTH,
                           TestSurface::HEIGHT));

    return projection;
  }

  bool Render(osmscout::MapPainterCairo& painter,
              const osmscout::MercatorProjection& projection,
              const osmscout::MapParameter& parameter,
              const osmscout::MapData& data,
              cairo_t*context)
  {
    std::vector<osmscout::MapData> dataList{data};

    return painter.DrawMap(projection,
                           parameter,
                           dataList,
                           context);
  }

  /**
   * Directory holding the images the stylesheets use, resolved relative to the test data
   * directory like the other style tests do it.
   */
  std::filesystem::path GetShippedImageDir()
  {
    const char *topDir=std::getenv("TESTS_TOP_DIR");

    REQUIRE(topDir!=nullptr);

    return std::filesystem::path(topDir) / ".." / "libosmscout" / "data" / "icons" / "14x14" / "standard";
  }
}

TEST_CASE("Cairo reports a pattern fill without a pattern image source","[MapPainterCairoPattern]")
{
  TestTypes         types=MakeTypes();

  osmscout::MapData data;

  data.styleConfig=MakeStyles(types);
  data.areas.push_back(MakeArea(types,0.0));

  osmscout::MapParameter    parameter;

  TestSurface               surface;
  LogCapture                capture;

  osmscout::MapPainterCairo painter;

  REQUIRE(Render(painter,MakeProjection(),parameter,data,surface.context));

  // the misconfiguration is reported once, naming the pattern
  REQUIRE(capture.CountLinesContaining("No pattern image source is configured")==1);
  REQUIRE(capture.CountLinesContaining(PatternName)==1);

  // and the area is drawn with the solid fallback colour
  REQUIRE(surface.ContainsColor(FillColor));
}

TEST_CASE("Cairo reports a pattern fill once for several areas","[MapPainterCairoPattern]")
{
  TestTypes         types=MakeTypes();

  osmscout::MapData data;

  data.styleConfig=MakeStyles(types);
  data.areas.push_back(MakeArea(types,0.0));
  data.areas.push_back(MakeArea(types,-0.0020));

  osmscout::MapParameter    parameter;

  TestSurface               surface;
  LogCapture                capture;

  osmscout::MapPainterCairo painter;

  REQUIRE(Render(painter,MakeProjection(),parameter,data,surface.context));

  REQUIRE(data.areas.size()==2);
  REQUIRE(capture.CountLinesContaining("No pattern image source is configured")==1);
}

TEST_CASE("Cairo stays quiet when it can serve the pattern","[MapPainterCairoPattern]")
{
  TestTypes         types=MakeTypes();

  osmscout::MapData data;

  data.styleConfig=MakeStyles(types);
  data.areas.push_back(MakeArea(types,0.0));

  std::filesystem::path patternDir=GetShippedImageDir();

  REQUIRE(std::filesystem::exists(patternDir / (std::string(PatternName) + ".png")));

  osmscout::MapParameter parameter;

  parameter.SetPatternPaths({patternDir.string()});

  TestSurface surface;
  LogCapture  capture;

  // Cairo reports a served pattern on the debug level, which is switched off by default
  osmscout::log.Debug(true);

  osmscout::MapPainterCairo painter;

  auto                      projection=MakeProjection();

  REQUIRE(Render(painter,projection,parameter,data,surface.context));

  REQUIRE(capture.CountLinesContaining("Loaded pattern image")==1);
  REQUIRE(capture.CountLinesContaining("No pattern image source is configured")==0);
  REQUIRE(capture.CountLinesContaining("not found")==0);
  REQUIRE(capture.CountLinesContaining("ERROR while loading pattern image")==0);

  // the pattern was served, so the style is not marked as an unresolvable pattern
  auto fillStyle=data.styleConfig->GetAreaFillStyle(types.areaType,
                                                    data.areas[0]->GetFeatureValueBuffer(),
                                                    projection);

  REQUIRE(fillStyle!=nullptr);
  REQUIRE(fillStyle->GetPatternId()!=0);
}
