
#include <osmscout/util/Transformation.h>

#include <TestMain.h>

using namespace osmscout;

TEST_CASE("Check bounding box of generated parallel way")
{
  CoordBuffer      buffer;
  CoordBufferRange range;

  buffer.PushCoord(0,0);
  buffer.PushCoord(10,0);
  buffer.PushCoord(5,1);
  buffer.PushCoord(0,0);

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
