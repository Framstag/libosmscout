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

#include <osmscout/ObjectVariantDataFile.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Logger.h>

namespace osmscout {

  ObjectVariantDataFile::ObjectVariantDataFile()
  : isLoaded(false)
  {
    // no code
  }

  ObjectVariantDataFile::~ObjectVariantDataFile()
  {
    // no code
  }

  /**
   * Load the object variant data from the given file.
   *
   * @param typeConfig
   *    TypeConfig instance
   * @param filename
   *    Name of the file containing the object variant data
   * @return
   *    True on success, else false
   */
  bool ObjectVariantDataFile::Load(const TypeConfig& typeConfig,
                                   const std::string& filename)
  {
    FileScanner scanner;

    data.clear();

    isLoaded=false;
    this->filename=filename;

    try {
      scanner.Open(filename,
                   FileScanner::Sequential,
                   true);

      uint32_t dataCount;

      scanner.Read(dataCount);

      data.resize(dataCount);

      for (size_t i=0; i<dataCount; i++) {
        data[i].Read(typeConfig,
                     scanner);
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
