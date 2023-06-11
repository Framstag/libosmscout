/*
  ReaderScannerPerformance - a test program for libosmscout
  Copyright (C) 2009  Tim Teulings

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

#include <iostream>

#include <osmscout/Way.h>

#include <osmscout/io/File.h>
#include <osmscout/io/FileScanner.h>

#include <osmscout/util/StopClock.h>

/**
  Sequentially read the ways.dat file in the current directory first using
  FileReader and then using FileScanner and compare execution time.

  Call this program repeately to avoid different timing because of OS file caching.
*/

int main(int argc, char* argv[])
{
  osmscout::StopClock   scannerTimer;

  osmscout::TypeConfig  typeConfig;
  osmscout::FileScanner scanner;

  if (argc!=2) {
    std::cerr << "ReadScannerPerformance <map directory>" << std::endl;
    return 1;
  }

  std::string mapDirectory=argv[1];
  std::string wayDatFilename=osmscout::AppendFileToDir(mapDirectory,"ways.dat");

  if (!typeConfig.LoadFromDataFile(mapDirectory)) {
    std::cerr << "Cannot open type configuration!" << std::endl;
    return 1;
  }

  try {
    scanner.Open(wayDatFilename,osmscout::FileScanner::Sequential,true);

    std::cout << "Start reading files using FileScanner..." << std::endl;

    uint32_t wayCount=scanner.ReadUInt32();

    for (size_t w=1; w<=wayCount; w++) {
      osmscout::Way way;

      way.Read(typeConfig,
               scanner);
    }

    scanner.Close();

    scannerTimer.Stop();

    std::cout << "Reading " << wayCount << " ways via FileScanner took " << scannerTimer << std::endl;
  }
  catch (osmscout::IOException& e) {
    std::cerr << e.GetDescription() << std::endl;
    return 1;
  }

  return 0;
}
