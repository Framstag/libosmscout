#include <osmscoutmap/LabelLayouter.h>

#include <osmscout/projection/MercatorProjection.h>

#include <catch2/catch_test_macros.hpp>

using namespace osmscout;

namespace {
  struct TestGlyph {};
  struct TestLabel {};

  using TestLabelType = Label<TestGlyph, TestLabel>;

  class TestTextLayouter
  {
  public:
    // Every text is laid out as a 40 x 20 label
    std::shared_ptr<TestLabelType> Layout(const Projection& /*projection*/,
                                          const MapParameter& /*parameter*/,
                                          const std::string& text,
                                          double fontSize,
                                          double /*objectWidth*/,
                                          bool /*enableWrapping*/,
                                          bool /*contourLabel*/)
    {
      auto label=std::make_shared<TestLabelType>();

      label->text=text;
      label->fontSize=fontSize;
      label->width=40;
      label->height=20;

      return label;
    }
  };

  using TestLayouter = LabelLayouter<TestGlyph, TestLabel, TestTextLayouter>;

  // A 14 x 14 icon followed by a 40 x 20 text
  std::vector<LabelData> IconAndText()
  {
    LabelData icon;

    icon.type=LabelData::Icon;
    icon.iconWidth=14;
    icon.iconHeight=14;

    LabelData text;

    text.type=LabelData::Text;
    text.text="Label";

    return {icon, text};
  }

  std::vector<TestLayouter::LabelInstanceType::Element> RegisterIconAndTextAt(const Vertex2D& point)
  {
    MercatorProjection projection;
    MapParameter       parameter;
    TestTextLayouter   textLayouter;
    TestLayouter       layouter(&textLayouter);

    layouter.RegisterLabel(projection, parameter, false, ObjectFileRef(), point, IconAndText());

    REQUIRE(layouter.Labels().size()==1);

    return layouter.Labels().front().elements;
  }
}

TEST_CASE("Label elements are centred on the point, then stacked below each other")
{
  auto elements=RegisterIconAndTextAt(Vertex2D(100, 100));

  REQUIRE(elements.size()==2);
  REQUIRE(elements[0].y==93);  // icon centred on the point
  REQUIRE(elements[1].y==107); // text right below the icon
}

TEST_CASE("Label elements are stacked the same way for a point above the drawn area")
{
  // The icon ends above the drawn area, at y=-3. The text used to be centred on the point
  // again, over the icon, because a negative offset marked the first element of the stack.
  auto elements=RegisterIconAndTextAt(Vertex2D(100, -10));

  REQUIRE(elements.size()==2);
  REQUIRE(elements[0].y==-17);
  REQUIRE(elements[1].y==-3);
}
