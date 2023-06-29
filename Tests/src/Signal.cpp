#include <osmscout/async/Signal.h>

#include <TestMain.h>

#include <string>

TEST_CASE("basic signal <-> slot connection") {
  osmscout::Signal<std::string, double> source;

  std::map<std::string, double> map;
  osmscout::Slot<std::string, double> target([&](const std::string& str, const double &d) mutable {
    map[str]=d;
  });

  source.Connect(target);

  source.Emit("Pi", 3.1415);
  source.Emit("the meaning of life", 42);

  REQUIRE(map.size()==2);
  REQUIRE(map["Pi"]==3.1415);
  REQUIRE(map["the meaning of life"]==42);
}
