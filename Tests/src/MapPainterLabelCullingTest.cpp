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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <osmscout/GeoCoord.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscout/util/Magnification.h>
#include <osmscout/util/Transformation.h>
#include <osmscoutmap/LabelLayouter.h>
#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/Styles.h>

/*
 * Tests for the contract of the volume of the label stage (spec map-painter-label-culling): a
 * label whose rectangle cannot intersect the view is neither measured nor stored nor laid out,
 * the decision keeps every label the drawing path of the label stage would draw, it holds for
 * text, icon and symbol elements and for element lists, and the placement of the labels that take
 * part in the frame stays what the unculled pipeline produces.
 *
 * The tests drive the shared label layouter with a test text layouter, so they need no backend,
 * no font and no database. The test text layouter counts its layout calls, which is the
 * measurement work of the label stage.
 */

namespace {

  struct TestGlyph
  {};

  struct TestNativeLabel
  {};

  using TestLabel = osmscout::Label<TestGlyph,TestNativeLabel>;

  /**
   * Text layouter that measures a label by its text length and counts its layout calls. Its
   * advance per character is half of the font size in pixels, i.e. smaller than the conservative
   * extent bound of the label helper, so a test can state the difference between the measured
   * rectangle and the bound.
   */
  class TestTextLayouter
  {
  private:
    size_t layoutCalls=0;

  public:
    size_t LayoutCalls() const
    {
      return layoutCalls;
    }

    /**
     * Advance of one character, in multiples of the font size in pixels
     */
    static constexpr double CharacterAdvance=0.5;

    std::shared_ptr<TestLabel> Layout(const osmscout::Projection& projection,
                                      const osmscout::MapParameter& parameter,
                                      const std::string& text,
                                      double fontSize,
                                      double /*objectWidth*/,
                                      bool /*enableWrapping*/,
                                      bool /*contourLabel*/)
    {
      layoutCalls++;

      double fontSizePixel=fontSize*projection.ConvertWidthToPixel(parameter.GetFontSize());

      auto   label=std::make_shared<TestLabel>();

      label->width=(double)text.size()*fontSizePixel*CharacterAdvance;
      label->height=fontSizePixel;
      label->fontSize=fontSize;
      label->text=text;

      return label;
    }

    osmscout::ScreenVectorRectangle GlyphBoundingBox(const TestGlyph& /*glyph*/) const
    {
      return {0.0,0.0,0.0,0.0};
    }
  };

  using TestLayouter = osmscout::LabelLayouter<TestGlyph,TestNativeLabel,TestTextLayouter>;

  constexpr double CanvasWidth=400.0;
  constexpr double CanvasHeight=400.0;

  osmscout::MercatorProjection MakeProjection()
  {
    osmscout::MercatorProjection projection;

    REQUIRE(projection.Set(osmscout::GeoCoord(50.001,8.001),
                           osmscout::Magnification(osmscout::Magnification::magClose),
                           300,
                           400,
                           400));

    return projection;
  }

  /**
   * Map parameter with the label paddings and the label layouter overlap switched off, so that a
   * test can state the decision of the label stage as "the rectangle of the label against the
   * view": the margin the label layout leaves around an element is covered by its own test case.
   */
  osmscout::MapParameter MakeParameter()
  {
    osmscout::MapParameter parameter;

    parameter.SetLabelLayouterOverlap(0.0);
    parameter.SetIconPadding(0.0);
    parameter.SetLabelPadding(0.0);
    parameter.SetPlateLabelPadding(0.0);
    parameter.SetContourLabelPadding(0.0);
    parameter.SetOverlayLabelPadding(0.0);

    return parameter;
  }

  /**
   * Map parameter with the defaults of the library, including the margin the label layout leaves
   * around an element
   */
  osmscout::MapParameter MakeDefaultParameter()
  {
    return osmscout::MapParameter();
  }

  osmscout::LabelData MakeTextLabel(const std::string& text,
                                    double fontSize=1.3)
  {
    osmscout::LabelData data;

    data.type=osmscout::LabelData::Type::Text;
    data.text=text;
    data.fontSize=fontSize;
    data.alpha=1.0;
    data.priority=0;

    return data;
  }

  osmscout::LabelData MakeIconLabel(double width,
                                    double height)
  {
    osmscout::LabelData data;

    data.type=osmscout::LabelData::Type::Icon;
    data.iconWidth=width;
    data.iconHeight=height;
    data.alpha=1.0;
    data.priority=0;

    return data;
  }

  osmscout::ScreenVectorRectangle VisibleViewport()
  {
    return {0.0,0.0,CanvasWidth,CanvasHeight};
  }

  /**
   * Pixels a label of the given data reaches beyond its anchor position, derived the same way the
   * label layouter derives it: the sum of the extents of the elements of the label
   */
  double LabelReach(const osmscout::Projection& projection,
                    const osmscout::MapParameter& parameter,
                    const std::vector<osmscout::LabelData>& data)
  {
    double fontSizePixel=projection.ConvertWidthToPixel(parameter.GetFontSize());
    double reach=0.0;

    for (const auto& d : data) {
      if (d.type==osmscout::LabelData::Type::Text) {
        reach+=2.0*osmscout::GetLabelExtentBound(d.text.size(),
                                                 osmscout::CountLabelWords(d.text),
                                                 d.fontSize*fontSizePixel);
      }
      else {
        reach+=std::max(d.iconWidth,d.iconHeight);
      }
    }

    return reach;
  }

  osmscout::ScreenVectorRectangle ReachBox(const osmscout::Projection& projection,
                                           const osmscout::MapParameter& parameter,
                                           const std::vector<osmscout::LabelData>& data,
                                           const osmscout::Vertex2D& anchor)
  {
    double reach=LabelReach(projection,parameter,data);

    return {anchor.GetX()-reach,
            anchor.GetY()-reach,
            2.0*reach,
            2.0*reach};
  }

  /**
   * The rectangle the label stage places the text element of a label in, derived from the test
   * text layouter (the element is centered on the anchor)
   */
  osmscout::ScreenVectorRectangle TextElementRectangle(const osmscout::Projection& projection,
                                                       const osmscout::MapParameter& parameter,
                                                       const osmscout::LabelData& data,
                                                       const osmscout::Vertex2D& anchor)
  {
    double fontSizePixel=data.fontSize*projection.ConvertWidthToPixel(parameter.GetFontSize());
    double width=(double)data.text.size()*fontSizePixel*TestTextLayouter::CharacterAdvance;
    double height=fontSizePixel;

    return {anchor.GetX()-width/2.0,
            anchor.GetY()-height/2.0,
            width,
            height};
  }

  /**
   * Prepare a layouter of the test for a frame: it lays out against the visible viewport of the
   * test without a layout overlap. The layouter reuses its frame state, so it is neither copyable
   * nor movable and the caller has to own it; it is therefore configured in place.
   */
  void ConfigureLayouter(TestLayouter& layouter)
  {
    layouter.SetViewport(VisibleViewport());
    layouter.SetLayoutOverlap(0);
  }

  /**
   * Pixels a label element may lie outside the visible view and still take part in a frame, as the
   * label layouter derives it from the map parameters
   */
  double LayoutMargin(const osmscout::Projection& projection,
                      const osmscout::MapParameter& parameter)
  {
    return osmscout::GetLabelLayoutMarginPixel(projection,parameter);
  }
}

/**
 * A label whose rectangle cannot intersect the view is neither measured nor stored (spec
 * map-painter-label-culling, requirement "A label that cannot intersect the view is neither
 * measured nor stored nor laid out").
 */
TEST_CASE("A label outside the view is not measured","[MapPainterLabelCulling]")
{
  TestTextLayouter       textLayouter;
  osmscout::MapParameter parameter=MakeParameter();
  TestLayouter           layouter(&textLayouter);

  ConfigureLayouter(layouter);

  const auto projection=MakeProjection();
  auto       data=MakeTextLabel("Hauptstrasse");

  // Far outside the bound the label layouter uses
  osmscout::Vertex2D farOutside(20.0*CanvasWidth,20.0*CanvasHeight);

  REQUIRE(!ReachBox(projection,parameter,{data},farOutside).Intersects(VisibleViewport()));

  layouter.RegisterLabel(projection,
                         parameter,
                         false,
                         osmscout::ObjectFileRef(),
                         farOutside,
                         data);

  REQUIRE(textLayouter.LayoutCalls()==0);
  REQUIRE(layouter.Labels().empty());

  // Inside the viewport
  osmscout::Vertex2D inside(CanvasWidth/2.0,CanvasHeight/2.0);

  layouter.RegisterLabel(projection,
                         parameter,
                         false,
                         osmscout::ObjectFileRef(),
                         inside,
                         data);

  REQUIRE(textLayouter.LayoutCalls()==1);
  REQUIRE(layouter.Labels().size()==1);
}

/**
 * The decision is conservative: a label whose rectangle intersects the viewport is measured,
 * stored and placed exactly as the unculled pipeline places it, including a label whose anchor
 * lies outside the viewport (spec map-painter-label-culling, requirement "A label that cannot
 * intersect the view is neither measured nor stored nor laid out").
 */
TEST_CASE("A label that reaches into the view is measured","[MapPainterLabelCulling]")
{
  TestTextLayouter       textLayouter;
  osmscout::MapParameter parameter=MakeParameter();
  TestLayouter           layouter(&textLayouter);

  ConfigureLayouter(layouter);

  const auto projection=MakeProjection();
  auto       data=MakeTextLabel("Hauptstrasse");

  // The anchor is outside the viewport, but the label is wide enough to reach into it
  osmscout::Vertex2D anchor(CanvasWidth+10.0,CanvasHeight/2.0);

  REQUIRE(TextElementRectangle(projection,parameter,data,anchor).Intersects(VisibleViewport()));

  layouter.RegisterLabel(projection,
                         parameter,
                         false,
                         osmscout::ObjectFileRef(),
                         anchor,
                         data);

  REQUIRE(textLayouter.LayoutCalls()==1);
  REQUIRE(layouter.Labels().size()==1);

  // The label is placed exactly as the unculled pipeline places it: the element is centered on
  // the anchor
  REQUIRE(layouter.Labels().front().elements.size()==1);

  const auto & element=layouter.Labels().front().elements.front();

  REQUIRE(element.x==anchor.GetX()-element.label->width/2.0);
  REQUIRE(element.y==anchor.GetY()-element.label->height/2.0);
}

/**
 * The decision holds for every kind of label element: icon and symbol elements, text elements and
 * element lists (spec map-painter-label-culling, requirement "A label that cannot intersect the
 * view is neither measured nor stored nor laid out", scenario "The same rule holds for every
 * label source").
 */
TEST_CASE("Icon and element list labels outside the view are not stored","[MapPainterLabelCulling]")
{
  TestTextLayouter       textLayouter;
  osmscout::MapParameter parameter=MakeParameter();
  TestLayouter           layouter(&textLayouter);

  ConfigureLayouter(layouter);

  const auto         projection=MakeProjection();

  auto               icon=MakeIconLabel(14.0,14.0);

  osmscout::Vertex2D farOutside(20.0*CanvasWidth,CanvasHeight/2.0);
  osmscout::Vertex2D inside(CanvasWidth/2.0,CanvasHeight/2.0);

  // An icon element far outside the view is not stored, an icon inside the view is
  layouter.RegisterLabel(projection,
                         parameter,
                         false,
                         osmscout::ObjectFileRef(),
                         farOutside,
                         icon);

  REQUIRE(layouter.Labels().empty());

  layouter.RegisterLabel(projection,
                         parameter,
                         false,
                         osmscout::ObjectFileRef(),
                         inside,
                         icon);

  REQUIRE(layouter.Labels().size()==1);

  // An icon element is not measured at all
  REQUIRE(textLayouter.LayoutCalls()==0);

  layouter.Reset();

  // A frame reports its viewport before the labels of its later steps are registered, so the test
  // reports it again after the label instances of the previous frame were dropped
  layouter.SetViewport(VisibleViewport());
  layouter.SetLayoutOverlap(0);

  std::vector<osmscout::LabelData> elements;

  elements.push_back(MakeTextLabel("Hauptstrasse"));
  elements.push_back(MakeIconLabel(14.0,14.0));

  // An element list far outside the view is dropped as a whole, so its text element is not
  // measured either
  layouter.RegisterLabel(projection,
                         parameter,
                         false,
                         osmscout::ObjectFileRef(),
                         farOutside,
                         elements);

  REQUIRE(layouter.Labels().empty());
  REQUIRE(textLayouter.LayoutCalls()==0);

  // The same list inside the view is stored, and its text element is measured once
  layouter.RegisterLabel(projection,
                         parameter,
                         false,
                         osmscout::ObjectFileRef(),
                         inside,
                         elements);

  REQUIRE(layouter.Labels().size()==1);
  REQUIRE(layouter.Labels().front().elements.size()==2);
  REQUIRE(textLayouter.LayoutCalls()==1);
}

/**
 * The number of measurements follows the labels that can appear in the view: a view with many
 * further registered labels outside the view measures and stores no more than the view without
 * them (spec map-painter-label-culling, requirement "The label stage budget follows the labels
 * that can appear").
 */
TEST_CASE("Off-view labels do not add measurements","[MapPainterLabelCulling]")
{
  TestTextLayouter       textLayouter;
  osmscout::MapParameter parameter=MakeParameter();
  TestLayouter           layouter(&textLayouter);

  ConfigureLayouter(layouter);

  const auto       projection=MakeProjection();
  auto             data=MakeTextLabel("Hauptstrasse");

  constexpr size_t farLabelCount=1000;

  for (size_t i=0; i<farLabelCount; i++) {
    osmscout::Vertex2D anchor(6.0*CanvasWidth+10.0*(double)i,
                              6.0*CanvasHeight+10.0*(double)i);

    REQUIRE(!ReachBox(projection,parameter,{data},anchor).Intersects(VisibleViewport()));

    layouter.RegisterLabel(projection,
                           parameter,
                           false,
                           osmscout::ObjectFileRef(),
                           anchor,
                           data);
  }

  REQUIRE(textLayouter.LayoutCalls()==0);
  REQUIRE(layouter.Labels().empty());

  layouter.RegisterLabel(projection,
                         parameter,
                         false,
                         osmscout::ObjectFileRef(),
                         osmscout::Vertex2D(CanvasWidth/2.0,CanvasHeight/2.0),
                         data);

  REQUIRE(textLayouter.LayoutCalls()==1);
  REQUIRE(layouter.Labels().size()==1);
}

/**
 * The stored labels are exactly the labels that can appear in the view: every label whose
 * rectangle intersects the viewport is stored, and no label whose reach cannot touch the layout
 * viewport is stored (spec map-painter-label-culling, requirement "Label set, placement, draw
 * order and measurement results are unchanged").
 */
TEST_CASE("The stored labels are the labels that can appear","[MapPainterLabelCulling]")
{
  TestTextLayouter       textLayouter;
  osmscout::MapParameter parameter=MakeParameter();
  TestLayouter           layouter(&textLayouter);

  ConfigureLayouter(layouter);

  const auto projection=MakeProjection();
  auto       data=MakeTextLabel("Hauptstrasse");

  // Positions from inside the view to far outside of it, on both axes
  std::vector<osmscout::Vertex2D> anchors;

  for (int i=-1; i<=12; i++) {
    anchors.push_back(osmscout::Vertex2D(CanvasWidth/2.0+100.0*(double)i,
                                         CanvasHeight/2.0+100.0*(double)i));
  }

  size_t storedCount=0;
  size_t mustBeStored=0;
  size_t mustNotBeStored=0;
  size_t reachTouches=0;

  for (const auto& anchor : anchors) {
    bool   drawnByTheLabelStage=TextElementRectangle(projection,parameter,data,anchor).Intersects(VisibleViewport());
    bool   reachTouchesViewport=ReachBox(projection,parameter,{data},anchor).Intersects(VisibleViewport());
    size_t measurementsBefore=textLayouter.LayoutCalls();

    if (reachTouchesViewport) {
      reachTouches++;
    }

    layouter.RegisterLabel(projection,
                           parameter,
                           false,
                           osmscout::ObjectFileRef(),
                           anchor,
                           data);

    const auto & stored=layouter.Labels();

    if (drawnByTheLabelStage) {
      // The drawing path would draw this label, so it has to be stored
      REQUIRE(!stored.empty());
      REQUIRE(stored.back().elements.size()==1);
      REQUIRE(stored.back().elements.front().x==
              anchor.GetX()-stored.back().elements.front().label->width/2.0);

      mustBeStored++;
    }
    else if (!reachTouchesViewport) {
      // The label cannot reach the layout viewport, so it must neither be stored nor measured
      mustNotBeStored++;

      REQUIRE(textLayouter.LayoutCalls()==measurementsBefore);

      if (storedCount>0) {
        REQUIRE(stored.size()==storedCount);
      }
      else {
        REQUIRE(stored.empty());
      }

      continue;
    }

    storedCount=stored.size();
  }

  REQUIRE(mustBeStored>0);
  REQUIRE(mustNotBeStored>0);
  REQUIRE(reachTouches>0);

  // A label is measured exactly when its reach can touch the layout viewport, and the measurement
  // is remembered per measurement key (text, font, font size, proposed width, wrapping), so the
  // repeated label of this case is measured once instead of once per registration
  REQUIRE(textLayouter.LayoutCalls()==1);
}

/**
 * A label that is registered before the viewport of the frame is known is kept: the viewport is
 * reported by the drawing target of the backend while the frame is drawn, i.e. after the first
 * steps of the frame registered their labels, so a label of an early step cannot be decided
 * against the viewport of the previous frame (spec map-painter-label-culling, requirement "A label
 * that cannot intersect the view is neither measured nor stored nor laid out").
 */
TEST_CASE("Labels of a frame without a known viewport are kept","[MapPainterLabelCulling]")
{
  TestTextLayouter       textLayouter;
  osmscout::MapParameter parameter=MakeParameter();
  TestLayouter           layouter(&textLayouter);

  ConfigureLayouter(layouter);

  const auto projection=MakeProjection();
  auto       data=MakeTextLabel("Hauptstrasse");

  // The end of a frame resets the layouter, and the viewport of the next frame is not known
  // before its early steps have registered their labels
  layouter.Reset();

  layouter.RegisterLabel(projection,
                         parameter,
                         false,
                         osmscout::ObjectFileRef(),
                         osmscout::Vertex2D(20.0*CanvasWidth,20.0*CanvasHeight),
                         data);

  REQUIRE(textLayouter.LayoutCalls()==1);
  REQUIRE(layouter.Labels().size()==1);

  // After the viewport of the frame is known, the decision applies again
  layouter.SetViewport(VisibleViewport());
  layouter.SetLayoutOverlap(0);

  layouter.RegisterLabel(projection,
                         parameter,
                         false,
                         osmscout::ObjectFileRef(),
                         osmscout::Vertex2D(20.0*CanvasWidth,20.0*CanvasHeight),
                         data);

  REQUIRE(textLayouter.LayoutCalls()==1);
  REQUIRE(layouter.Labels().size()==1);
}

/**
 * The label layout marks an element in a view that is enlarged by the layouter overlap and in a
 * rectangle that is enlarged by a padding, so a label outside the visible view can still suppress
 * a label inside it (spec map-painter-label-culling, requirement "The label stage budget follows
 * the labels that can appear").
 */
TEST_CASE("Labels in the layout margin are kept","[MapPainterLabelCulling]")
{
  TestTextLayouter       textLayouter;
  osmscout::MapParameter parameter=MakeDefaultParameter();
  TestLayouter           layouter(&textLayouter);

  ConfigureLayouter(layouter);

  const auto projection=MakeProjection();
  auto       data=MakeTextLabel("Hauptstrasse");

  double     margin=LayoutMargin(projection,parameter);

  REQUIRE(margin>0.0);

  // A label just outside the visible view, but inside the margin the layout uses, is kept
  osmscout::Vertex2D anchor(CanvasWidth+margin/2.0,CanvasHeight/2.0);

  layouter.RegisterLabel(projection,
                         parameter,
                         false,
                         osmscout::ObjectFileRef(),
                         anchor,
                         data);

  REQUIRE(textLayouter.LayoutCalls()==1);
  REQUIRE(layouter.Labels().size()==1);
}
