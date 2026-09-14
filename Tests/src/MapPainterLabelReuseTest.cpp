#include <osmscout/GeoCoord.h>
#include <osmscout/ObjectRef.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscout/projection/Projection.h>
#include <osmscout/util/Magnification.h>
#include <osmscoutmap/LabelLayouter.h>
#include <osmscoutmap/MapParameter.h>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cstdlib>
#include <memory>
#include <new>
#include <string>
#include <vector>

/*
 * Tests for the contract of the label stage of the map painter: a label measurement is reused
 * while its measurement inputs and the measurement environment are unchanged, a label's glyph
 * data is derived once per measured label, the label stage's scratch storage is reused, the
 * results stay those of a fresh measurement, and the label set, placement and draw order stay
 * unchanged.
 *
 * The tests drive a LabelLayouter with a counting fake text layouter, so they need no database,
 * no stylesheet and no rendering backend. The allocation counter replaces the global
 * operator new/delete (the same approach as Tests/src/PerformanceTest.cpp); it is disabled in
 * sanitizer builds, whose runtimes provide their own operators.
 */

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define LABEL_REUSE_HAVE_ALLOCATION_COUNTER 0
#elif defined(__has_feature)
#if __has_feature(memory_sanitizer) || __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define LABEL_REUSE_HAVE_ALLOCATION_COUNTER 0
#else
#define LABEL_REUSE_HAVE_ALLOCATION_COUNTER 1
#endif
#else
#define LABEL_REUSE_HAVE_ALLOCATION_COUNTER 1
#endif

#if LABEL_REUSE_HAVE_ALLOCATION_COUNTER

namespace {

  /**
   * Counts every heap allocation of the test process. The difference between two calls measures
   * the allocation volume of the code in between, which is the property that tells whether the
   * label stage allocates per label and per frame or reuses its buffers.
   */
  std::atomic<size_t> allocationCounter{0};
}

/*
 * The replaced operators pair malloc with free. GCC reports that pairing as a mismatched
 * new/delete (-Wmismatched-new-delete) wherever the label layouter template is instantiated in
 * the same translation unit, which this test does on purpose, so the diagnostic is disabled
 * around the operators.
 */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

void* operator new(std::size_t size)
{
  allocationCounter.fetch_add(1,std::memory_order_relaxed);

  void *memory=std::malloc(size);

  if (memory==nullptr) {
    throw std::bad_alloc();
  }

  return memory;
}

void* operator new[](std::size_t size)
{
  return ::operator new(size);
}

void* operator new(std::size_t size,
                   const std::nothrow_t&) noexcept
{
  allocationCounter.fetch_add(1,std::memory_order_relaxed);

  return std::malloc(size);
}

void* operator new[](std::size_t size,
                     const std::nothrow_t& tag) noexcept
{
  return ::operator new(size,tag);
}

void operator delete(void* memory) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory) noexcept
{
  std::free(memory);
}

void operator delete(void* memory,
                     std::size_t /*size*/) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory,
                       std::size_t /*size*/) noexcept
{
  std::free(memory);
}

void operator delete(void* memory,
                     const std::nothrow_t&) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory,
                       const std::nothrow_t&) noexcept
{
  std::free(memory);
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif

namespace {

#if LABEL_REUSE_HAVE_ALLOCATION_COUNTER

  size_t GetAllocationCount()
  {
    return allocationCounter.load(std::memory_order_relaxed);
  }

#endif

  /**
   * Native label and glyph types of the fake text layouter. The glyph data the layouter derives
   * from a label is counted, which is how the tests observe the reuse.
   */
  struct TestNativeLabel
  {
    std::vector<double> advances;
  };

  struct TestNativeGlyph
  {
    double advance=0.0;
  };

  using TestGlyph = osmscout::Glyph<TestNativeGlyph>;
  using TestLabel = osmscout::Label<TestNativeGlyph,TestNativeLabel>;
  using TestLabelInstance = osmscout::LabelInstance<TestNativeGlyph,TestNativeLabel>;
  using TestContourLabel = osmscout::ContourLabel<TestNativeGlyph>;

  /**
   * Number of glyph derivations of the whole test process. Counted in the ToGlyphs()
   * specialization below, because the layouter calls it through the label.
   */
  size_t glyphDerivationCount=0;

  /**
   * Stand-in for a backend: measures text with a fixed character width and counts the
   * measurements it is asked for.
   */
  class FakeTextLayouter
  {
  public:
    size_t measurementCount=0;
    double characterWidth=7.0;
    double lineHeight=0.5;

  public:
    std::shared_ptr<TestLabel> Layout(const osmscout::Projection& /*projection*/,
                                      const osmscout::MapParameter& /*parameter*/,
                                      const std::string& text,
                                      double fontSize,
                                      double objectWidth,
                                      bool /*enableWrapping*/,
                                      bool /*contourLabel*/)
    {
      measurementCount++;

      auto label=std::make_shared<TestLabel>();

      label->text=text;
      label->fontSize=fontSize;
      label->width=characterWidth*static_cast<double>(text.size());
      label->height=fontSize*lineHeight;

      label->label.advances.clear();

      for (size_t i=0; i<text.size(); i++) {
        label->label.advances.push_back(characterWidth);
      }

      // The proposed width is not used for measuring in this fake; it is part of the
      // measurement key and therefore still separates different registrations
      (void)objectWidth;

      return label;
    }

    osmscout::ScreenVectorRectangle GlyphBoundingBox(const TestNativeGlyph& glyph) const
    {
      return osmscout::ScreenVectorRectangle(0.0,0.0,glyph.advance,8.0);
    }
  };

  using TestLayouter = osmscout::LabelLayouter<TestNativeGlyph,TestNativeLabel,FakeTextLayouter>;

  osmscout::MercatorProjection MakeProjection()
  {
    osmscout::MercatorProjection projection;

    REQUIRE(projection.Set(osmscout::GeoCoord(50.001,8.001),
                           osmscout::Magnification(osmscout::Magnification::magClose),
                           400,
                           400,
                           400));

    return projection;
  }

  osmscout::LabelData MakeLabelData(const std::string& text,
                                    double fontSize,
                                    size_t priority)
  {
    osmscout::LabelData data;

    data.type=osmscout::LabelData::Type::Text;
    data.priority=priority;
    data.alpha=1.0;
    data.fontSize=fontSize;
    data.text=text;

    return data;
  }

  void RegisterTextLabel(TestLayouter& layouter,
                         const osmscout::Projection& projection,
                         const osmscout::MapParameter& parameter,
                         const std::string& text,
                         double fontSize,
                         double objectWidth,
                         double x,
                         double y,
                         size_t priority=1)
  {
    std::vector<osmscout::LabelData> data{MakeLabelData(text,fontSize,priority)};

    layouter.RegisterLabel(projection,
                           parameter,
                           false,
                           osmscout::ObjectFileRef(),
                           osmscout::Vertex2D(x,y),
                           data,
                           objectWidth);
  }

  osmscout::LabelPath MakeLabelPath(double length)
  {
    osmscout::LabelPath path;

    path.AddPoint(osmscout::Vertex2D(0.0,100.0));
    path.AddPoint(osmscout::Vertex2D(length,100.0));

    return path;
  }

  void RegisterPathLabel(TestLayouter& layouter,
                         const osmscout::Projection& projection,
                         const osmscout::MapParameter& parameter,
                         const std::string& text,
                         double fontSize)
  {
    osmscout::PathLabelData data;

    data.priority=1;
    data.text=text;
    data.height=fontSize;
    data.contourLabelOffset=2.0;
    data.contourLabelSpace=10.0;

    layouter.RegisterContourLabel(projection,
                                  parameter,
                                  false,
                                  osmscout::ObjectFileRef(),
                                  data,
                                  MakeLabelPath(400.0));
  }

  size_t CountElements(const std::vector<TestLabelInstance>& instances)
  {
    size_t count=0;

    for (const auto& instance : instances) {
      count+=instance.elements.size();
    }

    return count;
  }
}

namespace osmscout {

  // The layouter derives the glyph data of a label through this hook; the tests count the
  // derivations, which is how they observe that a label's glyphs are derived only once
  template<>
  std::vector<Glyph<TestNativeGlyph>> Label<TestNativeGlyph,TestNativeLabel>::ToGlyphs() const
                                        {
    ::glyphDerivationCount++;

    std::vector<Glyph<TestNativeGlyph>> result;

    for (size_t i=0; i<label.advances.size(); i++) {
      Glyph<TestNativeGlyph> glyph;

      glyph.glyph.advance=label.advances[i];
      glyph.position=Vertex2D(label.advances[i]*static_cast<double>(i),0.0);

      result.push_back(glyph);
    }

    return result;
  }
}

TEST_CASE("A repeated frame measures no label again","[MapPainterLabelReuse]")
{
  FakeTextLayouter       fake;
  TestLayouter           layouter(&fake);
  auto                   projection=MakeProjection();
  osmscout::MapParameter parameter;

  layouter.SetViewport(osmscout::ScreenVectorRectangle(0,0,400,400));
  layouter.SetLayoutOverlap(0);

  // First frame: five distinct labels, some of them registered more than once
  RegisterTextLabel(layouter,projection,parameter,"Main Street",12.0,120.0,50,50);
  RegisterTextLabel(layouter,projection,parameter,"Market Square",12.0,120.0,150,50);
  RegisterTextLabel(layouter,projection,parameter,"Main Street",12.0,120.0,250,50);
  RegisterTextLabel(layouter,projection,parameter,"Station Road",12.0,120.0,50,150);
  RegisterTextLabel(layouter,projection,parameter,"Main Street",12.0,120.0,150,150);
  RegisterTextLabel(layouter,projection,parameter,"Church Lane",12.0,120.0,250,150);

  size_t measurementsOfFirstFrame=fake.measurementCount;

  REQUIRE(measurementsOfFirstFrame>0);
  REQUIRE(measurementsOfFirstFrame<=6);

  layouter.Layout(projection,parameter);
  layouter.Reset();

  // Second frame: the same registrations
  RegisterTextLabel(layouter,projection,parameter,"Main Street",12.0,120.0,50,50);
  RegisterTextLabel(layouter,projection,parameter,"Market Square",12.0,120.0,150,50);
  RegisterTextLabel(layouter,projection,parameter,"Main Street",12.0,120.0,250,50);
  RegisterTextLabel(layouter,projection,parameter,"Station Road",12.0,120.0,50,150);
  RegisterTextLabel(layouter,projection,parameter,"Main Street",12.0,120.0,150,150);
  RegisterTextLabel(layouter,projection,parameter,"Church Lane",12.0,120.0,250,150);

  REQUIRE(fake.measurementCount==measurementsOfFirstFrame);

  layouter.Layout(projection,parameter);
  layouter.Reset();

  // A third frame with many more labels does not add measurements for the known labels
  for (size_t i=0; i<10; i++) {
    RegisterTextLabel(layouter,projection,parameter,"Main Street",12.0,120.0,50.0+3.0*i,250);
    RegisterTextLabel(layouter,projection,parameter,"Market Square",12.0,120.0,150.0+3.0*i,300);
  }

  REQUIRE(fake.measurementCount==measurementsOfFirstFrame);
}

TEST_CASE("A changed measurement input is measured again","[MapPainterLabelReuse]")
{
  FakeTextLayouter       fake;
  TestLayouter           layouter(&fake);
  auto                   projection=MakeProjection();
  osmscout::MapParameter parameter;

  layouter.SetViewport(osmscout::ScreenVectorRectangle(0,0,400,400));
  layouter.SetLayoutOverlap(0);

  RegisterTextLabel(layouter,projection,parameter,"Main Street",12.0,120.0,50,50);
  layouter.Layout(projection,parameter);
  layouter.Reset();

  size_t afterFirstFrame=fake.measurementCount;

  // Same text, other font size
  RegisterTextLabel(layouter,projection,parameter,"Main Street",20.0,120.0,50,50);
  layouter.Layout(projection,parameter);
  layouter.Reset();

  REQUIRE(fake.measurementCount==afterFirstFrame+1);

  // Same text and font size, other proposed width
  RegisterTextLabel(layouter,projection,parameter,"Main Street",20.0,240.0,50,50);
  layouter.Layout(projection,parameter);
  layouter.Reset();

  REQUIRE(fake.measurementCount==afterFirstFrame+2);
}

TEST_CASE("A changed measurement environment is measured again","[MapPainterLabelReuse]")
{
  FakeTextLayouter       fake;
  TestLayouter           layouter(&fake);
  auto                   projection=MakeProjection();
  osmscout::MapParameter parameter;

  layouter.SetViewport(osmscout::ScreenVectorRectangle(0,0,400,400));
  layouter.SetLayoutOverlap(0);

  layouter.SetMeasurementEnvironment("dpi=96");

  RegisterTextLabel(layouter,projection,parameter,"Main Street",12.0,120.0,50,50);
  layouter.Layout(projection,parameter);
  layouter.Reset();

  REQUIRE(fake.measurementCount==1);
  REQUIRE(layouter.GetMeasurementCount()==1);

  // Same environment: the measurement is reused
  RegisterTextLabel(layouter,projection,parameter,"Main Street",12.0,120.0,50,50);
  layouter.Layout(projection,parameter);
  layouter.Reset();

  REQUIRE(fake.measurementCount==1);

  // Another environment: the measurement is dropped and made again
  layouter.SetMeasurementEnvironment("dpi=192");

  REQUIRE(layouter.GetMeasurementCount()==0);

  RegisterTextLabel(layouter,projection,parameter,"Main Street",12.0,120.0,50,50);
  layouter.Layout(projection,parameter);
  layouter.Reset();

  REQUIRE(fake.measurementCount==2);
}

TEST_CASE("Reuse does not depend on the label having been drawn","[MapPainterLabelReuse]")
{
  FakeTextLayouter       fake;
  TestLayouter           layouter(&fake);
  auto                   projection=MakeProjection();
  osmscout::MapParameter parameter;

  layouter.SetViewport(osmscout::ScreenVectorRectangle(0,0,400,400));
  layouter.SetLayoutOverlap(0);

  // Two labels at the same point with the same priority: the second one loses the overlap
  // resolution and is not drawn
  RegisterTextLabel(layouter,projection,parameter,"Main Street",12.0,120.0,200,200,1);
  RegisterTextLabel(layouter,projection,parameter,"Market Square",12.0,120.0,200,200,1);

  layouter.Layout(projection,parameter);

  REQUIRE(fake.measurementCount==2);
  REQUIRE(layouter.Labels().size()==1);
  REQUIRE(CountElements(layouter.Labels())==1);

  layouter.Reset();

  // The label that lost the resolution is registered again: its measurement is still reused
  RegisterTextLabel(layouter,projection,parameter,"Main Street",12.0,120.0,200,200,1);
  RegisterTextLabel(layouter,projection,parameter,"Market Square",12.0,120.0,200,200,1);

  REQUIRE(fake.measurementCount==2);

  layouter.Layout(projection,parameter);

  // The drawn label of the two colliding registrations is "Main Street" (it was registered
  // first). The reused measurement of the same text reports the dimensions of a fresh
  // measurement: a layouter that never saw the label measures the same
  REQUIRE(layouter.Labels().size()==1);
  REQUIRE(layouter.Labels()[0].elements.size()==1);
  REQUIRE(layouter.Labels()[0].elements[0].labelData.text=="Main Street");

  double widthOfReusedLabel=layouter.Labels()[0].elements[0].label->width;
  double heightOfReusedLabel=layouter.Labels()[0].elements[0].label->height;

  REQUIRE(widthOfReusedLabel>0.0);

  FakeTextLayouter freshFake;
  TestLayouter     freshLayouter(&freshFake);

  freshLayouter.SetViewport(osmscout::ScreenVectorRectangle(0,0,400,400));
  freshLayouter.SetLayoutOverlap(0);

  RegisterTextLabel(freshLayouter,projection,parameter,"Main Street",12.0,120.0,200,200,1);
  freshLayouter.Layout(projection,parameter);

  REQUIRE(freshFake.measurementCount==1);
  REQUIRE(freshLayouter.Labels().size()==1);
  REQUIRE(freshLayouter.Labels()[0].elements[0].label->width==widthOfReusedLabel);
  REQUIRE(freshLayouter.Labels()[0].elements[0].label->height==heightOfReusedLabel);

  layouter.Reset();
}

TEST_CASE("Glyphs of a label that takes part in two frames are derived once","[MapPainterLabelReuse]")
{
  FakeTextLayouter       fake;
  TestLayouter           layouter(&fake);
  auto                   projection=MakeProjection();
  osmscout::MapParameter parameter;

  layouter.SetViewport(osmscout::ScreenVectorRectangle(0,0,400,400));
  layouter.SetLayoutOverlap(0);

  glyphDerivationCount=0;

  RegisterPathLabel(layouter,projection,parameter,"Main Street",12.0);

  layouter.Layout(projection,parameter);

  REQUIRE(fake.measurementCount==1);
  REQUIRE(glyphDerivationCount==1);
  REQUIRE(!layouter.ContourLabels().empty());

  auto glyphsOfFirstFrame=layouter.ContourLabels()[0].glyphs;

  layouter.Reset();

  RegisterPathLabel(layouter,projection,parameter,"Main Street",12.0);

  layouter.Layout(projection,parameter);

  // Both the measurement and the glyph data of the label are reused
  REQUIRE(fake.measurementCount==1);
  REQUIRE(glyphDerivationCount==1);

  auto glyphsOfSecondFrame=layouter.ContourLabels()[0].glyphs;

  REQUIRE(glyphsOfFirstFrame.size()==glyphsOfSecondFrame.size());

  for (size_t i=0; i<glyphsOfFirstFrame.size(); i++) {
    REQUIRE(glyphsOfFirstFrame[i].position.GetX()==glyphsOfSecondFrame[i].position.GetX());
    REQUIRE(glyphsOfFirstFrame[i].position.GetY()==glyphsOfSecondFrame[i].position.GetY());
    REQUIRE(glyphsOfFirstFrame[i].trPosition.GetX()==glyphsOfSecondFrame[i].trPosition.GetX());
    REQUIRE(glyphsOfFirstFrame[i].trPosition.GetY()==glyphsOfSecondFrame[i].trPosition.GetY());
    REQUIRE(glyphsOfFirstFrame[i].trWidth==glyphsOfSecondFrame[i].trWidth);
    REQUIRE(glyphsOfFirstFrame[i].trHeight==glyphsOfSecondFrame[i].trHeight);
  }

  // A fresh layouter derives the same glyph data
  FakeTextLayouter freshFake;
  TestLayouter     freshLayouter(&freshFake);

  freshLayouter.SetViewport(osmscout::ScreenVectorRectangle(0,0,400,400));
  freshLayouter.SetLayoutOverlap(0);

  glyphDerivationCount=0;

  RegisterPathLabel(freshLayouter,projection,parameter,"Main Street",12.0);
  freshLayouter.Layout(projection,parameter);

  REQUIRE(glyphDerivationCount==1);

  auto freshGlyphs=freshLayouter.ContourLabels()[0].glyphs;

  REQUIRE(freshGlyphs.size()==glyphsOfSecondFrame.size());

  for (size_t i=0; i<freshGlyphs.size(); i++) {
    REQUIRE(freshGlyphs[i].position.GetX()==glyphsOfSecondFrame[i].position.GetX());
    REQUIRE(freshGlyphs[i].trWidth==glyphsOfSecondFrame[i].trWidth);
    REQUIRE(freshGlyphs[i].trHeight==glyphsOfSecondFrame[i].trHeight);
  }

  // An environment change drops the reused glyph data with the measurement
  layouter.SetMeasurementEnvironment("other");

  REQUIRE(layouter.GetMeasurementCount()==0);

  RegisterPathLabel(layouter,projection,parameter,"Main Street",12.0);
  layouter.Layout(projection,parameter);

  REQUIRE(glyphDerivationCount==2);
}

TEST_CASE("The label set, placement and order are unchanged","[MapPainterLabelReuse]")
{
  FakeTextLayouter       fake;
  TestLayouter           layouter(&fake);
  auto                   projection=MakeProjection();
  osmscout::MapParameter parameter;

  layouter.SetViewport(osmscout::ScreenVectorRectangle(0,0,400,400));
  layouter.SetLayoutOverlap(0);

  auto registerLabels=[&]() {
                         RegisterTextLabel(layouter,projection,parameter,"Main Street",12.0,120.0,50,50);
                         RegisterTextLabel(layouter,projection,parameter,"Market Square",16.0,120.0,150,50,2);
                         RegisterTextLabel(layouter,projection,parameter,"Station Road",12.0,120.0,250,50,3);
                         RegisterTextLabel(layouter,projection,parameter,"Church Lane",10.0,120.0,50,250,4);
                       };

  registerLabels();
  layouter.Layout(projection,parameter);

  std::vector<TestLabelInstance> firstFrame=layouter.Labels();

  layouter.Reset();

  registerLabels();
  layouter.Layout(projection,parameter);

  std::vector<TestLabelInstance> secondFrame=layouter.Labels();

  REQUIRE(firstFrame.size()==secondFrame.size());
  REQUIRE(firstFrame.size()>0);

  for (size_t i=0; i<firstFrame.size(); i++) {
    REQUIRE(firstFrame[i].priority.priority==secondFrame[i].priority.priority);
    REQUIRE(firstFrame[i].elements.size()==secondFrame[i].elements.size());

    for (size_t e=0; e<firstFrame[i].elements.size(); e++) {
      REQUIRE(firstFrame[i].elements[e].x==secondFrame[i].elements[e].x);
      REQUIRE(firstFrame[i].elements[e].y==secondFrame[i].elements[e].y);
      REQUIRE(firstFrame[i].elements[e].labelData.text==secondFrame[i].elements[e].labelData.text);
      REQUIRE(firstFrame[i].elements[e].label->width==secondFrame[i].elements[e].label->width);
      REQUIRE(firstFrame[i].elements[e].label->height==secondFrame[i].elements[e].label->height);
    }
  }
}

#if LABEL_REUSE_HAVE_ALLOCATION_COUNTER

TEST_CASE("The label stage allocates a constant amount per frame","[MapPainterLabelReuse]")
{
  FakeTextLayouter       fake;
  TestLayouter           layouter(&fake);
  auto                   projection=MakeProjection();
  osmscout::MapParameter parameter;

  layouter.SetViewport(osmscout::ScreenVectorRectangle(0,0,400,400));
  layouter.SetLayoutOverlap(0);

  auto registerLabels=[&]() {
                         for (size_t i=0; i<40; i++) {
                           RegisterTextLabel(layouter,
                                             projection,
                                             parameter,
                                             "Label "+std::to_string(i),
                                             12.0,
                                             120.0,
                                             10.0+8.0*i,
                                             60.0+8.0*i);
                         }
                       };

  // First frame warms the buffers up
  registerLabels();
  layouter.Layout(projection,parameter);
  layouter.Reset();

  registerLabels();

  size_t allocationsBefore=GetAllocationCount();

  layouter.Layout(projection,parameter);

  size_t allocationsOfSecondFrame=GetAllocationCount()-allocationsBefore;

  // The frame's own allocations (the label data of the resolved instances and its strings)
  // do not grow with the frame number
  layouter.Reset();

  registerLabels();

  allocationsBefore=GetAllocationCount();

  layouter.Layout(projection,parameter);

  size_t allocationsOfThirdFrame=GetAllocationCount()-allocationsBefore;

  REQUIRE(allocationsOfThirdFrame<=allocationsOfSecondFrame);
  REQUIRE(allocationsOfThirdFrame<40);
}

TEST_CASE("Labels that do not take part in the frame do not allocate","[MapPainterLabelReuse]")
{
  FakeTextLayouter       fake;
  TestLayouter           layouter(&fake);
  auto                   projection=MakeProjection();
  osmscout::MapParameter parameter;

  layouter.SetViewport(osmscout::ScreenVectorRectangle(0,0,400,400));
  layouter.SetLayoutOverlap(0);

  // Five labels that take part in the frame, stacked far enough apart that they do not collide,
  // plus a configurable number of labels that lose the overlap resolution because they are
  // registered at the same point
  auto registerLabels=[&](size_t hiddenCount) {
                         for (size_t i=0; i<5; i++) {
                           RegisterTextLabel(layouter,
                                             projection,
                                             parameter,
                                             "Visible "+std::to_string(i),
                                             12.0,
                                             120.0,
                                             200.0,
                                             40.0+70.0*i,
                                             i+1);
                         }

                         for (size_t i=0; i<hiddenCount; i++) {
                           RegisterTextLabel(layouter,
                                             projection,
                                             parameter,
                                             "Hidden "+std::to_string(i),
                                             12.0,
                                             120.0,
                                             200.0,
                                             40.0,
                                             100+i);
                         }
                       };

  // Warm up with the light view, then measure the light view
  registerLabels(0);
  layouter.Layout(projection,parameter);

  size_t lightLabelCount=layouter.Labels().size();

  REQUIRE(lightLabelCount>0);

  layouter.Reset();

  registerLabels(0);

  size_t allocationsBefore=GetAllocationCount();

  layouter.Layout(projection,parameter);

  size_t lightAllocations=GetAllocationCount()-allocationsBefore;

  layouter.Reset();

  // The heavy view registers the same visible labels plus 500 hidden ones
  registerLabels(500);
  layouter.Layout(projection,parameter);

  REQUIRE(layouter.Labels().size()==lightLabelCount);

  layouter.Reset();

  registerLabels(500);

  allocationsBefore=GetAllocationCount();

  layouter.Layout(projection,parameter);

  size_t heavyAllocations=GetAllocationCount()-allocationsBefore;

  REQUIRE(heavyAllocations<=lightAllocations+8);
}

#endif

TEST_CASE("The measurement cache stays within its bound","[MapPainterLabelReuse]")
{
  FakeTextLayouter       fake;
  TestLayouter           layouter(&fake,4);
  auto                   projection=MakeProjection();
  osmscout::MapParameter parameter;

  layouter.SetViewport(osmscout::ScreenVectorRectangle(0,0,400,400));
  layouter.SetLayoutOverlap(0);

  REQUIRE(layouter.GetMaxMeasurementCount()==4);

  // Three labels fit into the bound without eviction, so a repeated frame reuses them
  auto registerThreeLabels=[&]() {
                              for (size_t i=0; i<3; i++) {
                                RegisterTextLabel(layouter,
                                                  projection,
                                                  parameter,
                                                  "Label "+std::to_string(i),
                                                  12.0,
                                                  120.0,
                                                  20.0+30.0*i,
                                                  100.0);
                              }
                            };

  registerThreeLabels();
  layouter.Layout(projection,parameter);
  layouter.Reset();

  REQUIRE(fake.measurementCount==3);
  REQUIRE(layouter.GetMeasurementCount()==3);

  registerThreeLabels();
  layouter.Layout(projection,parameter);
  layouter.Reset();

  REQUIRE(fake.measurementCount==3);

  // Ten labels stay within the bound: the oldest measurements are dropped
  for (size_t i=0; i<10; i++) {
    RegisterTextLabel(layouter,
                      projection,
                      parameter,
                      "Other "+std::to_string(i),
                      12.0,
                      120.0,
                      20.0+30.0*i,
                      150.0);
  }

  layouter.Layout(projection,parameter);

  REQUIRE(fake.measurementCount==13);
  REQUIRE(layouter.GetMeasurementCount()<=4);

  layouter.Reset();

  // The measurements that are still remembered are reused
  size_t measurementsOfPreviousFrames=fake.measurementCount;
  size_t measurementsRemembered=layouter.GetMeasurementCount();

  REQUIRE(measurementsRemembered>0);

  for (size_t i=0; i<10; i++) {
    RegisterTextLabel(layouter,
                      projection,
                      parameter,
                      "Other "+std::to_string(i),
                      12.0,
                      120.0,
                      20.0+30.0*i,
                      150.0);
  }

  layouter.Layout(projection,parameter);

  // The remembered measurements are reused, the dropped ones are measured again; the frame
  // therefore performs at most 10 measurements and at least the 10 minus the remembered ones
  REQUIRE(fake.measurementCount>=measurementsOfPreviousFrames+10-measurementsRemembered);
  REQUIRE(fake.measurementCount<=measurementsOfPreviousFrames+10);
  REQUIRE(layouter.GetMeasurementCount()<=4);
}
