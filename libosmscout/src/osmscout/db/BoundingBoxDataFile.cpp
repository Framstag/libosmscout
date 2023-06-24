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

#include <osmscout/db/BoundingBoxDataFile.h>

#include <osmscout/io/File.h>
#include <osmscout/io/FileScanner.h>

#include <osmscout/log/Logger.h>

namespace osmscout {

  const char* const BoundingBoxDataFile::BOUNDINGBOX_DAT="bounding.dat";

  BoundingBoxDataFile::BoundingBoxDataFile()
  : isLoaded(false)
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
  bool BoundingBoxDataFile::Load(const std::string& path)
  {
    FileScanner scanner;

    isLoaded=false;
    filename=AppendFileToDir(path,
                             BOUNDINGBOX_DAT);

    try {
      scanner.Open(filename,
                   FileScanner::Sequential,
                   true);

      boundingBox=scanner.ReadBox();

      log.Debug() << "BoundingBox: " << boundingBox.GetDisplayText();

      scanner.Close();

      isLoaded=true;
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();

      return false;
    }

    return true;
  }
}
