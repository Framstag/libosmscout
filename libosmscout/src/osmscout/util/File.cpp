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

#include <osmscout/private/Config.h>

#include <osmscout/util/File.h>

#include <stdio.h>
#include <stdlib.h>

namespace osmscout {

  bool GetFileSize(const std::string& filename, FileOffset& size)
  {
    FILE *file;

    file=fopen(filename.c_str(),"rb");

    if (file==NULL) {
      return false;
    }

#if defined(HAVE_FSEEKO)
    if (fseeko(file,0L,SEEK_END)!=0) {
      fclose(file);

      return false;
    }

    off_t pos=ftello(file);

    if (pos==-1) {
      fclose(file);

      return false;
    }

    size=(unsigned long)pos;

#else
    if (fseek(file,0L,SEEK_END)!=0) {
      fclose(file);

      return false;
    }

    unsigned long pos=ftell(file);

    if (pos==-1) {
      fclose(file);

      return false;
    }

    size=(unsigned long)pos;
#endif

    fclose(file);

    return true;
  }

  bool RemoveFile(const std::string& filename)
  {
    return remove(filename.c_str())==0;
  }

  /**
   * Rename a file
   */
  bool RenameFile(const std::string& oldFilename,
                  const std::string& newFilename)
  {
    return rename(oldFilename.c_str(),
                  newFilename.c_str())==0;
  }

  std::string AppendFileToDir(const std::string& dir, const std::string& file)
  {
#if defined(__WIN32__) || defined(WIN32)
    std::string result(dir);

    if (result.length()>0 && result[result.length()-1]!='\\') {
      result.append("\\");
    }

    result.append(file);

    return result;
#else
    std::string result(dir);

    if (result.length()>0 && result[result.length()-1]!='/') {
      result.append("/");
    }

    result.append(file);

    return result;
#endif
  }

  uint8_t BytesNeededToAddressFileData(FileOffset size)
  {
    uint8_t bytes=0;

    while (size>0) {
      size=size/256;
      bytes++;
    }

    if (bytes==0) {
      bytes=1;
    }

    return bytes;
  }

  bool BytesNeeededToAddressFileData(const std::string& filename,
                                     uint8_t& bytes)
  {
    FileOffset fileSize;

    if (!GetFileSize(filename,
                     fileSize)) {
      return false;
    }

    bytes=BytesNeededToAddressFileData(fileSize);

    return false;
  }
}
