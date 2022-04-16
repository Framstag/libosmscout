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

#include <cstdio>
#include <cstdlib>

#include <filesystem>

#include <osmscout/util/Exception.h>
#include <osmscout/util/File.h>
#include <osmscout/util/Number.h>
#include <osmscout/util/Logger.h>

namespace osmscout {

  FileOffset GetFileSize(const std::string& filename)
  {
    try {
      return std::filesystem::file_size(filename);
    }
    catch (std::filesystem::filesystem_error& e) {
      throw IOException(filename,"Cannot read size of file",e);
    }
  }

  bool RemoveFile(const std::string& filename)
  {
    try {
      std::filesystem::remove(filename);

      return true;
    }
    catch (std::filesystem::filesystem_error& e) {
      return false;
    }
  }

  /**
   * Rename a file
   */
  bool RenameFile(const std::string& oldFilename,
                  const std::string& newFilename)
  {
    try {
      std::filesystem::rename(oldFilename,
                             newFilename);

      return true;
    }
    catch (std::filesystem::filesystem_error& e) {
      return false;
    }
  }

  std::string GetDirectory(const std::string& file)
  {
    std::filesystem::path filePath=file;

    if (!filePath.has_filename()) {
      return filePath.make_preferred().string();
    }

    return filePath.remove_filename().make_preferred().string();
  }


  std::string AppendFileToDir(const std::string& dir, const std::string& file)
  {
    std::filesystem::path dirPath=dir;

    return dirPath.append(file).make_preferred().string();
  }

  uint8_t BytesNeededToAddressFileData(const std::string& filename)
  {
    FileOffset fileSize=GetFileSize(filename);

    return BytesNeededToEncodeNumber(fileSize);
  }

  bool ExistsInFilesystem(const std::string& filename)
  {
    try {
      return std::filesystem::exists(filename);
    }
    catch (std::filesystem::filesystem_error& e) {
      return false;
    }
  }

  bool IsDirectory(const std::string& filename)
  {
    try {
      return std::filesystem::is_directory(filename);
    }
    catch (std::filesystem::filesystem_error& e) {
      return false;
    }
  }

  bool ReadFile(const std::string& filename, std::vector<char> &contentOut)
  {
    if (!ExistsInFilesystem(filename)){
      return false;
    }

    try {
      FILE*      file;
      FileOffset fileSize;

      fileSize=GetFileSize(filename);

      file=fopen(filename.c_str(),"rb");
      if (file==nullptr) {
        log.Error() << "Cannot open file '" << filename << "'";

        return false;
      }

      contentOut.resize(fileSize);

      if (fread(contentOut.data(),1,fileSize,file)!=(size_t)fileSize) {
        log.Error() << "Cannot load file '" << filename << "'";
        fclose(file);
        return false;
      }

      fclose(file);

    } catch (const IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
    return true;
  }
}
