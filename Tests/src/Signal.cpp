#include <osmscout/async/Signal.h>

#include <TestMain.h>

#include <string>

TEST_CASE("basic signal <-> slot connection") {
  osmscout::Signal<std::string, double> source;

  std::map<std::string, double> map;
  osmscout::Slot<std::string, double> target([&map](const std::string& str, const double &d) mutable {
    map[str]=d;
  });

  source.Connect(target);

  source.Emit("Pi", 3.1415);
  source.Emit("the meaning of life", 42);

  REQUIRE(map.size()==2);
  REQUIRE(map["Pi"]==3.1415);
  REQUIRE(map["the meaning of life"]==42);
}

TEST_CASE("multiple signal <-> slot connections") {
  osmscout::Signal<std::string, double> source1;
  osmscout::Signal<std::string, double> source2;

  std::map<std::string, double> map1;
  osmscout::Slot<std::string, double> target1([&map1](const std::string& str, const double &d) mutable {
    map1[str]=d;
  });

  std::map<std::string, double> map2;
  osmscout::Slot<std::string, double> target2([&map2](const std::string& str, const double &d) mutable {
    map2[str]=d;
  });

  source1.Connect(target1);
  source2.Connect(target1);

  source1.Emit("A", 1);

  source2.Connect(target2);
  source2.Emit("B", 1);

  REQUIRE(map1.size() == 2);
  REQUIRE(map1["A"] == 1);
  REQUIRE(map1["B"] == 1);

  REQUIRE(map2.size() == 1);
  REQUIRE(map2["B"] == 1);
}

TEST_CASE("slot disconnection on destructor") {
  osmscout::Signal<std::string, double> source;

  std::map<std::string, double> map;
  {
    osmscout::Slot<std::string, double> target([&map](const std::string &str, const double &d) mutable {
      map[str] = d;
    });

    source.Connect(target);

    source.Emit("A", 1);
  }
  source.Emit("B", 2);

  REQUIRE(map.size()==1);
  REQUIRE(map["A"]==1);
}

TEST_CASE("shorter signal live cycle") {
  std::map<std::string, double> map;
  osmscout::Slot<std::string, double> target([&map](const std::string &str, const double &d) mutable {
    map[str] = d;
  });

  {
    osmscout::Signal<std::string, double> source;
    source.Connect(target);
    source.Emit("A", 1);
  }

  {
    osmscout::Signal<std::string, double> source;
    source.Connect(target);
    source.Emit("B", 2);
  }

  REQUIRE(map.size()==2);
  REQUIRE(map["A"]==1);
  REQUIRE(map["B"]==2);
}
