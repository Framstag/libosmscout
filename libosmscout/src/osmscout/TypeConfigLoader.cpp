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

#include <osmscout/ost/Parser.h>
#include <osmscout/ost/Scanner.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>

#include <iostream>
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

    if (fread(content,1,fileSize,file)!=(size_t)fileSize) {
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

  bool LoadTypeData(const std::string& path,
                    TypeConfig& config)
  {
    FileScanner scanner;

    if (!scanner.Open(AppendFileToDir(path,"types.dat"))) {
      std::cerr << "Cannot open file '" << scanner.GetFilename() << "'" << std::endl;
     return false;
    }

    uint32_t tagCount;

    if (!scanner.ReadNumber(tagCount)) {
      std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
      return false;
    }

    for (size_t i=1; i<=tagCount; i++) {
      TagId       id;
      std::string name;
      bool        internalOnly;

      if (!(scanner.ReadNumber(id) &&
            scanner.Read(name),
            scanner.Read(internalOnly))) {
        std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
      }

      TagInfo tagInfo(name,internalOnly);

      tagInfo.SetId(id);

      config.RestoreTagInfo(tagInfo);
    }

    uint32_t typeCount;

    if (!scanner.ReadNumber(typeCount)) {
      std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
      return false;
    }

    for (size_t i=1; i<=typeCount; i++) {
      TypeId      id;
      std::string name;
      bool        canBeNode;
      bool        canBeWay;
      bool        canBeArea;
      bool        canBeRelation;
      bool        canBeRoute;
      bool        canBeIndexed;
      bool        consumeChildren;

      if (!(scanner.ReadNumber(id) &&
            scanner.Read(name) &&
            scanner.Read(canBeNode) &&
            scanner.Read(canBeWay) &&
            scanner.Read(canBeArea) &&
            scanner.Read(canBeRelation) &&
            scanner.Read(canBeRoute) &&
            scanner.Read(canBeIndexed) &&
            scanner.Read(consumeChildren))) {

        std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
        return false;
      }

      TypeInfo typeInfo;

      typeInfo.SetId(id);

      typeInfo.SetType(name);

      typeInfo.CanBeNode(canBeNode);
      typeInfo.CanBeWay(canBeWay);
      typeInfo.CanBeArea(canBeArea);
      typeInfo.CanBeRelation(canBeRelation);
      typeInfo.CanBeRoute(canBeRoute);
      typeInfo.CanBeIndexed(canBeIndexed);
      typeInfo.SetConsumeChildren(consumeChildren);

      config.AddTypeInfo(typeInfo);
    }

    return !scanner.HasError() && scanner.Close();
  }
}
