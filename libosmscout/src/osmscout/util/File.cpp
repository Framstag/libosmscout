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

#include <stdio.h>
#include <stdlib.h>

#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif

#if defined(__WIN32__) || defined(WIN32)
#include <windows.h>
#endif

#include <osmscout/util/Exception.h>
#include <osmscout/util/File.h>
#include <osmscout/util/Number.h>

namespace osmscout {

  FileOffset GetFileSize(const std::string& filename)
  {
    FILE *file;

    file=fopen(filename.c_str(),"rb");

    if (file==NULL) {
      throw IOException(filename,"Opening file");
    }

#if defined(__WIN32__) || defined(WIN32)
  if (_fseeki64(file, 0L, SEEK_END) != 0) {
    fclose(file);
    throw IOException(filename, "Seeking end of file");
  }

  const __int64 size = _ftelli64(file);

  if (size == -1) {
    fclose(file);

    throw IOException(filename, "Getting current file position");
  }

  fclose(file);

  return (FileOffset)size;
#elif defined(HAVE_FSEEKO)
    if (fseeko(file,0L,SEEK_END)!=0) {
      fclose(file);

      throw IOException(filename,"Seeking end of file");
    }

    off_t pos=ftello(file);

    if (pos==-1) {
      fclose(file);

      throw IOException(filename,"Getting current file position");
    }

    fclose(file);

    return (FileOffset)pos;

#else
    if (fseek(file,0L,SEEK_END)!=0) {
      fclose(file);

      throw IOException(filename,"Seeking end of file");
    }

    long int pos=ftell(file);

    if (pos==-1) {
      fclose(file);

      throw IOException(filename,"Getting current file position");
    }

    fclose(file);

    return (FileOffset)pos;
#endif
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

  uint8_t BytesNeededToAddressFileData(const std::string& filename)
  {
    FileOffset fileSize=GetFileSize(filename);

    return BytesNeededToEncodeNumber(fileSize);
  }

  bool ExistsInFilesystem(const std::string& filename)
  {
#if defined(__WIN32__) || defined(WIN32)
    return GetFileAttributes(filename.c_str())!=INVALID_FILE_ATTRIBUTES;
#elif defined(HAVE_SYS_STAT_H)
    struct stat s;
    return stat(filename.c_str(),&s)==0;
#else
    throw IOException(filename,"Is file directory","Not implemented");
#endif
  }

  bool IsDirectory(const std::string& filename)
  {
#if defined(__WIN32__) || defined(WIN32)
    DWORD attributes=GetFileAttributes(filename.c_str());

    if (attributes==INVALID_FILE_ATTRIBUTES) {
      throw IOException(filename,"Is file directory");
    }

    return (attributes & FILE_ATTRIBUTE_DIRECTORY)!=0;
#elif defined(HAVE_SYS_STAT_H)
    struct stat s;

    if (stat(filename.c_str(),
             &s)!=0) {
      throw IOException(filename,"Is file directory");
    }

    return s.st_mode & S_IFDIR;
#else
    throw IOException(filename,"Is file directory","Not implemented");
#endif
  }

}
