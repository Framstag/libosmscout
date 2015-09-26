/*
  CachePerformance - a test program for libosmscout
  Copyright (C) 2015  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

#include <osmscout/AreaDataFile.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/WayDataFile.h>

#include <osmscout/util/StopClock.h>

class Statistics
{
public:
  size_t  numberOfVectors;   // Overall number of vectors
  size_t  emptyVectorCount;  // Number of vectors with size()==0
  size_t  sixBitVectorCount; // Length of vector is 0..63
  size_t  coordCount;        // Overall number of coordinates
  size_t  minLength;         // Minimum length of coordinate array
  size_t  maxLength;         // Maximum length of coordinate array
  int32_t minDelta;
  int32_t maxDelta;
  size_t  deltaCount;
  int64_t deltaSum;
  size_t  threeBitDeltaCount;
  size_t  fourBitDeltaCount;
  size_t  fiveBitDeltaCount;
  size_t  sixBitDeltaCount;
  size_t  sevenBitDeltaCount;
  size_t  fifteenBitDeltaCount;
  size_t  twentythreeBitDeltaCount;


public:
  Statistics()
  : numberOfVectors(0),
    emptyVectorCount(0),
    sixBitVectorCount(0),
    coordCount(0),
    minLength(std::numeric_limits<size_t>::max()),
    maxLength(std::numeric_limits<size_t>::min()),
    minDelta(std::numeric_limits<int32_t>::max()),
    maxDelta(std::numeric_limits<int32_t>::min()),
    deltaCount(0),
    deltaSum(0),
    threeBitDeltaCount(0),
    fourBitDeltaCount(0),
    fiveBitDeltaCount(0),
    sixBitDeltaCount(0),
    sevenBitDeltaCount(0),
    fifteenBitDeltaCount(0),
    twentythreeBitDeltaCount(0)
  {
    // no code
  }

  void Measure(const std::vector<osmscout::GeoCoord>& coords)
  {
    numberOfVectors++;
    coordCount+=coords.size();

    if (coords.size()<64) {
      sixBitVectorCount++;
    }

    if (!coords.empty()) {
      minLength=std::min(minLength,coords.size());
      maxLength=std::max(maxLength,coords.size());
    }
    else {
      emptyVectorCount++;
    }

    if (coords.empty()) {
      return;
    }

    uint32_t lastLat=0;
    uint32_t lastLon=0;

    for (size_t i=0; i<coords.size(); i++) {
      uint32_t currentLat=(uint32_t)round(coords[i].GetLat()*osmscout::latConversionFactor);
      uint32_t currentLon=(uint32_t)round(coords[i].GetLon()*osmscout::lonConversionFactor);

      if (i>0) {
        int32_t deltaLat=currentLat-lastLat;
        int32_t deltaLon=currentLon-lastLon;

        minDelta=std::min(minDelta,std::abs(deltaLat));
        minDelta=std::min(minDelta,std::abs(deltaLon));

        maxDelta=std::max(maxDelta,std::abs(deltaLat));
        maxDelta=std::max(maxDelta,std::abs(deltaLon));

        deltaSum+=std::abs(deltaLat);
        deltaSum+=std::abs(deltaLon);
        deltaCount+=2;

        if (deltaLat>=-8 && deltaLat<=7) {
          threeBitDeltaCount++;
        }
        else if (deltaLat>=-16 && deltaLat<=15) {
          fourBitDeltaCount++;
        }
        else if (deltaLat>=-32 && deltaLat<=31) {
          fiveBitDeltaCount++;
        }
        else if (deltaLat>=-64 && deltaLat<=63) {
          sixBitDeltaCount++;
        }
        else if (deltaLat>=-128 && deltaLat<=127) {
          sevenBitDeltaCount++;
        }
        else if (deltaLat>=-32768 && deltaLat<=32767) {
          fifteenBitDeltaCount++;
        }
        else {
          twentythreeBitDeltaCount++;
        }

        if (deltaLon>=-8 && deltaLon<=7) {
          threeBitDeltaCount++;
        }
        else if (deltaLon>=-16 && deltaLon<=15) {
          fourBitDeltaCount++;
        }
        else if (deltaLon>=-32 && deltaLon<=31) {
          fiveBitDeltaCount++;
        }
        else if (deltaLon>=-64 && deltaLon<=63) {
          sixBitDeltaCount++;
        }
        else if (deltaLon>=-128 && deltaLon<=127) {
          sevenBitDeltaCount++;
        }
        else if (deltaLon>=-32768 && deltaLon<=32767) {
          fifteenBitDeltaCount++;
        }
        else {
          twentythreeBitDeltaCount++;
        }

        //bytesNeeded+=osmscout::EncodeNumber(deltaLat,buffer);
        //bytesNeeded+=osmscout::EncodeNumber(deltaLon,buffer);
      }

      lastLat=currentLat;
      lastLon=currentLon;
    }

  }
};

class Encoder
{
public:
  std::string name;
  size_t      bytesNeeded;

public:
  Encoder(const std::string& name)
  : name(name),
    bytesNeeded(0)
  {
    // no code
  }

  virtual ~Encoder()
  {
    // no code
  }

  virtual void Encode(osmscout::FileOffset offset, const std::vector<osmscout::GeoCoord>& coords) = 0;
};

/**
 * No encoding at all, just write down the coordinates as they come...
 */
class TrivialEncoder : public Encoder
{
private:
  char buffer[10]; // Enough for 64bit values
public:
  TrivialEncoder()
  : Encoder("TrivialEncoder")
  {
    // no code
  }

  void Encode(osmscout::FileOffset offset, const std::vector<osmscout::GeoCoord>& coords)
  {
    bytesNeeded+=osmscout::EncodeNumber(coords.size(),buffer);

    if (coords.empty()) {
      return;
    }

    bytesNeeded+=osmscout::coordByteSize*coords.size();
  }
};

/**
 * Calculate the minimum coordinate and encode all other values
 * as VLQ encoded values relative to the minimum.
 *
 * That is the current implementation in libosmscout
 * at the time of writing.
 */
class MinimumVLQDeltaEncoder : public Encoder
{
private:
  char buffer[10]; // Enough for 64bit values

public:
  MinimumVLQDeltaEncoder()
          : Encoder("MinimumVLQDeltaEncoder")
  {
    // no code
  }

  void Encode(osmscout::FileOffset offset, const std::vector<osmscout::GeoCoord>& coords)
  {
    bytesNeeded+=osmscout::EncodeNumber(coords.size(),buffer);

    if (coords.empty()) {
      return;
    }

    osmscout::GeoCoord minCoord=coords[0];

    for (size_t i=1; i<coords.size(); i++) {
      minCoord.Set(std::min(minCoord.GetLat(),coords[i].GetLat()),
                   std::min(minCoord.GetLon(),coords[i].GetLon()));
    }

    // minCoord without any compression
    bytesNeeded+=osmscout::coordByteSize;

    for (size_t i=0; i<coords.size(); i++) {
      bytesNeeded+=osmscout::EncodeNumber((uint32_t)round((coords[i].GetLat()-minCoord.GetLat())*osmscout::latConversionFactor),buffer);
      bytesNeeded+=osmscout::EncodeNumber((uint32_t)round((coords[i].GetLon()-minCoord.GetLon())*osmscout::lonConversionFactor),buffer);
    }
  }
};

/**
 * Just store the relative delta using VLQ between values.
 */
class VLQDeltaEncoder : public Encoder
{
private:
  char buffer[10]; // Enough for 64bit values

public:
  VLQDeltaEncoder()
          : Encoder("VLQDeltaEncoder")
  {
    // no code
  }

  void Encode(osmscout::FileOffset offset, const std::vector<osmscout::GeoCoord>& coords)
  {
    bytesNeeded+=osmscout::EncodeNumber(coords.size(),buffer);

    if (coords.empty()) {
      return;
    }

    uint32_t lastLat=0;
    uint32_t lastLon=0;

    for (size_t i=0; i<coords.size(); i++) {
      uint32_t currentLat=(uint32_t)round(coords[i].GetLat()*osmscout::latConversionFactor);
      uint32_t currentLon=(uint32_t)round(coords[i].GetLon()*osmscout::lonConversionFactor);

      if (i==0) {
        bytesNeeded+=osmscout::EncodeNumber(currentLat,buffer);
        bytesNeeded+=osmscout::EncodeNumber(currentLon,buffer);
      }
      else {
        int32_t deltaLat=currentLat-lastLat;
        int32_t deltaLon=currentLon-lastLon;

        bytesNeeded+=osmscout::EncodeNumber(deltaLat,buffer);
        bytesNeeded+=osmscout::EncodeNumber(deltaLon,buffer);
      }

      lastLat=currentLat;
      lastLon=currentLon;
    }
  }
};

/**
 * Encode some signal bits for signaling the delta byte size into the size
 * Calculate the maximum size of delta and encode all delta with the maximum required byte size.
 */
class StaticOptimizedDeltaEncoder : public Encoder
{
private:
  std::vector<int32_t> deltaBuffer;
  char                 buffer[10]; // Enough for 64bit values

public:
  StaticOptimizedDeltaEncoder()
          : Encoder("StaticOptimizedDeltaEncoder")
  {
    // no code
  }

  void Encode(osmscout::FileOffset offset, const std::vector<osmscout::GeoCoord>& coords)
  {
    if (coords.empty()) {
      bytesNeeded++;
      return;
    }

    if (coords.size()<64) /* 2^6) */ {
      bytesNeeded++;
    }
    else if (coords.size()<8192) /* 2^(6+7) */ {
      bytesNeeded+=2;
    }
    else /* 2097152 / 2^(6+7+8)) */ {
      bytesNeeded+=3;
    }

    uint32_t lastLat=(uint32_t)round(coords[0].GetLat()*osmscout::latConversionFactor);
    uint32_t lastLon=(uint32_t)round(coords[0].GetLon()*osmscout::lonConversionFactor);

    bytesNeeded+=osmscout::coordByteSize; // VLQ does not make things better :-/

    deltaBuffer.resize((coords.size()-1)*2);
    size_t pos=0;
    for (size_t i=1; i<coords.size(); i++) {
      uint32_t currentLat=(uint32_t)round(coords[i].GetLat()*osmscout::latConversionFactor);
      uint32_t currentLon=(uint32_t)round(coords[i].GetLon()*osmscout::lonConversionFactor);

      deltaBuffer[pos]=currentLat-lastLat;
      pos++;

      deltaBuffer[pos]=currentLon-lastLon;
      pos++;

      lastLat=currentLat;
      lastLon=currentLon;
    }

    int coordBitSize=0; // space needed to encode lat and lon

    for (size_t i=0; i<deltaBuffer.size()-1; i++) {
      if (deltaBuffer[i]>=-128 && deltaBuffer[i]<=127) {
        coordBitSize=std::max(coordBitSize,16);
      }
      else if (deltaBuffer[i]>=-32768 && deltaBuffer[i]<=32767) {
        coordBitSize=std::max(coordBitSize,32);
      }
      else {
        coordBitSize=std::max(coordBitSize,48);
      }
    }

    bytesNeeded+=(coords.size()-1)*coordBitSize/8;

    if (coordBitSize % 8 !=0) {
      bytesNeeded++;
    }
  }
};

int main(int argc, char* argv[])
{
  if (argc!=2) {
    std::cerr << "CoordinateEncoding <map directory>" << std::endl;
    return 1;
  }

  std::list<Encoder*>   encoders;
  Statistics            statistics;

  encoders.push_back(new TrivialEncoder());
  encoders.push_back(new MinimumVLQDeltaEncoder());
  encoders.push_back(new VLQDeltaEncoder());
  encoders.push_back(new StaticOptimizedDeltaEncoder());

  std::string mapDirectory=argv[1];
  std::string areaDatFilename=osmscout::AppendFileToDir(mapDirectory,"areas.dat");
  std::string wayDatFilename=osmscout::AppendFileToDir(mapDirectory,"ways.dat");

  osmscout::TypeConfig  typeConfig;
  osmscout::FileScanner scanner;
  uint32_t              dataCount;

  std::cout << "Reading type config from map directory '" << mapDirectory << "'..." << std::endl;

  if (!typeConfig.LoadFromDataFile(mapDirectory)) {
    std::cerr << "Cannot open type config" << std::endl;
    return 1;
  }

  std::cout << "Reading '" << areaDatFilename << "'..." << std::endl;

  if (!scanner.Open(areaDatFilename,osmscout::FileScanner::Sequential,true)) {
    std::cerr << "Cannot open '" << scanner.GetFilename() << "'" << std::endl;
    return 1;
  }

  if (!scanner.Read(dataCount)) {
    std::cerr << "Cannot read number of entries in file" << std::endl;
  }

  std::cout << dataCount << " entries..." << std::endl;

  for (size_t i=1; i<=dataCount; i++) {
    osmscout::Area area;

    if (!area.Read(typeConfig,scanner)) {
      std::cerr << "Cannot read data set #" << i << "'from file " << scanner.GetFilename() << "'" << std::endl;
      return 1;
    }

    for (const auto& ring : area.rings) {
      statistics.Measure(ring.nodes);

      for (auto& encoder : encoders) {
        encoder->Encode(area.GetFileOffset(),ring.nodes);
      }
    }
  }

  scanner.Close();

  std::cout << "Reading " << wayDatFilename << "..." << std::endl;

  if (!scanner.Open(wayDatFilename,osmscout::FileScanner::Sequential,true)) {
    std::cerr << "Cannot open '" << scanner.GetFilename() << "'" << std::endl;
    return 1;
  }

  if (!scanner.Read(dataCount)) {
    std::cerr << "Cannot read number of entries in file" << std::endl;
  }

  std::cout << dataCount << " entries..." << std::endl;

  for (size_t i=1; i<=dataCount; i++) {
    osmscout::Way way;

    if (!way.Read(typeConfig,scanner)) {
      std::cerr << "Cannot read data set #" << i << "'from file " << scanner.GetFilename() << "'" << std::endl;
      return 1;
    }

    statistics.Measure(way.nodes);

    for (auto& encoder : encoders) {
      encoder->Encode(way.GetFileOffset(),way.nodes);
    }
  }

  scanner.Close();

  std::cout << "---" << std::endl;

  for (auto& encoder : encoders) {
    std::cout << "Encoder: " << encoder->name << " " << encoder->bytesNeeded << std::endl;
    delete encoder;
  }

  encoders.clear();

  std::cout << "---" << std::endl;
  std::cout << "Number of vectors: " << statistics.numberOfVectors << std::endl;
  std::cout << "Number of empty vectors: " << statistics.emptyVectorCount << std::endl;
  std::cout << "Number of six bit length vectors: " << statistics.sixBitVectorCount << " " << statistics.sixBitVectorCount*100.0/statistics.numberOfVectors << "%" << std::endl;
  std::cout << "Number of coords: " << statistics.coordCount << std::endl;
  std::cout << "Min. length: " << statistics.minLength << std::endl;
  std::cout << "Max. length: " << statistics.maxLength << std::endl;
  std::cout << "Avg. length: " << statistics.coordCount*1.0/statistics.numberOfVectors << std::endl;
  std::cout << "Delta: " << statistics.minDelta << " - " << statistics.deltaSum/statistics.deltaCount << " - " << statistics.maxDelta;
  std::cout << " (" << statistics.deltaSum << "/" << statistics.deltaCount << ")" << std::endl;
  std::cout << "Delta distribution (3,4,5,6,7,15,23 bits): ";
  std::cout << statistics.threeBitDeltaCount*100.0/statistics.deltaCount << "% ";
  std::cout << statistics.fourBitDeltaCount*100.0/statistics.deltaCount << "% ";
  std::cout << statistics.fiveBitDeltaCount*100.0/statistics.deltaCount << "% ";
  std::cout << statistics.sixBitDeltaCount*100.0/statistics.deltaCount << "% ";
  std::cout << statistics.sevenBitDeltaCount*100.0/statistics.deltaCount << "% ";
  std::cout << statistics.fifteenBitDeltaCount*100.0/statistics.deltaCount << "% ";
  std::cout << statistics.twentythreeBitDeltaCount*100.0/statistics.deltaCount << "%" << std::endl;

  return 0;
}
