#ifndef OSMSCOUT_IMPORT_SHAPEFILESCANNER_H
#define OSMSCOUT_IMPORT_SHAPEFILESCANNER_H

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

#include <cstdio>
#include <string>
#include <vector>

#include <osmscout/private/ImportImportExport.h>

#include <osmscout/util/GeoBox.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * Visitor for scanner contents of a shape file.
   *
   * A (derived) visitor instance is passed to the ShapeFileScanner
   * and repeately called on corresponding data.
   */
  class OSMSCOUT_IMPORT_API ShapeFileVisitor
  {
  public:
    virtual ~ShapeFileVisitor();
    virtual void OnFileBoundingBox(const GeoBox& boundingBox);
    virtual void OnProgress(double current, double total);
    virtual void OnPolyline(int32_t recordNumber,
                            const GeoBox& boundingBox,
                            const std::vector<GeoCoord>& coords);
  };

  /**
   * Class for reading shape files
   */
  class OSMSCOUT_IMPORT_API ShapeFileScanner CLASS_FINAL
  {
  private:
    std::string           filename;
    FILE                  *file;
    std::vector<GeoCoord> buffer;

  private:
    uint8_t ReadByte();
    int32_t ReadIntegerBE();
    int32_t ReadIntegerLE();
    double ReadDoubleLE();

  public:
    ShapeFileScanner(const std::string& filename);
    ~ShapeFileScanner();

    void Open();
    void Close();

    void Visit(ShapeFileVisitor& visitor);
  };
}

#endif
