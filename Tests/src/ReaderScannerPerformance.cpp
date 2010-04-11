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

#include <osmscout/FileReader.h>
#include <osmscout/FileScanner.h>
#include <osmscout/Way.h>
#include <osmscout/Util.h>

/**
  Sequentially read the ways.dat file in the current directory first using
  FileReader and then using FileScanner and compare execution time.

  Call this program repeately to avoid different timing because of OS file caching.
*/

int main(int argc, char* argv[])
{
  std::string filename="ways.dat";
  size_t      scannerWayCount;

  osmscout::StopClock scannerTimer;

  osmscout::FileScanner scanner;

  if (!scanner.Open(filename)) {
    std::cerr << "Cannot open of file '" << filename << "'!" << std::endl;
    return 1;
  }

  std::cout << "Start reading files using FileScanner..." << std::endl;

  scannerWayCount=0;
  while (!scanner.HasError()) {
    osmscout::Way way;

    if (way.Read(scanner)) {
      scannerWayCount++;
    }
  }

  scanner.Close();

  scannerTimer.Stop();

  std::cout << "Reading " << scannerWayCount << " ways via FileScanner took " << scannerTimer << std::endl;

  return 0;
}