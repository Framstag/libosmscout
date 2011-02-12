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

#include <osmscout/TypeConfigLoader.h>

#include <cstdio>
#include <iostream>

#include <osmscout/Util.h>

#include <osmscout/ost/Parser.h>
#include <osmscout/ost/Scanner.h>

namespace osmscout {

  bool LoadTypeConfig(const char* typeFile,
                      TypeConfig& config)
  {
    long  fileSize;
    FILE* file;
    bool  success=false;

    if (!GetFileSize(typeFile,fileSize)) {
      std::cerr << "Cannot get size of file '" << typeFile << "'" << std::endl;
      return false;
    }

    file=fopen(typeFile,"rb");
    if (file==NULL) {
      std::cerr << "Cannot open file '" << typeFile << "'" << std::endl;
      return false;
    }

    unsigned char* content=new unsigned char[fileSize];

    if (fread(content,1,fileSize,file)!=fileSize) {
      std::cerr << "Cannot load file '" << typeFile << "'" << std::endl;
      delete [] content;
      fclose(file);
      return false;
    }

    fclose(file);

    ost::Scanner *scanner=new ost::Scanner(content,fileSize);
    ost::Parser  *parser=new ost::Parser(scanner,config);

    delete [] content;

    parser->Parse();

    success=!parser->errors->hasErrors;

    delete parser;
    delete scanner;

    return success;
  }
}
