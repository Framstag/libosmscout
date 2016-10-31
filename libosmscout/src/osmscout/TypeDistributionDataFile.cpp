/*
  This source is part of the libosmscout library
  Copyright (C) 2016  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/TypeDistributionDataFile.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Logger.h>

namespace osmscout {

  const char* TypeDistributionDataFile::DISTRIBUTION_DAT="distribution.dat";


  TypeDistributionDataFile::Distribution::Distribution()
    : nodeCount(0),
      wayCount(0),
      areaCount(0)
  {
    // no code
  }

  TypeDistributionDataFile::TypeDistributionDataFile()
    : isLoaded(false)
  {
    // no code
  }

  TypeDistributionDataFile::~TypeDistributionDataFile()
  {
    // no code
  }

  /**
   *
   * Load the bounding box data and return, if this operation was successful.
   *
   * @param path
   *    Directory, wehre the data file has been placed
   * @return
   *    True on success, else false
   */
  bool TypeDistributionDataFile::Load(const TypeConfig& typeConfig,
                                      const std::string& path)
  {
    isLoaded=false;
    distribution.clear();

    filename=AppendFileToDir(path,
                             DISTRIBUTION_DAT);

    distribution.resize(typeConfig.GetTypeCount());

    FileScanner scanner;

    try {
      scanner.Open(filename,
                   FileScanner::Sequential,
                   true);

      for (const auto &type : typeConfig.GetTypes()) {
        scanner.Read(distribution[type->GetIndex()].nodeCount);
        scanner.Read(distribution[type->GetIndex()].wayCount);
        scanner.Read(distribution[type->GetIndex()].areaCount);
      }

      scanner.Close();

      isLoaded=true;
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }

    return true;
  }
}
