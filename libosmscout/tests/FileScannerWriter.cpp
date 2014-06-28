#include <iostream>
#include <limits>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

#include <osmscout/CoreFeatures.h>

int errors=0;

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

#if defined(OSMSCOUT_HAVE_UINT64_T)
  uint64_t              out64u1=std::numeric_limits<uint64_t>::min();
  uint64_t              out64u2=std::numeric_limits<uint64_t>::max()/2;
  uint64_t              out64u3=std::numeric_limits<uint64_t>::max();
#endif

  osmscout::FileOffset  outfo1=std::numeric_limits<osmscout::FileOffset>::min();
  osmscout::FileOffset  outfo2=std::numeric_limits<osmscout::FileOffset>::max()/2;
  osmscout::FileOffset  outfo3=std::numeric_limits<osmscout::FileOffset>::max();

  bool                  inBool;
  uint16_t              in16u;
  uint32_t              in32u;
#if defined(OSMSCOUT_HAVE_UINT64_T)
  uint64_t              in64u;
#endif

  osmscout::FileOffset  info;

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

#if defined(OSMSCOUT_HAVE_UINT64_T)
    writer.WriteNumber(out64u1);
    writer.WriteNumber(out64u2);
    writer.WriteNumber(out64u3);
#endif

    writer.WriteFileOffset(outfo1);
    writer.WriteFileOffset(outfo2);
    writer.WriteFileOffset(outfo3);

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

#if defined(OSMSCOUT_HAVE_UINT64_T)
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
#endif

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

#if defined(OSMSCOUT_HAVE_UINT64_T)
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
#endif

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
