/*
  This source is part of the libosmscout library
  Copyright (C) 2025 Tim Teulings

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

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <osmscoutmap/LabelLayouter.h>

#include <osmscoutmap/MapParameter.h>

#include <osmscout/GeoCoord.h>
#include <osmscout/ObjectRef.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscout/util/Magnification.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace osmscout;

namespace {

constexpr double viewWidth=800.0;
constexpr double viewHeight=600.0;

/**
 * Native glyph/label types for the fake text layouter. They carry no
 * rendering information, only enough structure to instantiate the
 * LabelLayouter template.
 */
struct FakeGlyph {
  char c{'?'};
};

struct FakeLabel {
  // empty: no native label data
};

/**
 * Mock of the backend text layouter contract documented on LabelLayouter
 * (see the template documentation of LabelLayouter). Metrics are
 * deterministic and derived from the text content only:
 * width = 20.0 * character count, height = 20.0.
 */
class FakeTextLayouter
{
public:
  using NativeGlyph=FakeGlyph;
  using NativeLabel=FakeLabel;

  std::shared_ptr<Label<FakeGlyph,FakeLabel>> Layout(const Projection& /*projection*/,
                                                     const MapParameter& /*parameter*/,
                                                     const std::string& text,
                                                     double /*fontSize*/,
                                                     double /*objectWidth*/,
                                                     bool /*enableWrapping*/=false,
                                                     bool /*contourLabel*/=false)
  {
    std::shared_ptr<Label<FakeGlyph,FakeLabel>> label=
        std::make_shared<Label<FakeGlyph,FakeLabel>>();

    label->width=20.0*static_cast<double>(text.size());
    label->height=20.0;
    label->fontSize=10.0;
    label->text=text;

    return label;
  }

  ScreenVectorRectangle GlyphBoundingBox(const FakeGlyph& /*glyph*/) const
  {
    return ScreenVectorRectangle(0.0,0.0,20.0,20.0);
  }
};

struct LabelSpec
{
  ObjectFileRef ref;
  size_t        priority;
  std::string   text;
  double        x;
  double        y;
};

struct PlacedLabel
{
  double x;
  double y;
  double width;
  double height;
};

bool operator==(const PlacedLabel& a, const PlacedLabel& b)
{
  return a.x==b.x && a.y==b.y && a.width==b.width && a.height==b.height;
}

LabelData CreateTextData(const LabelSpec& spec)
{
  LabelData data;

  data.type=LabelData::Type::Text;
  data.priority=spec.priority;
  data.alpha=1.0;
  data.fontSize=10.0;
  data.style=std::make_shared<TextStyle>();
  data.text=spec.text;

  return data;
}

/**
 * Execute one full label layout round for the given labels and viewport
 * with the shared LabelLayouter, without any rendering backend.
 * Returns the position of every label that survived collision resolution,
 * keyed by its object reference.
 */
std::map<ObjectFileRef,PlacedLabel> RunLayout(const Projection& projection,
                                              const MapParameter& parameter,
                                              const ScreenVectorRectangle& viewport,
                                              const std::vector<LabelSpec>& labels,
                                              double shiftX,
                                              double shiftY)
{
  FakeTextLayouter                                    textLayouter;
  LabelLayouter<FakeGlyph,FakeLabel,FakeTextLayouter> layouter(&textLayouter);

  layouter.SetViewport(viewport);
  layouter.SetLayoutOverlap(0);

  for (const LabelSpec& spec : labels) {
    layouter.RegisterLabel(projection,
                           parameter,
                           /*basemap*/ false,
                           spec.ref,
                           Vertex2D(spec.x+shiftX,spec.y+shiftY),
                           std::vector<LabelData>{CreateTextData(spec)});
  }

  layouter.Layout(projection,parameter);

  std::map<ObjectFileRef,PlacedLabel> result;

  for (const auto& instance : layouter.Labels()) {
    for (const auto& element : instance.elements) {
      result[instance.priority.ref]=PlacedLabel(element.x,
                                                element.y,
                                                element.label->width,
                                                element.label->height);
    }
  }

  return result;
}

/**
 * A layout session keeps one LabelLayouter instance across multiple
 * frames, exactly like the renderers do (DrawLabels -> Reset -> next
 * draw). It provides the object identity across frames that the
 * visible-label hysteresis of the layouter works on.
 */
class LayoutSession
{
public:
  LayoutSession(const Projection& projection, MapParameter& parameter)
  : layouter(&textLayouter),
    projection(projection),
    parameter(parameter)
  {
  }

  std::map<ObjectFileRef,PlacedLabel> Frame(const ScreenVectorRectangle& viewport,
                                            const std::vector<LabelSpec>& labels,
                                            double shiftX=0.0,
                                            double shiftY=0.0)
  {
    layouter.Reset();
    layouter.SetViewport(viewport);
    layouter.SetLayoutOverlap(0);

    for (const LabelSpec& spec : labels) {
      layouter.RegisterLabel(projection,
                             parameter,
                             /*basemap*/ false,
                             spec.ref,
                             Vertex2D(spec.x+shiftX,spec.y+shiftY),
                             std::vector<LabelData>{CreateTextData(spec)});
    }

    layouter.Layout(projection,parameter);

    std::map<ObjectFileRef,PlacedLabel> result;

    for (const auto& instance : layouter.Labels()) {
      for (const auto& element : instance.elements) {
        result[instance.priority.ref]=PlacedLabel(element.x,
                                                  element.y,
                                                  element.label->width,
                                                  element.label->height);
      }
    }

    return result;
  }

private:
  FakeTextLayouter                                    textLayouter;
  LabelLayouter<FakeGlyph,FakeLabel,FakeTextLayouter> layouter;
  const Projection&                                   projection;
  MapParameter&                                       parameter;
};

LabelSpec MakeSpec(size_t id,
                   size_t priority,
                   const std::string& text,
                   double x,
                   double y)
{
  return LabelSpec{ObjectFileRef(id,refNode),priority,text,x,y};
}

bool IsFullyInside(const LabelSpec& spec,
                   double visibleWidth,
                   double visibleHeight)
{
  double labelWidth=20.0*static_cast<double>(spec.text.size());

  return spec.x-labelWidth/2>=0.0 &&
         spec.x+labelWidth/2<=visibleWidth &&
         spec.y-10.0>=0.0 &&
         spec.y+10.0<=visibleHeight;
}

/**
 * Assert that a label survives layout in both runs and that its position
 * in the shifted run equals its baseline position plus the shift.
 */
void RequireStableLabel(const std::map<ObjectFileRef,PlacedLabel>& baseline,
                        const std::map<ObjectFileRef,PlacedLabel>& shifted,
                        const LabelSpec& spec,
                        double shiftX,
                        double shiftY)
{
  auto baseIter=baseline.find(spec.ref);
  auto shiftedIter=shifted.find(spec.ref);

  INFO("Label ref " << spec.ref.GetName() << " text '" << spec.text << "' priority " << spec.priority);
  INFO("Shift by (" << shiftX << ", " << shiftY << ")");

  if (baseIter==baseline.end()) {
    FAIL("Label not visible in baseline layout");
  }

  if (shiftedIter==shifted.end()) {
    FAIL("Label visible in baseline layout but invisible in shifted layout");
  }

  PlacedLabel expected{baseIter->second.x+shiftX,
                       baseIter->second.y+shiftY,
                       baseIter->second.width,
                       baseIter->second.height};

  if (!(shiftedIter->second==expected)) {
    FAIL("Position in shifted layout differs from shifted baseline position");
  }
}

std::unique_ptr<Projection> CreateProjection(double width, double height)
{
  auto projection=std::make_unique<MercatorProjection>();

  REQUIRE(projection->Set(GeoCoord(51.0,7.0),
                          /*angle*/ 0.0,
                          Magnification(Magnification::magClose),
                          /*dpi*/ 96.0,
                          static_cast<size_t>(width),
                          static_cast<size_t>(height)));

  return projection;
}

/**
 * Four well separated labels, all fully inside an 800x600 viewport,
 * with clear space between each other.
 */
std::vector<LabelSpec> CreateSpreadScene()
{
  std::vector<LabelSpec> labels;

  labels.push_back(MakeSpec(1,1,"Alpha",150.0,150.0));   // 100..200 x 140..160
  labels.push_back(MakeSpec(2,2,"Beta",400.0,300.0));    // 360..440 x 290..310
  labels.push_back(MakeSpec(3,3,"Gamma",600.0,450.0));   // 550..650 x 440..460
  labels.push_back(MakeSpec(4,4,"Delta",350.0,550.0));   // 300..400 x 540..560

  return labels;
}

} // namespace

TEST_CASE("LabelLayouter: layout without a rendering backend returns label placements", "[labelLayouter]")
{
  std::unique_ptr<Projection> projection=CreateProjection(viewWidth,viewHeight);
  MapParameter parameter;
  std::vector<LabelSpec> labels=CreateSpreadScene();

  // No rendering backend, graphics context or font subsystem is created.
  // The layouter only works on label data and viewport geometry.
  std::map<ObjectFileRef,PlacedLabel> layout=RunLayout(*projection,
                                                       parameter,
                                                       ScreenVectorRectangle(0,0,viewWidth,viewHeight),
                                                       labels,
                                                       0,0);

  REQUIRE(layout.size()==labels.size());

  for (const LabelSpec& spec : labels) {
    auto iter=layout.find(spec.ref);

    REQUIRE(iter!=layout.end());

    double labelWidth=20.0*static_cast<double>(spec.text.size());

    // label is centered on its given point
    REQUIRE(iter->second.x==Catch::Approx(spec.x-labelWidth/2));
    REQUIRE(iter->second.y==Catch::Approx(spec.y-10.0));
    REQUIRE(iter->second.width==Catch::Approx(labelWidth));
    REQUIRE(iter->second.height==Catch::Approx(20.0));
  }
}

TEST_CASE("LabelLayouter: repeated layout with identical viewport is deterministic", "[labelLayouter]")
{
  std::unique_ptr<Projection> projection=CreateProjection(viewWidth,viewHeight);
  MapParameter parameter;

  // two competing labels
  std::vector<LabelSpec> labels;

  labels.push_back(MakeSpec(1,1,"Alpha",400.0,300.0)); // spans 350..450
  labels.push_back(MakeSpec(2,2,"Beta",430.0,300.0));  // spans 390..470, overlaps "Alpha"

  std::map<ObjectFileRef,PlacedLabel> first=RunLayout(*projection,
                                                      parameter,
                                                      ScreenVectorRectangle(0,0,viewWidth,viewHeight),
                                                      labels,
                                                      0,0);
  std::map<ObjectFileRef,PlacedLabel> second=RunLayout(*projection,
                                                       parameter,
                                                       ScreenVectorRectangle(0,0,viewWidth,viewHeight),
                                                       labels,
                                                       0,0);

  REQUIRE(first.size()==second.size());

  for (const auto& entry : first) {
    auto iter=second.find(entry.first);

    REQUIRE(iter!=second.end());
    REQUIRE(iter->second==entry.second);
  }
}
namespace {

/**
 * Pan the scene: all labels and the viewport origin are shifted by the
 * same delta, which is equivalent to panning the map view. Returns the
 * layout of the panned scene relative to its shifted viewport.
 */
std::map<ObjectFileRef,PlacedLabel> RunPannedLayout(const Projection& projection,
                                                    const MapParameter& parameter,
                                                    const std::vector<LabelSpec>& labels,
                                                    double viewX,
                                                    double viewY,
                                                    double shiftX,
                                                    double shiftY)
{
  return RunLayout(projection,
                   parameter,
                   ScreenVectorRectangle(viewX+shiftX,viewY+shiftY,viewWidth,viewHeight),
                   labels,
                   shiftX,
                   shiftY);
}

} // namespace

TEST_CASE("LabelLayouter: horizontal pan does not change visibility of fully visible labels", "[labelLayouter]")
{
  std::unique_ptr<Projection> projection=CreateProjection(viewWidth,viewHeight);
  MapParameter parameter;
  std::vector<LabelSpec> labels=CreateSpreadScene();

  std::map<ObjectFileRef,PlacedLabel> baseline=RunLayout(*projection,
                                                         parameter,
                                                         ScreenVectorRectangle(0,0,viewWidth,viewHeight),
                                                         labels,
                                                         0,0);
  std::map<ObjectFileRef,PlacedLabel> shifted=RunPannedLayout(*projection,
                                                              parameter,
                                                              labels,
                                                              0,0,
                                                              50.0,0.0);

  REQUIRE(baseline.size()==labels.size());
  REQUIRE(shifted.size()==labels.size());

  for (const LabelSpec& spec : labels) {
    REQUIRE(IsFullyInside(spec,viewWidth,viewHeight));
    RequireStableLabel(baseline,shifted,spec,50.0,0.0);
  }
}

TEST_CASE("LabelLayouter: vertical pan does not change visibility of fully visible labels", "[labelLayouter]")
{
  std::unique_ptr<Projection> projection=CreateProjection(viewWidth,viewHeight);
  MapParameter parameter;
  std::vector<LabelSpec> labels=CreateSpreadScene();

  std::map<ObjectFileRef,PlacedLabel> baseline=RunLayout(*projection,
                                                         parameter,
                                                         ScreenVectorRectangle(0,0,viewWidth,viewHeight),
                                                         labels,
                                                         0,0);
  std::map<ObjectFileRef,PlacedLabel> shifted=RunPannedLayout(*projection,
                                                              parameter,
                                                              labels,
                                                              0,0,
                                                              0.0,50.0);

  REQUIRE(baseline.size()==labels.size());
  REQUIRE(shifted.size()==labels.size());

  for (const LabelSpec& spec : labels) {
    REQUIRE(IsFullyInside(spec,viewWidth,viewHeight));
    RequireStableLabel(baseline,shifted,spec,0.0,50.0);
  }
}

TEST_CASE("LabelLayouter: sub-pixel pan does not change visibility of fully visible labels", "[labelLayouter]")
{
  std::unique_ptr<Projection> projection=CreateProjection(viewWidth,viewHeight);
  MapParameter parameter;
  std::vector<LabelSpec> labels=CreateSpreadScene();

  std::map<ObjectFileRef,PlacedLabel> baseline=RunLayout(*projection,
                                                         parameter,
                                                         ScreenVectorRectangle(0,0,viewWidth,viewHeight),
                                                         labels,
                                                         0,0);

  // sub-pixel steps are the typical increments while the map is sliding
  std::map<ObjectFileRef,PlacedLabel> shifted=RunPannedLayout(*projection,
                                                              parameter,
                                                              labels,
                                                              0,0,
                                                              0.5,0.25);

  REQUIRE(baseline.size()==labels.size());
  REQUIRE(shifted.size()==labels.size());

  for (const LabelSpec& spec : labels) {
    REQUIRE(IsFullyInside(spec,viewWidth,viewHeight));
    RequireStableLabel(baseline,shifted,spec,0.5,0.25);
  }
}

TEST_CASE("LabelLayouter: enlarged viewport around the same visible center does not change layout", "[labelLayouter]")
{
  std::unique_ptr<Projection> projection=CreateProjection(viewWidth,viewHeight);
  MapParameter parameter;
  std::vector<LabelSpec> labels=CreateSpreadScene();

  // no map content exists outside the 800x600 visible area in this scene
  std::map<ObjectFileRef,PlacedLabel> baseline=RunLayout(*projection,
                                                         parameter,
                                                         ScreenVectorRectangle(0,0,viewWidth,viewHeight),
                                                         labels,
                                                         0,0);

  // a 1200x900 canvas centered on the same central point as the 800x600
  // viewport: origin (-200,-150); all label points shift accordingly
  double shiftX=200.0;
  double shiftY=150.0;

  std::map<ObjectFileRef,PlacedLabel> enlarged=RunLayout(*projection,
                                                          parameter,
                                                          ScreenVectorRectangle(-200,-150,1200,900),
                                                          labels,
                                                          shiftX,shiftY);

  REQUIRE(baseline.size()==labels.size());
  REQUIRE(enlarged.size()==labels.size());

  for (const LabelSpec& spec : labels) {
    RequireStableLabel(baseline,enlarged,spec,shiftX,shiftY);
  }
}

TEST_CASE("LabelLayouter: collision winner with distinct priorities is independent of pan", "[labelLayouter]")
{
  std::unique_ptr<Projection> projection=CreateProjection(viewWidth,viewHeight);
  MapParameter parameter;

  // two labels of adjacent priorities at the same y position, their
  // rectangles overlap:
  // "Alpha" spans 350..450, "Beta" spans 390..470
  std::vector<LabelSpec> labels;

  labels.push_back(MakeSpec(1,1,"Alpha",400.0,300.0));
  labels.push_back(MakeSpec(2,2,"Beta",430.0,300.0));

  std::map<ObjectFileRef,PlacedLabel> baseline=RunLayout(*projection,
                                                         parameter,
                                                         ScreenVectorRectangle(0,0,viewWidth,viewHeight),
                                                         labels,
                                                         0,0);
  std::map<ObjectFileRef,PlacedLabel> shifted=RunPannedLayout(*projection,
                                                              parameter,
                                                              labels,
                                                              0,0,
                                                              50.0,0.0);

  // both labels are fully inside both viewports, the pan must not
  // influence which of them wins the collision
  REQUIRE(baseline.find(labels[0].ref)!=baseline.end());
  REQUIRE(baseline.find(labels[1].ref)==baseline.end());
  REQUIRE(shifted.find(labels[0].ref)!=shifted.end());
  REQUIRE(shifted.find(labels[1].ref)==shifted.end());
}

namespace {

/**
 * 5 priority classes over the label set to make the competition realistic.
 */
size_t workerPriority(size_t id)
{
  return 1+id%5;
}

/**
 * Fixed-width label text (4 alphabetic + 2 digit characters), so that
 * every label of the dense scene has identical 120px width.
 */
std::string PaddedShopName(size_t id)
{
  std::string idText=std::to_string(id);

  if (idText.size()<2) {
    idText="0"+idText;
  }

  return "Shop"+idText;
}

/**
 * Dense scene (simulating a city center with many competing shop
 * labels): a 9x7 grid of labels, each 120px wide ("ShopNN"), label
 * centers at a horizontal spacing of 70px, so that horizontally
 * adjacent labels overlap heavily. All labels are fully inside the
 * 800x600 viewport.
 */
std::vector<LabelSpec> CreateDenseScene()
{
  std::vector<LabelSpec> labels;
  size_t id=1;

  for (size_t row=0; row<7; row++) {
    for (size_t col=0; col<9; col++) {
      double x=120.0+static_cast<double>(col)*70.0;
      double y=45.0+static_cast<double>(row)*80.0;

      labels.push_back(MakeSpec(id, workerPriority(id), PaddedShopName(id), x, y));

      id++;
    }
  }

  return labels;
}

/**
 * Additional labels outside the 800x600 visible viewport but inside the
 * 1600x1200 enlarged canvas: the oversized canvases rendered by the
 * plane renderer contain such off-screen content, and every such label
 * takes part in collision resolution.
 */
std::vector<LabelSpec> CreateOffViewportLabels()
{
  std::vector<LabelSpec> labels;
  size_t id=100;

  // rows above and below the visible viewport
  for (size_t col=0; col<16; col++) {
    double x=-380.0+static_cast<double>(col)*105.0;

    labels.push_back(MakeSpec(id,workerPriority(id), PaddedShopName(id), x, -15.0));

    id++;

    labels.push_back(MakeSpec(id,workerPriority(id), PaddedShopName(id), x, 615.0));

    id++;
  }

  // columns at the viewport border: their rectangles reach into the
  // visible region and overlap the labels of the dense scene's first
  // and last column. These model content that is present in one
  // oversized render (image swap n) and absent in the next one
  // (image swap n+1) because the request bounding box moved.
  for (size_t row=0; row<7; row++) {
    double y=45.0+static_cast<double>(row)*80.0;

    labels.push_back(MakeSpec(id,0, PaddedShopName(id), 10.0, y));

    id++;

    labels.push_back(MakeSpec(id,0, PaddedShopName(id), 790.0, y));

    id++;
  }

  return labels;
}

} // namespace

TEST_CASE("LabelLayouter: dense scene visibility is stable when the candidate set grows between frames", "[labelLayouter]")
{
  std::unique_ptr<Projection> projection=CreateProjection(viewWidth,viewHeight);
  MapParameter parameter;
  std::vector<LabelSpec> denseLabels=CreateDenseScene();
  std::vector<LabelSpec> offViewport=CreateOffViewportLabels();

  // the renderer keeps one layouter across frames. Frame 1: layout
  // viewport equals the visible viewport, candidates are only the
  // labels of the dense scene.
  LayoutSession session(*projection,parameter);

  std::map<ObjectFileRef,PlacedLabel> baseline=session.Frame(ScreenVectorRectangle(0,0,viewWidth,viewHeight),
                                                             denseLabels);

  // frame 2 (next swap of the oversized canvas): layout viewport is
  // 1600x1200 centered on the same central point (origin -400,-300) and
  // contains additional label content near the border
  std::vector<LabelSpec> allLabels=denseLabels;

  allLabels.insert(allLabels.end(),offViewport.begin(),offViewport.end());

  std::map<ObjectFileRef,PlacedLabel> enlarged=session.Frame(ScreenVectorRectangle(-400,-300,1600,1200),
                                                             allLabels,
                                                             400.0,300.0);

  size_t vanished=0;

  for (const LabelSpec& spec : denseLabels) {
    REQUIRE(IsFullyInside(spec,viewWidth,viewHeight));

    if (baseline.find(spec.ref)==baseline.end()) {
      // baseline itself lost the label (dense scene): not part of the
      // stability assertion, the baseline visible set is the reference
      continue;
    }

    if (enlarged.find(spec.ref)==enlarged.end()) {
      vanished++;
      INFO("Visible label disappeared when the layout viewport was "
           "enlarged with additional border content: ref " << spec.ref.GetName()
           << " text '" << spec.text << "'");
    }
    else {
      RequireStableLabel(baseline,enlarged,spec,400.0,300.0);
    }
  }

  INFO("Labels vanished due to additional border content: " << vanished);

  REQUIRE(vanished==0);
}

TEST_CASE("LabelLayouter: previously visible label precedes new overlapping candidate", "[labelLayouter]")
{
  std::unique_ptr<Projection> projection=CreateProjection(viewWidth,viewHeight);
  MapParameter parameter;

  // frame 1: label A visible alone
  std::vector<LabelSpec> frame1;

  frame1.push_back(MakeSpec(1,1,"Alpha",400.0,300.0)); // 120px wide: 340..460

  LayoutSession session(*projection,parameter);

  auto baseline=session.Frame(ScreenVectorRectangle(0,0,viewWidth,viewHeight),frame1);

  REQUIRE(baseline.find(frame1[0].ref)!=baseline.end());

  // frame 2: new candidate N with the best priority (0), overlapping A,
  // placed next to it. Without hysteresis the priority order would draw
  // N first and hide A.
  std::vector<LabelSpec> frame2=frame1;

  frame2.push_back(MakeSpec(2,0,"Novum",460.0,300.0)); // 400..520: overlaps A

  auto next=session.Frame(ScreenVectorRectangle(0,0,viewWidth,viewHeight),frame2);

  REQUIRE(next.find(frame1[0].ref)!=next.end());
  REQUIRE(next.find(frame2[1].ref)==next.end());

  // frame 3: both previously visible -> priority decides within the group
  std::vector<LabelSpec> frame3;

  frame3.push_back(MakeSpec(1,2,"Alpha",400.0,300.0));
  frame3.push_back(MakeSpec(3,0,"Ceterum",430.0,300.0));

  // "Ceterum" was not visible in frame 2 -> it lands in the newcomer
  // group behind "Alpha" (visible) although its priority is better
  auto third=session.Frame(ScreenVectorRectangle(0,0,viewWidth,viewHeight),frame3);

  REQUIRE(third.find(frame1[0].ref)!=third.end());
  REQUIRE(third.find(frame3[1].ref)==third.end());
}

TEST_CASE("LabelLayouter: session state survives reset and follows the last layout", "[labelLayouter]")
{
  std::unique_ptr<Projection> projection=CreateProjection(viewWidth,viewHeight);
  MapParameter parameter;

  std::vector<LabelSpec> frame1;

  frame1.push_back(MakeSpec(1,1,"Alpha",150.0,150.0));
  frame1.push_back(MakeSpec(2,2,"Beta",450.0,300.0));

  LayoutSession session(*projection,parameter);

  auto first=session.Frame(ScreenVectorRectangle(0,0,viewWidth,viewHeight),frame1);

  REQUIRE(first.find(frame1[0].ref)!=first.end());
  REQUIRE(first.find(frame1[1].ref)!=first.end());

  // frame 2: same content -> same visible set; the reset between the
  // frames must not have discarded the hysteresis state
  auto second=session.Frame(ScreenVectorRectangle(0,0,viewWidth,viewHeight),frame1);

  REQUIRE(second==first);

  // frame 3: Beta is not registered anymore (left the viewport/asks gone).
  // The state must follow the visible set of the last layout: the layout
  // without Beta must equal a fresh instance layout of the same content.
  std::vector<LabelSpec> frame3;

  frame3.push_back(MakeSpec(1,1,"Alpha",150.0,150.0));
  frame3.push_back(MakeSpec(3,3,"Gamma",400.0,150.0));

  auto third=session.Frame(ScreenVectorRectangle(0,0,viewWidth,viewHeight),frame3);

  std::map<ObjectFileRef,PlacedLabel> fresh=RunLayout(*projection,
                                                      parameter,
                                                      ScreenVectorRectangle(0,0,viewWidth,viewHeight),
                                                      frame3,
                                                      0,0);

  REQUIRE(third==fresh);
}

TEST_CASE("LabelLayouter: session rounds stay identical for identical input", "[labelLayouter]")
{
  std::unique_ptr<Projection> projection=CreateProjection(viewWidth,viewHeight);
  MapParameter parameter;
  std::vector<LabelSpec> labels=CreateSpreadScene();

  LayoutSession session(*projection,parameter);

  auto first=session.Frame(ScreenVectorRectangle(0,0,viewWidth,viewHeight),labels);
  auto second=session.Frame(ScreenVectorRectangle(0,0,viewWidth,viewHeight),labels);
  auto third=session.Frame(ScreenVectorRectangle(0,0,viewWidth,viewHeight),labels);

  REQUIRE(first==second);
  REQUIRE(second==third);
}

TEST_CASE("LabelLayouter: previously visible labels survive single-pixel mask jitter", "[labelLayouter]")
{
  std::unique_ptr<Projection> projection=CreateProjection(viewWidth,viewHeight);
  MapParameter parameter;

  // no padding: the mask rectangles are plain truncations of the label
  // positions -> deterministic reproduction of the jitter mechanism
  parameter.SetLabelPadding(0.0);

  // two labels 100px wide, masks exactly adjacent in frame 1:
  // A covers 400..499 (element x 400.3), B covers 500..599 (element x 500.0)
  std::vector<LabelSpec> frame1;

  frame1.push_back(MakeSpec(1,1,"Alpha",400.3,300.0));
  frame1.push_back(MakeSpec(2,2,"Berta",500.0,300.0));

  LayoutSession session(*projection,parameter);

  auto first=session.Frame(ScreenVectorRectangle(0,0,viewWidth,viewHeight),frame1);

  REQUIRE(first.find(frame1[0].ref)!=first.end());
  REQUIRE(first.find(frame1[1].ref)!=first.end());

  // the identical scene shifted by fractional deltas: the truncation of
  // the fractional positions moves the two masks +-1px against each other
  // (A crosses a pixel boundary while B does not, and vice versa) ->
  // the masks collide in single frames although the real label distance
  // is unchanged (~50px). Without tolerance B flips visible/hidden.
  for (double shift : {0.8, 2.0, 0.8, 2.0}) {
    std::vector<LabelSpec> frame=frame1;

    std::map<ObjectFileRef,PlacedLabel> round=session.Frame(ScreenVectorRectangle(0,0,viewWidth,viewHeight),frame,shift,0.0);

    INFO("shift " << shift);

    REQUIRE(round.find(frame1[0].ref)!=round.end());
    REQUIRE(round.find(frame1[1].ref)!=round.end());
  }
}
