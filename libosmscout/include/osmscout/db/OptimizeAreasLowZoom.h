#ifndef OSMSCOUT_OPTIMIZEAREASLOWZOOM_H
#define OSMSCOUT_OPTIMIZEAREASLOWZOOM_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>

#include <osmscout/TypeInfoSet.h>

#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/io/FileScanner.h>

#include <osmscout/util/Magnification.h>

namespace osmscout {

  /**
   * \ingroup Database
   */
  class OSMSCOUT_API OptimizeAreasLowZoom
  {
  public:
    static const char* const FILE_AREASOPT_DAT;

  private:
    struct TypeData
    {
      uint32_t   optLevel;       //!< The display level this data was optimized for
      uint32_t   indexLevel;     //!< Magnification level of index

      uint32_t   cellXStart;
      uint32_t   cellXEnd;
      uint32_t   cellYStart;
      uint32_t   cellYEnd;

      FileOffset bitmapOffset;   //!< Position in file where the offset of the bitmap is written
      uint8_t    dataOffsetBytes;

      uint32_t   cellXCount;
      uint32_t   cellYCount;

      double     cellWidth;
      double     cellHeight;

      double     minLon;
      double     maxLon;
      double     minLat;
      double     maxLat;
    };

  private:
    TypeConfigRef                              typeConfig;    //!< Metadata information for loading the actual objects
    std::string                                datafilename;  //!< complete filename for data file
    mutable FileScanner                        scanner;       //!< File stream to the data file

    double                                     magnification; //!< Magnification, up to which we support optimization
    std::map<TypeInfoRef,std::list<TypeData> > areaTypesData; //!< Index information for all area types

    mutable std::mutex                         lookupMutex;

  private:
    void ReadTypeData(FileScanner& scanner,
                      TypeData& data);

    void GetOffsets(const TypeData& typeData,
                    const GeoBox& boundingBox,
                    std::set<FileOffset>& offsets) const;

    void LoadData(std::set<FileOffset>& offsets,
                  std::vector<AreaRef>& areas) const;

  public:
    OptimizeAreasLowZoom();
    virtual ~OptimizeAreasLowZoom();

    bool Open(const TypeConfigRef& typeConfig,
              const std::string& path,
              bool memoryMappedData);
    bool Close();

    bool HasOptimizations(double magnification) const;

    void GetTypes(const Magnification& magnification,
                  const TypeInfoSet& areaTypes,
                 TypeInfoSet& availableAreaTypes) const;
    bool GetAreas(const GeoBox& boundingBox,
                  const Magnification& magnification,
                  const TypeInfoSet& areaTypes,
                  std::vector<AreaRef>& areas,
                  TypeInfoSet& loadedAreaTypes) const;
  };

  using OptimizeAreasLowZoomRef = std::shared_ptr<OptimizeAreasLowZoom>;
}

#endif
