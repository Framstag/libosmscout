/*
  This source is part of the libosmscout library
  Copyright (C) 2017 Tim Teulings

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

#include <osmscout/import/ShapeFileScanner.h>

#include <osmscout/util/Exception.h>

namespace osmscout {

  ShapeFileVisitor::~ShapeFileVisitor()
  {

  }

  void ShapeFileVisitor::OnFileBoundingBox(const GeoBox& /*boundingBox*/)
  {
    // no code
  }

  void ShapeFileVisitor::OnPolyline(int32_t /*recordNumber*/,
                                    const GeoBox& /*boundingBox*/,
                                    const std::vector<GeoCoord>& /*coords*/)
  {
    // no code
  }

  void ShapeFileVisitor::OnProgress(double /*current*/,
                                    double /*total*/)
  {

  }

  ShapeFileScanner::ShapeFileScanner(const std::string& filename)
    : filename(filename),
      file(nullptr)
  {
    // no code
  }

  ShapeFileScanner::~ShapeFileScanner()
  {
    Close();
  }

  void ShapeFileScanner::Open()
  {
    if (file!=nullptr) {
      throw IOException(filename,"Cannot open file","File is already open");
    }

    file=fopen(filename.c_str(),"rb");

    if (file==nullptr) {
      throw IOException(filename,"Cannot open file for reading");
    }
  }

  void ShapeFileScanner::Close()
  {
    if (file!=nullptr)
    {
      fclose(file);
      file=nullptr;
    }
  }

  uint8_t ShapeFileScanner::ReadByte()
  {
    uint8_t value;

    if (fread(&value,1,1,file)!=1) {
      throw IOException(filename,"Cannot read byte");
    }

    return value;
  }

  int32_t ShapeFileScanner::ReadIntegerBE()
  {
    uint8_t v1=ReadByte();
    uint8_t v2=ReadByte();
    uint8_t v3=ReadByte();
    uint8_t v4=ReadByte();

    return (int32_t)((((uint32_t)v1) << 24) | (((uint32_t)v2) << 16) | (((uint32_t)v3) << 8) | v4);
  }

  int32_t ShapeFileScanner::ReadIntegerLE()
  {
    uint8_t v1=ReadByte();
    uint8_t v2=ReadByte();
    uint8_t v3=ReadByte();
    uint8_t v4=ReadByte();

    return (int32_t)((((uint32_t)v4) << 24) | (((uint32_t)v3) << 16) | (((uint32_t)v2) << 8) | v1);
  }

  double ShapeFileScanner::ReadDoubleLE()
  {
    double         value;
    unsigned char* valueBytes=(unsigned char*)&value;

    valueBytes[0]=ReadByte();
    valueBytes[1]=ReadByte();
    valueBytes[2]=ReadByte();
    valueBytes[3]=ReadByte();
    valueBytes[4]=ReadByte();
    valueBytes[5]=ReadByte();
    valueBytes[6]=ReadByte();
    valueBytes[7]=ReadByte();

    return value;
  }

  void ShapeFileScanner::Visit(ShapeFileVisitor& visitor)
  {
    // Shape files has a maximum size of 31 bits, so no need to fiddle around
    // with large file support stuff

    if (file==nullptr) {
      throw IOException(filename,"Cannot visit file content","File not opened");
    }

    if (fseek(file,0,SEEK_SET)!=0) {
      throw IOException(filename,"Cannot go to start of file");
    }

    int32_t fileCode=ReadIntegerBE();

    if (fileCode!=9994) {
      throw IOException(filename,"Format error","Expected file code 9994, got "+std::to_string(fileCode));
    }

    // Skip reserved space
    for (size_t i=1; i<=5; i++) {
      /* ignore */ReadIntegerBE();
    }

    int32_t fileLength=ReadIntegerBE()*2; // fileLength is in 2 byte units
    int32_t version   =ReadIntegerLE();

    if (version!=1000) {
      throw IOException(filename,"Format error","Expected shape file version 1000, got "+std::to_string(version));
    }

    int32_t shapeType=ReadIntegerLE();

    if (shapeType!=3) {
      throw IOException(filename,"Not implemented","Shapes of type "+std::to_string(shapeType)+" are not supported");
    }

    double xMin=ReadDoubleLE();
    double yMin=ReadDoubleLE();
    double xMax=ReadDoubleLE();
    double yMax=ReadDoubleLE();

    visitor.OnFileBoundingBox(GeoBox(GeoCoord(yMin,xMin),
                                     GeoCoord(yMax,xMax)));

    double zMin=ReadDoubleLE();
    double zMax=ReadDoubleLE();
    double mMin=ReadDoubleLE();
    double mMax=ReadDoubleLE();

    unused(zMin);
    unused(zMax);
    unused(mMin);
    unused(mMax);

    long filePos=ftell(file);

    if (filePos==-1) {
      throw IOException(filename,"Error while retrieving current file position");
    }

    while (filePos+1<fileLength) {
      visitor.OnProgress(filePos,fileLength);

      int32_t recordNumber   =ReadIntegerBE();
      int32_t length         =ReadIntegerBE();
      int32_t recordShapeType=ReadIntegerLE();

      unused(length);

      if (recordShapeType==3) {
        if (recordShapeType!=shapeType) {
          throw IOException(filename,
                            "Error while reading shape file record",
                            "Shape file is type shape type "+std::to_string(shapeType)+
                            ", but shape type of record is "+std::to_string(recordShapeType));
        }

        double recordXMin=ReadDoubleLE();
        double recordXMax=ReadDoubleLE();
        double recordYMin=ReadDoubleLE();
        double recordYMax=ReadDoubleLE();

        GeoBox recordBoundingBox(GeoCoord(recordYMin,recordXMin),
                                 GeoCoord(recordYMax,recordXMax));

        int32_t numParts =ReadIntegerLE();
        int32_t numPoints=ReadIntegerLE();

        for (int32_t i=1; i<=numParts; i++) {
          int32_t startIndex=ReadIntegerLE();

          // We current do not evaluate start index
          unused(startIndex);
        }

        buffer.clear();
        buffer.reserve((size_t)numPoints);

        for (int32_t i=1; i<=numPoints; i++) {
          double x=ReadDoubleLE();
          double y=ReadDoubleLE();

          buffer.emplace_back(y,x);
        }

        visitor.OnPolyline(recordNumber,
                           recordBoundingBox,
                           buffer);
      }
      else {
        throw IOException(filename,"Not implemented","Shapes of type "+std::to_string(shapeType)+" not supported");
      }

      filePos=ftell(file);

      if (filePos==-1) {
        throw IOException(filename,"Error while retrieving current file position");
      }
    }
  }
}
