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

#include <osmscout/util/File.h>

#include <cstdio>
#include <cstdlib>

namespace osmscout {

  bool GetFileSize(const std::string& filename, long& size)
  {
    FILE *file;

    file=fopen(filename.c_str(),"rb");

    if (file==NULL) {
      return false;
    }

    if (fseek(file,0L,SEEK_END)!=0) {
      fclose(file);

      return false;
    }

    size=ftell(file);

    if (size==-1) {
      fclose(file);

      return false;
    }

    fclose(file);

    return true;
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
}
