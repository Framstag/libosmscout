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
#include <vector>

#include <osmscout/CoreImportExport.h>

#include <osmscout/OSMScoutTypes.h>

namespace osmscout {


  /**
   * \defgroup File Platform independent file access
   *
   * Classes and methods related to low-level platform
   * independent access to files on disk.
   */

  /**
   * \ingroup File
   *
   * Return the size of the file in the parameter size.
   *
   * @throws IOException
   */
  extern OSMSCOUT_API FileOffset GetFileSize(const std::string& filename);

  /**
   * \ingroup File
   *
   * Deletes the given file
   */
  extern OSMSCOUT_API bool RemoveFile(const std::string& filename);

  /**
   * \ingroup File
   *
   * Rename a file
   */
  extern OSMSCOUT_API bool RenameFile(const std::string& oldFilename,
                                      const std::string& newFilename);

  /**
   * \ingroup File
   *
   * Append the filename 'name' to the directory name 'name' correctly adding directory
   * delimiter if necessary.
   */
  extern OSMSCOUT_API std::string AppendFileToDir(const std::string& dir, const std::string& file);

  /**
   * \ingroup File
   *
   * Number of bytes needed to address the complete content of the given file.
   *
   * @throws IOException
   */
  extern OSMSCOUT_API uint8_t BytesNeededToAddressFileData(const std::string& filename);

  /**
   * \ingroup File
   *
   * Returns true of the given filename exists in the filesystem. Else it returns false.
   *
   * Note that this function returns, does not tell you anything about the type of the
   * filesystem entry. It you still be a regular file, a directory, a pipe or something
   * completely different.
   *
   * Also not ethat there is no differenciation in error codes. If there was an error
   * during access to the file entry while though the file entry does in fact exists,
   * false ist still returned.
   *
   * @throws IOException if the function is not implemented.
   */
  extern OSMSCOUT_API bool ExistsInFilesystem(const std::string& filename);

  /**
   * \ingroup File
   *
   * Returns true of the given filename points to a directory. Else it returns false.
   *
   * @throws IOException if there was an error or if the function is not implemented.
   */
  extern OSMSCOUT_API bool IsDirectory(const std::string& filename);

  extern OSMSCOUT_API bool ReadFile(const std::string& filename, std::vector<char>& content);
}

#endif
