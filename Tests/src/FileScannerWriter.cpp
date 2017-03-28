#include <algorithm>
#include <iostream>
#include <limits>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

#include <osmscout/CoreFeatures.h>

int errors=0;

void DumpGeoCoords(const std::vector<osmscout::Point>& coords)
{
  std::cout << "[";
  for (size_t i=0; i<coords.size(); i++) {
    if (i>0) {
      std::cout << ", ";
    }

    std::cout << coords[i].GetCoord().GetDisplayText();
  }
  std::cout << "]";
}

bool Equals(const std::vector<osmscout::Point>& coordsA, const std::vector<osmscout::Point>& coordsB)
{
  if (coordsA.size()!=coordsB.size()) {
    return false;
  }

  for (size_t i=0; i<coordsA.size(); i++) {
    if (coordsA[i].GetCoord().GetDisplayText()!=coordsB[i].GetCoord().GetDisplayText()) {
      std::cerr << "Difference at offset " << i << " " << coordsA[i].GetCoord().GetDisplayText() << " <=> " << coordsB[i].GetCoord().GetDisplayText() << std::endl;
      return false;
    }
  }

  return true;
}

int main()
{
  osmscout::FileWriter  writer;
  osmscout::FileScanner scanner;

  bool                  outBool1=false;
  bool                  outBool2=true;

  uint16_t              out16u1=std::numeric_limits<uint16_t>::min();
  uint16_t              out16u2=std::numeric_limits<uint16_t>::max()/2;
  uint16_t              out16u3=std::numeric_limits<uint16_t>::max();

  uint32_t              out32u1=std::numeric_limits<uint32_t>::min();
  uint32_t              out32u2=std::numeric_limits<uint32_t>::max()/2;
  uint32_t              out32u3=std::numeric_limits<uint32_t>::max();

  uint64_t              out64u1=std::numeric_limits<uint64_t>::min();
  uint64_t              out64u2=std::numeric_limits<uint64_t>::max()/2;
  uint64_t              out64u3=std::numeric_limits<uint64_t>::max();

  osmscout::FileOffset  outfo1=std::numeric_limits<osmscout::FileOffset>::min();
  osmscout::FileOffset  outfo2=std::numeric_limits<osmscout::FileOffset>::max()/2;
  osmscout::FileOffset  outfo3=std::numeric_limits<osmscout::FileOffset>::max();

  osmscout::GeoCoord    outCoord1(51.57231,7.46418);

  std::vector<osmscout::Point> outCoords1;
  std::vector<osmscout::Point> outCoords2;
  std::vector<osmscout::Point> outCoords3;
  std::vector<osmscout::Point> outCoords4;
  std::vector<osmscout::Point> outCoords5;
  std::vector<osmscout::Point> outCoords6;
  std::vector<osmscout::Point> outCoords7;


  osmscout::FileOffset  finalWriteFileOffset;

  bool                  inBool;
  uint16_t              in16u;
  uint32_t              in32u;
  uint64_t              in64u;

  osmscout::FileOffset  info;

  osmscout::GeoCoord    inCoord1;

  std::vector<osmscout::Point> inCoords1;
  std::vector<osmscout::Point> inCoords2;
  std::vector<osmscout::Point> inCoords3;
  std::vector<osmscout::Point> inCoords4;
  std::vector<osmscout::Point> inCoords5;
  std::vector<osmscout::Point> inCoords6;
  std::vector<osmscout::Point> inCoords7;

  osmscout::FileOffset  finalReadFileOffset;

  outCoords1.push_back(osmscout::Point(0,osmscout::GeoCoord(51.57231,7.46418)));
  outCoords1.push_back(osmscout::Point(0,osmscout::GeoCoord(51.57233,7.46430)));
  outCoords1.push_back(osmscout::Point(0,osmscout::GeoCoord(51.57261,7.46563)));
  outCoords1.push_back(osmscout::Point(0,osmscout::GeoCoord(51.57269,7.46594)));

  outCoords2=outCoords1;
  std::reverse(outCoords2.begin(),outCoords2.end());

  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58549,7.55493)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58549,7.55494)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58550,7.55496)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58547,7.55504)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58544,7.55506)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58544,7.55507)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58543,7.55508)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58542,7.55508)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58540,7.55508)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58539,7.55508)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58538,7.55500)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58538,7.55498)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58539,7.55495)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58540,7.55494)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58540,7.55488)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58541,7.55484)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58542,7.55484)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58544,7.55483)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58546,7.55484)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58547,7.55485)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58548,7.55488)));
  outCoords3.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58549,7.55492)));

  outCoords4=outCoords3;
  std::reverse(outCoords4.begin(),outCoords4.end());

  outCoords5.push_back(osmscout::Point(0,osmscout::GeoCoord(5.0,-5.0)));
  outCoords5.push_back(osmscout::Point(0,osmscout::GeoCoord(5.0,5.0)));
  outCoords5.push_back(osmscout::Point(0,osmscout::GeoCoord(-5.0,5.0)));

  outCoords6=outCoords5;
  std::reverse(outCoords6.begin(),outCoords6.end());

  uint64_t maxCoords=1000000;

  for (uint64_t i=1; i<=maxCoords; i++) {
    outCoords7.push_back(osmscout::Point(0,osmscout::GeoCoord(51.58549,7.55493)));
  }

  try {
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

    writer.WriteNumber(out16u1);
    writer.WriteNumber(out16u2);
    writer.WriteNumber(out16u3);

    writer.WriteNumber(out32u1);
    writer.WriteNumber(out32u2);
    writer.WriteNumber(out32u3);

    writer.WriteNumber(out64u1);
    writer.WriteNumber(out64u2);
    writer.WriteNumber(out64u3);

    writer.WriteFileOffset(outfo1);
    writer.WriteFileOffset(outfo2);
    writer.WriteFileOffset(outfo3);

    writer.WriteCoord(outCoord1);

    writer.Write(outCoords1,false);
    writer.Write(outCoords2,false);
    writer.Write(outCoords3,false);
    writer.Write(outCoords4,false);
    writer.Write(outCoords5,false);
    writer.Write(outCoords6,false);
    writer.Write(outCoords7,false);

    finalWriteFileOffset=writer.GetPos();

    writer.Close();

    scanner.Open("test.dat",osmscout::FileScanner::Normal,false);

    // Read/Write

    scanner.Read(inBool);
    if (inBool!=outBool1) {
      std::cerr << "Read/Write(bool): Expected " << outBool1 << ", got " << inBool << std::endl;
      errors++;
    }

    scanner.Read(inBool);
    if (inBool!=outBool2) {
      std::cerr << "Read/Write(bool): Expected " << outBool2 << ", got " << inBool << std::endl;
      errors++;
    }

    scanner.Read(in16u);
    if (in16u!=out16u1) {
      std::cerr << "Read/Write(uint16_t): Expected " << out16u1 << ", got " << in16u << std::endl;
      errors++;
    }

    scanner.Read(in16u);
    if (in16u!=out16u2) {
      std::cerr << "Read/Write(uint16_t): Expected " << out16u2 << ", got " << in16u << std::endl;
      errors++;
    }

    scanner.Read(in16u);
    if (in16u!=out16u3) {
      std::cerr << "Read/Write(uint16_t): Expected " << out16u3 << ", got " << in16u << std::endl;
      errors++;
    }

    scanner.Read(in32u);
    if (in32u!=out32u1) {
      std::cerr << "Read/Write(uint32_t): Expected " << out32u1 << ", got " << in32u << std::endl;
      errors++;
    }

    scanner.Read(in32u);
    if (in32u!=out32u2) {
      std::cerr << "Read/Write(uint32_t): Expected " << out32u2 << ", got " << in32u << std::endl;
      errors++;
    }

    scanner.Read(in32u);
    if (in32u!=out32u3) {
      std::cerr << "Read/Write(uint32_t): Expected " << out32u3 << ", got " << in32u << std::endl;
      errors++;
    }

    scanner.Read(in64u);
    if (in64u!=out64u1) {
      std::cerr << "Read/Write(uint64_t): Expected " << out64u1 << ", got " << in64u << std::endl;
      errors++;
    }

    scanner.Read(in64u);
    if (in64u!=out64u2) {
      std::cerr << "Read/Write(uint64_t): Expected " << out64u2 << ", got " << in64u << std::endl;
      errors++;
    }

    scanner.Read(in64u);
    if (in64u!=out64u3) {
      std::cerr << "Read/Write(uint64_t): Expected " << out64u3 << ", got " << in64u << std::endl;
      errors++;
    }

    // Read/WriteNumber

    scanner.ReadNumber(in16u);
    if (in16u!=out16u1) {
      std::cerr << "Read/WriteNumber(uint16_t): Expected " << out16u1 << ", got " << in16u << std::endl;
      errors++;
    }

    scanner.ReadNumber(in16u);
    if (in16u!=out16u2) {
      std::cerr << "Read/WriteNumber(uint16_t): Expected " << out16u2 << ", got " << in16u << std::endl;
      errors++;
    }

    scanner.ReadNumber(in16u);
    if (in16u!=out16u3) {
      std::cerr << "Read/WriteNumber(uint16_t): Expected " << out16u3 << ", got " << in16u << std::endl;
      errors++;
    }

    scanner.ReadNumber(in32u);
    if (in32u!=out32u1) {
      std::cerr << "Read/WriteNumber(uint32_t): Expected " << out32u1 << ", got " << in32u << std::endl;
      errors++;
    }

    scanner.ReadNumber(in32u);
    if (in32u!=out32u2) {
      std::cerr << "Read/WriteNumber(uint32_t): Expected " << out32u2 << ", got " << in32u << std::endl;
      errors++;
    }

    scanner.ReadNumber(in32u);
    if (in32u!=out32u3) {
      std::cerr << "Read/WriteNumber(uint32_t): Expected " << out32u3 << ", got " << in32u << std::endl;
      errors++;
    }

    scanner.ReadNumber(in64u);
    if (in64u!=out64u1) {
      std::cerr << "Read/WriteNumber(uint64_t): Expected " << out64u1 << ", got " << in64u << std::endl;
      errors++;
    }

    scanner.ReadNumber(in64u);
    if (in64u!=out64u2) {
      std::cerr << "Read/WriteNumber(uint64_t): Expected " << out64u2 << ", got " << in64u << std::endl;
      errors++;
    }

    scanner.ReadNumber(in64u);
    if (in64u!=out64u3) {
      std::cerr << "Read/WriteNumber(uint64_t): Expected " << out64u3 << ", got " << in64u << std::endl;
      errors++;
    }

    // Read/WriteFileOffset

    scanner.ReadFileOffset(info);
    if (info!=outfo1) {
      std::cerr << "Read/WriteFileOffset(FileOffset): Expected " << outfo1 << ", got " << info << std::endl;
      errors++;
    }

    scanner.ReadFileOffset(info);
    if (info!=outfo2) {
      std::cerr << "Read/WriteFileOffset(FileOffset): Expected " << outfo2 << ", got " << info << std::endl;
      errors++;
    }

    scanner.ReadFileOffset(info);
    if (info!=outfo3) {
      std::cerr << "Read/WriteFileOffset(FileOffset): Expected " << outfo3 << ", got " << info << std::endl;
      errors++;
    }

    scanner.ReadCoord(inCoord1);

    if (inCoord1.GetDisplayText()!=outCoord1.GetDisplayText()) {
      std::cerr << "Read/WriteCoord(GeoCoord) 1: Expected " << outCoord1.GetDisplayText() << ", got " << inCoord1.GetDisplayText() << std::endl;
      errors++;
    }

    scanner.Read(inCoords1,false);
    if (!Equals(inCoords1,outCoords1)) {
      std::cerr << "Read/Write(std::vector<GeoCoord>) 1: Expected ";

      DumpGeoCoords(outCoords1);

      std::cout << ", got ";

      DumpGeoCoords(inCoords1);

      std::cout << std::endl;
      errors++;
    }

    scanner.Read(inCoords2,false);
    if (!Equals(inCoords2,outCoords2)) {
      std::cerr << "Read/Write(std::vector<GeoCoord>) 2: Expected ";

      DumpGeoCoords(outCoords2);

      std::cout << ", got ";

      DumpGeoCoords(inCoords2);

      std::cout << std::endl;
      errors++;
    }

    scanner.Read(inCoords3,false);
    if (!Equals(inCoords3,outCoords3)) {
      std::cerr << "Read/Write(std::vector<GeoCoord>) 3: Expected ";

      DumpGeoCoords(outCoords3);

      std::cout << ", got ";

      DumpGeoCoords(inCoords3);

      std::cout << std::endl;
      errors++;
    }

    scanner.Read(inCoords4,false);
    if (!Equals(inCoords4,outCoords4)) {
      std::cerr << "Read/Write(std::vector<GeoCoord>) 4: Expected ";

      DumpGeoCoords(outCoords4);

      std::cout << ", got ";

      DumpGeoCoords(inCoords4);

      std::cout << std::endl;
      errors++;
    }

    scanner.Read(inCoords5,false);
    if (!Equals(inCoords5,outCoords5)) {
      std::cerr << "Read/Write(std::vector<GeoCoord>) 5: Expected ";

      DumpGeoCoords(outCoords5);

      std::cout << ", got ";

      DumpGeoCoords(inCoords5);

      std::cout << std::endl;
      errors++;
    }

    scanner.Read(inCoords6,false);
    if (!Equals(inCoords6,outCoords6)) {
      std::cerr << "Read/Write(std::vector<GeoCoord>) 6: Expected ";

      DumpGeoCoords(outCoords6);

      std::cout << ", got ";

      DumpGeoCoords(inCoords6);

      std::cout << std::endl;
      errors++;
    }

    scanner.Read(inCoords7,false);
    if (!Equals(inCoords7,outCoords7)) {
      std::cerr << "Read/Write(std::vector<GeoCoord>) 7: Expected ";

      std::cout << outCoords7.size();

      std::cout << ", got ";

      std::cout << inCoords7.size();

      std::cout << std::endl;
      errors++;
    }

    finalReadFileOffset=scanner.GetPos();

    if (finalWriteFileOffset!=finalReadFileOffset) {
      std::cerr << "Final file offset check: Expected ";

      std::cout << finalWriteFileOffset;

      std::cout << ", got ";

      std::cout << finalReadFileOffset;

      std::cout << std::endl;
      errors++;
    }
  }
  catch (osmscout::IOException& e) {
    std::cerr << e.GetDescription() << std::endl;
    errors++;
  }

  if (errors!=0) {
    return 1;
  }
  else {
    return 0;
  }
}
