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

#include <osmscout/StyleConfigLoader.h>

#include <string.h>

#include <cassert>
#include <iostream>
#include <sstream>

#include <osmscout/Util.h>

#include <osmscout/oss/Parser.h>
#include <osmscout/oss/Scanner.h>

namespace osmscout {


  bool LoadStyleConfig(const char* styleFile,
                       StyleConfig& styleConfig)
  {
    long  fileSize;
    FILE* file;
    bool  success=false;

    if (!GetFileSize(styleFile,fileSize)) {
      std::cerr << "Cannot get size of file '" << styleFile << "'" << std::endl;
      return false;
    }

    file=fopen(styleFile,"rb");
    if (file==NULL) {
      std::cerr << "Cannot open file '" << styleFile << "'" << std::endl;
      return false;
    }

    unsigned char* content=new unsigned char[fileSize];

    if (fread(content,1,fileSize,file)!=(size_t)fileSize) {
      std::cerr << "Cannot load file '" << styleFile << "'" << std::endl;
      delete [] content;
      fclose(file);
      return false;
    }

    fclose(file);

    oss::Scanner *scanner=new oss::Scanner(content,fileSize);
    oss::Parser  *parser=new oss::Parser(scanner,styleConfig);

    delete [] content;

    parser->Parse();

    success=!parser->errors->hasErrors;

    delete parser;
    delete scanner;

    styleConfig.Postprocess();

    return success;
  }
}

