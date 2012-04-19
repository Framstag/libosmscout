#ifndef OSMSCOUT_UTIL_FILE_H
#define OSMSCOUT_UTIL_FILE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <string>

#include <osmscout/private/CoreImportExport.h>

namespace osmscout {

  /**
   * Return the size of the file in the parameter size. Returns true, if the file size
   * could be calculated, else false.
   */
  extern OSMSCOUT_API bool GetFileSize(const std::string& filename, long& size);

  /**
   * Deletes the given file
   */
  extern OSMSCOUT_API bool RemoveFile(const std::string& filename);

  /**
   * Rename a file
   */
  extern OSMSCOUT_API bool RenameFile(const std::string& oldFilename,
                                      const std::string& newFilename);

  /**
   * Append the filename 'name' to the directory name 'name' correctly adding directory
   * delimiter if necessary.
   */
  extern OSMSCOUT_API std::string AppendFileToDir(const std::string& dir, const std::string& file);
}

#endif
