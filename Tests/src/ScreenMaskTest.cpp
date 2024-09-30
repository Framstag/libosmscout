
#include <osmscoutmap/LabelLayouterHelper.h>

#include <catch2/catch_test_macros.hpp>

using namespace osmscout;

TEST_CASE("Simple ScreenRectMask")
{
  size_t screenWidth=100;

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(10,10,10,10));

  REQUIRE(screenRectMask1.GetFirstRow()==10);
  REQUIRE(screenRectMask1.GetLastRow()==19);

  REQUIRE(screenRectMask1.GetFirstCell()==0);
  REQUIRE(screenRectMask1.GetLastCell()==0);

  // 10x 0 bit + 10x 1 bit
  REQUIRE(screenRectMask1.GetCell(0)==0xffc00);
}

TEST_CASE("ScreenRectMask before CellBorder")
{
  size_t screenWidth=100;

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(53,10,10,10));

  REQUIRE(screenRectMask1.GetFirstRow()==10);
  REQUIRE(screenRectMask1.GetLastRow()==19);

  REQUIRE(screenRectMask1.GetFirstCell()==0);
  REQUIRE(screenRectMask1.GetLastCell()==0);
}

TEST_CASE("ScreenRectMask after CellBorder")
{
  size_t screenWidth=200;

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(64,10,10,10));

  REQUIRE(screenRectMask1.GetFirstRow()==10);
  REQUIRE(screenRectMask1.GetLastRow()==19);

  REQUIRE(screenRectMask1.GetFirstCell()==1);
  REQUIRE(screenRectMask1.GetLastCell()==1);
}

TEST_CASE("ScreenRectMask over CellBorder")
{
  size_t screenWidth=200;

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(32,10,64,10));

  REQUIRE(screenRectMask1.GetFirstRow()==10);
  REQUIRE(screenRectMask1.GetLastRow()==19);

  REQUIRE(screenRectMask1.GetFirstCell()==0);
  REQUIRE(screenRectMask1.GetLastCell()==1);

  // 32x 0 bit + 32x 1 bit
  REQUIRE(screenRectMask1.GetCell(0)==0xffffffff00000000);
  // 32x 1 bit + 32x 0 bit
  REQUIRE(screenRectMask1.GetCell(1)==0xffffffff);
}

TEST_CASE("ScreenRectMask over 3 cells")
{
  size_t screenWidth=200;

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(32,10,128,10));

  REQUIRE(screenRectMask1.GetFirstRow()==10);
  REQUIRE(screenRectMask1.GetLastRow()==19);

  REQUIRE(screenRectMask1.GetFirstCell()==0);
  REQUIRE(screenRectMask1.GetLastCell()==2);

  // 32x 0 bit + 32x 1 bit
  REQUIRE(screenRectMask1.GetCell(0)==0xffffffff00000000);
  // 34x 1 bit
  REQUIRE(screenRectMask1.GetCell(1)==0xffffffffffffffff);
  // 32x 1 bit + 32x 0 bit
  REQUIRE(screenRectMask1.GetCell(2)==0xffffffff);
}

TEST_CASE("ScreenRectMask clipped left")
{
  size_t screenWidth=200;

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(-5,10,10,10));

  REQUIRE(screenRectMask1.GetFirstRow()==10);
  REQUIRE(screenRectMask1.GetLastRow()==19);

  REQUIRE(screenRectMask1.GetFirstCell()==0);
  REQUIRE(screenRectMask1.GetLastCell()==0);

  // 5x 1 bit
  REQUIRE(screenRectMask1.GetCell(0)==0x1f);
}

TEST_CASE("ScreenRectMask clipped right")
{
  size_t screenWidth=10;

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(5,10,10,10));

  REQUIRE(screenRectMask1.GetFirstRow()==10);
  REQUIRE(screenRectMask1.GetLastRow()==19);

  REQUIRE(screenRectMask1.GetFirstCell()==0);
  REQUIRE(screenRectMask1.GetLastCell()==0);

  // 5x 0 bit, 5x 1 bit
  REQUIRE(screenRectMask1.GetCell(0)==0x3e0);
}

TEST_CASE("Left and right without gap")
{
  size_t screenWidth=100;
  size_t screenHeight=100;

  ScreenMask screenMask(screenWidth,screenHeight);

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(10,10,10,10));
  ScreenRectMask screenRectMask2(screenWidth,ScreenPixelRectangle(20,10,10,10));

  screenMask.AddMask(screenRectMask1);

  REQUIRE_FALSE(screenMask.HasCollision(screenRectMask2));
}

TEST_CASE("Left and right with gap")
{
  size_t screenWidth=100;
  size_t screenHeight=100;

  ScreenMask screenMask(screenWidth,screenHeight);

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(10,10,9,10));
  ScreenRectMask screenRectMask2(screenWidth,ScreenPixelRectangle(20,10,9,10));

  screenMask.AddMask(screenRectMask1);

  REQUIRE_FALSE(screenMask.HasCollision(screenRectMask2));
}

TEST_CASE("Above and beneath without gap")
{
  size_t screenWidth=100;
  size_t screenHeight=100;

  ScreenMask screenMask(screenWidth,screenHeight);

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(10,10,10,10));

  REQUIRE(screenRectMask1.GetFirstRow()==10);
  REQUIRE(screenRectMask1.GetLastRow()==19);

  ScreenRectMask screenRectMask2(screenWidth,ScreenPixelRectangle(10,20,10,10));

  REQUIRE(screenRectMask2.GetFirstRow()==20);
  REQUIRE(screenRectMask2.GetLastRow()==29);

  screenMask.AddMask(screenRectMask1);

  REQUIRE_FALSE(screenMask.HasCollision(screenRectMask2));
}

TEST_CASE("Above and beneath with gap")
{
  size_t screenWidth=100;
  size_t screenHeight=100;

  ScreenMask screenMask(screenWidth,screenHeight);

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(10,10,10,9));

  REQUIRE(screenRectMask1.GetFirstRow()==10);
  REQUIRE(screenRectMask1.GetLastRow()==18);

  ScreenRectMask screenRectMask2(screenWidth,ScreenPixelRectangle(10,20,10,9));

  REQUIRE(screenRectMask2.GetFirstRow()==20);
  REQUIRE(screenRectMask2.GetLastRow()==28);

  screenMask.AddMask(screenRectMask1);

  REQUIRE_FALSE(screenMask.HasCollision(screenRectMask2));
}

TEST_CASE("1 pixel overlap horizontally")
{
  size_t screenWidth=100;
  size_t screenHeight=100;

  ScreenMask screenMask(screenWidth,screenHeight);

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(10,10,10,10));
  ScreenRectMask screenRectMask2(screenWidth,ScreenPixelRectangle(19,10,10,10));

  screenMask.AddMask(screenRectMask1);

  REQUIRE(screenMask.HasCollision(screenRectMask2));
}

TEST_CASE("1 pixel overlap vertically")
{
  size_t screenWidth=100;
  size_t screenHeight=100;

  ScreenMask screenMask(screenWidth,screenHeight);

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(10,10,10,10));
  ScreenRectMask screenRectMask2(screenWidth,ScreenPixelRectangle(10,19,10,10));

  screenMask.AddMask(screenRectMask1);

  REQUIRE(screenMask.HasCollision(screenRectMask2));
}

TEST_CASE("1 pixel overlap on a corner")
{
  size_t screenWidth=100;
  size_t screenHeight=100;

  ScreenMask screenMask(screenWidth,screenHeight);

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(10,10,10,10));
  ScreenRectMask screenRectMask2(screenWidth,ScreenPixelRectangle(19,19,10,10));

  screenMask.AddMask(screenRectMask1);

  REQUIRE(screenMask.HasCollision(screenRectMask2));
}

TEST_CASE("Horizontal and vertical overlap")
{
  size_t screenWidth=100;
  size_t screenHeight=100;

  ScreenMask screenMask(screenWidth,screenHeight);

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(10,10,10,10));
  ScreenRectMask screenRectMask2(screenWidth,ScreenPixelRectangle(5,5,10,10));

  screenMask.AddMask(screenRectMask1);

  REQUIRE(screenMask.HasCollision(screenRectMask2));
}

TEST_CASE("Real world test 1")
{
  size_t screenWidth=3212;
  size_t screenHeight=2039;

  ScreenMask screenMask(screenWidth,screenHeight);

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(1113,1117,21,21)); // 1131,1137
  ScreenRectMask screenRectMask2(screenWidth,ScreenPixelRectangle(1098,1127,50,23)); // 1147,1149
  ScreenRectMask screenRectMask3(screenWidth,ScreenPixelRectangle(1114,1139,18,20)); // 1131,1158
  ScreenRectMask screenRectMask4(screenWidth,ScreenPixelRectangle(1088,1107,16,16)); // 1103,1122

  screenMask.AddMask(screenRectMask1);
  screenMask.AddMask(screenRectMask2);
  screenMask.AddMask(screenRectMask3);
  screenMask.AddMask(screenRectMask4);

  ScreenRectMask screenRectMask5(screenWidth,ScreenPixelRectangle(1054,1113,84,22)); // 1137,1134

  REQUIRE(screenMask.HasCollision(screenRectMask5)); // => mask1, mask2, mask4

  ScreenRectMask screenRectMask6(screenWidth,ScreenPixelRectangle(1036,1095,65,33)); // 1100,1127

  REQUIRE(screenMask.HasCollision(screenRectMask6)); // => mask2, mask4
}

TEST_CASE("Real world test 2")
{
  size_t screenWidth=3212;
  size_t screenHeight=2039;

  ScreenMask screenMask(screenWidth,screenHeight);

  ScreenRectMask screenRectMask1(screenWidth,ScreenPixelRectangle(1113,1072,21,21)); // 1133,1092 // +45
  ScreenRectMask screenRectMask2(screenWidth,ScreenPixelRectangle(1098,1082,50,23)); // 1147,1104 // +45
  ScreenRectMask screenRectMask3(screenWidth,ScreenPixelRectangle(1114,1094,18,20)); // 1131,1113 // +45
  ScreenRectMask screenRectMask4(screenWidth,ScreenPixelRectangle(1088,1061,16,16)); // 1103,1076 // +46

  screenMask.AddMask(screenRectMask1);
  screenMask.AddMask(screenRectMask2);
  screenMask.AddMask(screenRectMask3);
  screenMask.AddMask(screenRectMask4);

  ScreenRectMask screenRectMask5(screenWidth,ScreenPixelRectangle(1054,1067,84,22)); // 1137,1088 // +46

  REQUIRE(screenMask.HasCollision(screenRectMask5)); // => mask1,mask2, mask4

  ScreenRectMask screenRectMask6(screenWidth,ScreenPixelRectangle(1036,1049,65,33)); // 1100,1081 // +45

  REQUIRE(screenMask.HasCollision(screenRectMask6)); // => mask1,mask2, mask4
}
