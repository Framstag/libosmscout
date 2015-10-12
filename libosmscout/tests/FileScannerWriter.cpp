#include <algorithm>
#include <iostream>
#include <limits>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

#include <osmscout/CoreFeatures.h>

int errors=0;

void DumpGeoCoords(const std::vector<osmscout::GeoCoord>& coords)
{
  std::cout << "[";
  for (size_t i=0; i<coords.size(); i++) {
    if (i>0) {
      std::cout << ", ";
    }

    std::cout << coords[i].GetDisplayText();
  }
  std::cout << "]";
}

bool Equals(const std::vector<osmscout::GeoCoord>& coordsA, const std::vector<osmscout::GeoCoord>& coordsB)
{
  if (coordsA.size()!=coordsB.size()) {
    return false;
  }

  for (size_t i=0; i<coordsA.size(); i++) {
    if (coordsA[i].GetDisplayText()!=coordsB[i].GetDisplayText()) {
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

  std::vector<osmscout::GeoCoord> outCoords1;
  std::vector<osmscout::GeoCoord> outCoords2;
  std::vector<osmscout::GeoCoord> outCoords3;
  std::vector<osmscout::GeoCoord> outCoords4;
  std::vector<osmscout::GeoCoord> outCoords5;
  std::vector<osmscout::GeoCoord> outCoords6;

  bool                  inBool;
  uint16_t              in16u;
  uint32_t              in32u;
  uint64_t              in64u;

  osmscout::FileOffset  info;

  std::vector<osmscout::GeoCoord> inCoords1;
  std::vector<osmscout::GeoCoord> inCoords2;
  std::vector<osmscout::GeoCoord> inCoords3;
  std::vector<osmscout::GeoCoord> inCoords4;
  std::vector<osmscout::GeoCoord> inCoords5;
  std::vector<osmscout::GeoCoord> inCoords6;

  outCoords1.push_back(osmscout::GeoCoord(51.57231,7.46418));
  outCoords1.push_back(osmscout::GeoCoord(51.57233,7.46430));
  outCoords1.push_back(osmscout::GeoCoord(51.57261,7.46563));
  outCoords1.push_back(osmscout::GeoCoord(51.57269,7.46594));

  outCoords2=outCoords1;
  std::reverse(outCoords2.begin(),outCoords2.end());

  outCoords3.push_back(osmscout::GeoCoord(51.58549,7.55493));
  outCoords3.push_back(osmscout::GeoCoord(51.58549,7.55494));
  outCoords3.push_back(osmscout::GeoCoord(51.58550,7.55496));
  outCoords3.push_back(osmscout::GeoCoord(51.58547,7.55504));
  outCoords3.push_back(osmscout::GeoCoord(51.58544,7.55506));
  outCoords3.push_back(osmscout::GeoCoord(51.58544,7.55507));
  outCoords3.push_back(osmscout::GeoCoord(51.58543,7.55508));
  outCoords3.push_back(osmscout::GeoCoord(51.58542,7.55508));
  outCoords3.push_back(osmscout::GeoCoord(51.58540,7.55508));
  outCoords3.push_back(osmscout::GeoCoord(51.58539,7.55508));
  outCoords3.push_back(osmscout::GeoCoord(51.58538,7.55500));
  outCoords3.push_back(osmscout::GeoCoord(51.58538,7.55498));
  outCoords3.push_back(osmscout::GeoCoord(51.58539,7.55495));
  outCoords3.push_back(osmscout::GeoCoord(51.58540,7.55494));
  outCoords3.push_back(osmscout::GeoCoord(51.58540,7.55488));
  outCoords3.push_back(osmscout::GeoCoord(51.58541,7.55484));
  outCoords3.push_back(osmscout::GeoCoord(51.58542,7.55484));
  outCoords3.push_back(osmscout::GeoCoord(51.58544,7.55483));
  outCoords3.push_back(osmscout::GeoCoord(51.58546,7.55484));
  outCoords3.push_back(osmscout::GeoCoord(51.58547,7.55485));
  outCoords3.push_back(osmscout::GeoCoord(51.58548,7.55488));
  outCoords3.push_back(osmscout::GeoCoord(51.58549,7.55492));

  outCoords4=outCoords3;
  std::reverse(outCoords4.begin(),outCoords4.end());

  outCoords5.push_back(osmscout::GeoCoord(5.0,-5.0));
  outCoords5.push_back(osmscout::GeoCoord(5.0,5.0));
  outCoords5.push_back(osmscout::GeoCoord(-5.0,5.0));

  outCoords6=outCoords5;
  std::reverse(outCoords6.begin(),outCoords6.end());

  if (writer.Open("test.dat")) {
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

    writer.Write(outCoords1);
    writer.Write(outCoords2);
    writer.Write(outCoords3);
    writer.Write(outCoords4);
    writer.Write(outCoords5);
    writer.Write(outCoords6);

    writer.Close();

    if (scanner.Open("test.dat",osmscout::FileScanner::Normal,false)) {

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

      scanner.Read(inCoords1);
      if (!Equals(inCoords1,outCoords1)) {
        std::cerr << "Read/Write(std::vector<GeoCoord>): Expected ";

        DumpGeoCoords(outCoords1);

        std::cout << ", got ";

        DumpGeoCoords(inCoords1);

        std::cout << std::endl;
        errors++;
      }

      scanner.Read(inCoords2);
      if (!Equals(inCoords2,outCoords2)) {
        std::cerr << "Read/Write(std::vector<GeoCoord>): Expected ";

        DumpGeoCoords(outCoords2);

        std::cout << ", got ";

        DumpGeoCoords(inCoords2);

        std::cout << std::endl;
        errors++;
      }

      scanner.Read(inCoords3);
      if (!Equals(inCoords3,outCoords3)) {
        std::cerr << "Read/Write(std::vector<GeoCoord>): Expected ";

        DumpGeoCoords(outCoords3);

        std::cout << ", got ";

        DumpGeoCoords(inCoords3);

        std::cout << std::endl;
        errors++;
      }

      scanner.Read(inCoords4);
      if (!Equals(inCoords4,outCoords4)) {
        std::cerr << "Read/Write(std::vector<GeoCoord>): Expected ";

        DumpGeoCoords(outCoords4);

        std::cout << ", got ";

        DumpGeoCoords(inCoords4);

        std::cout << std::endl;
        errors++;
      }

      scanner.Read(inCoords5);
      if (!Equals(inCoords5,outCoords5)) {
        std::cerr << "Read/Write(std::vector<GeoCoord>): Expected ";

        DumpGeoCoords(outCoords5);

        std::cout << ", got ";

        DumpGeoCoords(inCoords5);

        std::cout << std::endl;
        errors++;
      }

      scanner.Read(inCoords6);
      if (!Equals(inCoords6,outCoords6)) {
        std::cerr << "Read/Write(std::vector<GeoCoord>): Expected ";

        DumpGeoCoords(outCoords6);

        std::cout << ", got ";

        DumpGeoCoords(inCoords6);

        std::cout << std::endl;
        errors++;
      }
    }
    else {
      std::cerr << "Cannot open file for reading" << std::endl;
      errors++;
    }
  }
  else {
    std::cerr << "Cannot open file for writing" << std::endl;
    errors++;
  }

  if (errors!=0) {
    return 1;
  }
  else {
    return 0;
  }
}
