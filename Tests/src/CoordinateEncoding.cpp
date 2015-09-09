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
#include <vector>

#include <osmscout/AreaDataFile.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/WayDataFile.h>

#include <osmscout/util/StopClock.h>

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

  virtual void Encode(const std::vector<osmscout::GeoCoord>& coords) = 0;
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

  void Encode(const std::vector<osmscout::GeoCoord>& coords)
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

  void Encode(const std::vector<osmscout::GeoCoord>& coords)
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

  void Encode(const std::vector<osmscout::GeoCoord>& coords)
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

int main(int argc, char* argv[])
{
  if (argc!=2) {
    std::cerr << "CoordinateEncoding <map directory>" << std::endl;
    return 1;
  }

  std::list<Encoder*> encoders;

  encoders.push_back(new TrivialEncoder());
  encoders.push_back(new MinimumVLQDeltaEncoder());
  encoders.push_back(new VLQDeltaEncoder());

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
      for (auto& encoder : encoders) {
        encoder->Encode(ring.nodes);
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

    for (auto& encoder : encoders) {
      encoder->Encode(way.nodes);
    }
  }

  scanner.Close();

  for (auto& encoder : encoders) {
    std::cout << "Encoder: " << encoder->name << " " << encoder->bytesNeeded << std::endl;
    delete encoder;
  }

  encoders.clear();

  return 0;
}
