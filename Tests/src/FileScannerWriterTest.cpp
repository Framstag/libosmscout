#include <algorithm>
#include <iostream>
#include <limits>

#include <osmscout/io/FileScanner.h>
#include <osmscout/io/FileWriter.h>

#include <catch2/catch_test_macros.hpp>

bool Equals(const std::vector<osmscout::Point>& coordsA, const std::vector<osmscout::Point>& coordsB)
{
    if (coordsA.size() != coordsB.size()) {
        return false;
    }

    for (size_t i = 0; i < coordsA.size(); i++) {
        if (coordsA[i].GetCoord().GetDisplayText() != coordsB[i].GetCoord().GetDisplayText()) {
            std::cerr << "Difference at offset " << i << " " << coordsA[i].GetCoord().GetDisplayText() << " <=> " << coordsB[i].GetCoord().GetDisplayText() << std::endl;
            return false;
        }
    }

    return true;
}

TEST_CASE("FileScannerWriter")
{
  osmscout::FileWriter  writer;
  osmscout::FileScanner scanner;

  osmscout::FileOffset  finalWriteFileOffset;

  osmscout::FileOffset  info;

  osmscout::FileOffset  finalReadFileOffset;

  bool                  inBool;
  uint16_t              in16u;
  uint32_t              in32u;
  uint64_t              in64u;

  int16_t              in16s;
  int32_t              in32s;
  int64_t              in64s;

  osmscout::GeoCoord    inCoord1;

  std::vector<osmscout::Point> inCoords1;
  std::vector<osmscout::Point> inCoords2;
  std::vector<osmscout::Point> inCoords3;
  std::vector<osmscout::Point> inCoords4;
  std::vector<osmscout::Point> inCoords5;
  std::vector<osmscout::Point> inCoords6;
  std::vector<osmscout::Point> inCoords7;

  bool                  outBool1 = false;
  bool                  outBool2 = true;

  uint16_t              out16u1 = std::numeric_limits<uint16_t>::min();
  uint16_t              out16u2 = std::numeric_limits<uint16_t>::max() / 2;
  uint16_t              out16u3 = std::numeric_limits<uint16_t>::max();

  uint32_t              out32u1 = std::numeric_limits<uint32_t>::min();
  uint32_t              out32u2 = std::numeric_limits<uint32_t>::max() / 2;
  uint32_t              out32u3 = std::numeric_limits<uint32_t>::max();

  uint64_t              out64u1 = std::numeric_limits<uint64_t>::min();
  uint64_t              out64u2 = std::numeric_limits<uint64_t>::max() / 2;
  uint64_t              out64u3 = std::numeric_limits<uint64_t>::max();

  int16_t               out16s1 = std::numeric_limits<int16_t>::min();
  int16_t               out16s2 = std::numeric_limits<int16_t>::max() / 2;
  int16_t               out16s3 = std::numeric_limits<int16_t>::max();

  int32_t               out32s1 = std::numeric_limits<int32_t>::min();
  int32_t               out32s2 = std::numeric_limits<int32_t>::max() / 2;
  int32_t               out32s3 = std::numeric_limits<int32_t>::max();

  int64_t               out64s1 = std::numeric_limits<int64_t>::min();
  int64_t               out64s2 = std::numeric_limits<int64_t>::max() / 2;
  int64_t               out64s3 = std::numeric_limits<int64_t>::max();

  osmscout::FileOffset  outfo1 = std::numeric_limits<osmscout::FileOffset>::min();
  osmscout::FileOffset  outfo2 = std::numeric_limits<osmscout::FileOffset>::max() / 2;
  osmscout::FileOffset  outfo3 = std::numeric_limits<osmscout::FileOffset>::max();

  osmscout::GeoCoord    outCoord1(51.57231, 7.46418);

  std::vector<osmscout::Point> outCoords1;
  outCoords1.reserve(4);
  std::vector<osmscout::Point> outCoords2;
  std::vector<osmscout::Point> outCoords3;
  outCoords3.reserve(22);
  std::vector<osmscout::Point> outCoords4;
  std::vector<osmscout::Point> outCoords5;
  outCoords5.reserve(3);
  std::vector<osmscout::Point> outCoords6;
  std::vector<osmscout::Point> outCoords7;

  outCoords1.emplace_back(0, osmscout::GeoCoord(51.57231, 7.46418));
  outCoords1.emplace_back(0, osmscout::GeoCoord(51.57233, 7.46430));
  outCoords1.emplace_back(0, osmscout::GeoCoord(51.57261, 7.46563));
  outCoords1.emplace_back(0, osmscout::GeoCoord(51.57269, 7.46594));

  outCoords2 = outCoords1;
  std::reverse(outCoords2.begin(), outCoords2.end());

  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58549, 7.55493));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58549, 7.55494));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58550, 7.55496));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58547, 7.55504));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58544, 7.55506));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58544, 7.55507));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58543, 7.55508));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58542, 7.55508));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58540, 7.55508));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58539, 7.55508));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58538, 7.55500));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58538, 7.55498));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58539, 7.55495));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58540, 7.55494));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58540, 7.55488));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58541, 7.55484));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58542, 7.55484));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58544, 7.55483));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58546, 7.55484));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58547, 7.55485));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58548, 7.55488));
  outCoords3.emplace_back(0, osmscout::GeoCoord(51.58549, 7.55492));

  outCoords4 = outCoords3;
  std::reverse(outCoords4.begin(), outCoords4.end());

  outCoords5.emplace_back(0, osmscout::GeoCoord(5.0, -5.0));
  outCoords5.emplace_back(0, osmscout::GeoCoord(5.0, 5.0));
  outCoords5.emplace_back(0, osmscout::GeoCoord(-5.0, 5.0));

  outCoords6 = outCoords5;
  std::reverse(outCoords6.begin(), outCoords6.end());

  uint64_t maxCoords = 1000000;

  for (uint64_t i = 1; i <= maxCoords; i++) {
      outCoords7.emplace_back(0, osmscout::GeoCoord(51.58549, 7.55493));
  }

  writer.Open("test.dat");
  writer.Write(outBool1);
  writer.Write(outBool2);

  writer.Write(out16u1);
  writer.Write(out16u2);
  writer.Write(out16u3);

  writer.Write(out32u1);
  writer.Write(out32u2);
  writer.Write(out32u3);

  writer.Write(out64u1);
  writer.Write(out64u2);
  writer.Write(out64u3);

  writer.Write(out16s1);
  writer.Write(out16s2);
  writer.Write(out16s3);

  writer.Write(out32s1);
  writer.Write(out32s2);
  writer.Write(out32s3);

  writer.Write(out64s1);
  writer.Write(out64s2);
  writer.Write(out64s3);

  writer.WriteNumber(out16u1);
  writer.WriteNumber(out16u2);
  writer.WriteNumber(out16u3);

  writer.WriteNumber(out32u1);
  writer.WriteNumber(out32u2);
  writer.WriteNumber(out32u3);

  writer.WriteNumber(out64u1);
  writer.WriteNumber(out64u2);
  writer.WriteNumber(out64u3);

  writer.WriteNumber(out16s1);
  writer.WriteNumber(out16s2);
  writer.WriteNumber(out16s3);

  writer.WriteNumber(out32s1);
  writer.WriteNumber(out32s2);
  writer.WriteNumber(out32s3);

  writer.WriteNumber(out64s1);
  writer.WriteNumber(out64s2);
  writer.WriteNumber(out64s3);

  writer.WriteFileOffset(outfo1);
  writer.WriteFileOffset(outfo2);
  writer.WriteFileOffset(outfo3);

  writer.WriteCoord(outCoord1);

  writer.Write(outCoords1, false);
  writer.Write(outCoords2, false);
  writer.Write(outCoords3, false);
  writer.Write(outCoords4, false);
  writer.Write(outCoords5, false);
  writer.Write(outCoords6, false);
  writer.Write(outCoords7, false);

  finalWriteFileOffset = writer.GetPos();

  writer.Close();

  for (int mmapMode = 0; mmapMode <= 1; mmapMode++)
  {
    scanner.Open("test.dat", osmscout::FileScanner::Normal, (bool)mmapMode);

    // Read/Write

    inBool=scanner.ReadBool();
    REQUIRE(inBool == outBool1);

    inBool=scanner.ReadBool();
    REQUIRE(inBool == outBool2);

    in16u=scanner.ReadUInt16();
    REQUIRE(in16u == out16u1);

    in16u=scanner.ReadUInt16();
    REQUIRE(in16u == out16u2);

    in16u=scanner.ReadUInt16();
    REQUIRE(in16u == out16u3);

    in32u=scanner.ReadUInt32();
    REQUIRE(in32u == out32u1);

    in32u=scanner.ReadUInt32();
    REQUIRE(in32u == out32u2);

    in32u=scanner.ReadUInt32();
    REQUIRE(in32u == out32u3);

    in64u=scanner.ReadUInt64();
    REQUIRE(in64u == out64u1);

    in64u=scanner.ReadUInt64();
    REQUIRE(in64u == out64u2);

    in64u=scanner.ReadUInt64();
    REQUIRE(in64u == out64u3);

    in16s=scanner.ReadInt16();
    REQUIRE(in16s == out16s1);

    in16s=scanner.ReadInt16();
    REQUIRE(in16s == out16s2);

    in16s=scanner.ReadInt16();
    REQUIRE(in16s == out16s3);

    in32s=scanner.ReadInt32();
    REQUIRE(in32s == out32s1);

    in32s=scanner.ReadInt32();
    REQUIRE(in32s == out32s2);

    in32s=scanner.ReadInt32();
    REQUIRE(in32s == out32s3);

    in64s=scanner.ReadInt64();
    REQUIRE(in64s == out64s1);

    in64s=scanner.ReadInt64();
    REQUIRE(in64s == out64s2);

    in64s=scanner.ReadInt64();
    REQUIRE(in64s == out64s3);

    // Read/WriteNumber

    in16u=scanner.ReadUInt16Number();
    REQUIRE(in16u == out16u1);

    in16u=scanner.ReadUInt16Number();
    REQUIRE(in16u == out16u2);

    in16u=scanner.ReadUInt16Number();
    REQUIRE(in16u == out16u3);

    in32u=scanner.ReadUInt32Number();
    REQUIRE(in32u == out32u1);

    in32u=scanner.ReadUInt32Number();
    REQUIRE(in32u == out32u2);

    in32u=scanner.ReadUInt32Number();
    REQUIRE(in32u == out32u3);

    in64u=scanner.ReadUInt64Number();
    REQUIRE(in64u == out64u1);

    in64u=scanner.ReadUInt64Number();
    REQUIRE(in64u == out64u2);

    in64u=scanner.ReadUInt64Number();
    REQUIRE(in64u == out64u3);

    in16s=scanner.ReadInt16Number();
    REQUIRE(in16s == out16s1);

    in16s=scanner.ReadInt16Number();
    REQUIRE(in16s == out16s2);

    in16s=scanner.ReadInt16Number();
    REQUIRE(in16s == out16s3);

    in32s=scanner.ReadInt32Number();
    REQUIRE(in32s == out32s1);

    in32s=scanner.ReadInt32Number();
    REQUIRE(in32s == out32s2);

    in32s=scanner.ReadInt32Number();
    REQUIRE(in32s == out32s3);

    in64s=scanner.ReadInt64Number();
    REQUIRE(in64s == out64s1);

    in64s=scanner.ReadInt64Number();
    REQUIRE(in64s == out64s2);

    in64s=scanner.ReadInt64Number();
    REQUIRE(in64s == out64s3);

    // Read/WriteFileOffset

    info=scanner.ReadFileOffset();
    REQUIRE(info == outfo1);

    info=scanner.ReadFileOffset();
    REQUIRE(info == outfo2);

    info=scanner.ReadFileOffset();
    REQUIRE(info == outfo3);

    inCoord1=scanner.ReadCoord();
    REQUIRE(inCoord1.GetDisplayText() == outCoord1.GetDisplayText());

    osmscout::GeoBox boundingBox;
    std::vector<osmscout::SegmentGeoBox> segments;

    scanner.Read(inCoords1, segments, boundingBox, false);
    REQUIRE(Equals(inCoords1, outCoords1));

    scanner.Read(inCoords2, segments, boundingBox, false);
    REQUIRE(Equals(inCoords2, outCoords2));

    scanner.Read(inCoords3, segments, boundingBox, false);
    REQUIRE(Equals(inCoords3, outCoords3));

    scanner.Read(inCoords4, segments, boundingBox, false);
    REQUIRE(Equals(inCoords4, outCoords4));

    scanner.Read(inCoords5, segments, boundingBox, false);
    REQUIRE(Equals(inCoords5, outCoords5));

    scanner.Read(inCoords6, segments, boundingBox, false);
    REQUIRE(Equals(inCoords6, outCoords6));

    scanner.Read(inCoords7, segments, boundingBox, false);
    REQUIRE(Equals(inCoords7, outCoords7));

    finalReadFileOffset = scanner.GetPos();
    REQUIRE(finalWriteFileOffset == finalReadFileOffset);

    scanner.Close();
  }
}
