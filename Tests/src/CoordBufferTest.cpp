
#include <osmscout/util/Transformation.h>

#include <catch2/catch_test_macros.hpp>

using namespace osmscout;

TEST_CASE("Check bounding box of generated parallel way")
{
  CoordBuffer      buffer;
  CoordBufferRange range;

  buffer.PushCoord(osmscout::Vertex2D(0.0,0.0));
  buffer.PushCoord(osmscout::Vertex2D(10.0,0.0));
  buffer.PushCoord(osmscout::Vertex2D(5.0,1.0));
  buffer.PushCoord(osmscout::Vertex2D(0.0,0.0));

  range=buffer.GenerateParallelWay(CoordBufferRange(buffer,0,3), /*offset*/-1);
  REQUIRE(range.GetEnd() - range.GetStart() >= 3);
  REQUIRE(range.GetStart() > 3);

  // get bounding box of parallel way
  double minX, maxX, minY, maxY;
  minX = maxX = buffer.buffer[range.GetStart()].GetX();
  minY = maxY = buffer.buffer[range.GetStart()].GetY();
  for (size_t i=range.GetStart(); i<=range.GetEnd(); i++){
    //std::cout << buffer.buffer[i].GetX() << " , " << buffer.buffer[i].GetY() << std::endl;
    minX = std::min(minX, buffer.buffer[i].GetX());
    maxX = std::max(maxX, buffer.buffer[i].GetX());
    minY = std::min(minY, buffer.buffer[i].GetY());
    maxY = std::max(maxY, buffer.buffer[i].GetY());
  }

  REQUIRE(minX > -1);
  REQUIRE(maxX <= 12);
  REQUIRE(minY >= -1);
  REQUIRE(maxY < 3);

  range=buffer.GenerateParallelWay(CoordBufferRange(buffer,0,3), /*offset*/1);
  REQUIRE(range.GetStart() > 7);
}
