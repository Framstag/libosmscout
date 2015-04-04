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

#include <set>
#include <string>

#include <osmscout/TypeSet.h>

#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Magnification.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  /**
   * \ingroup Database
   */
  class OSMSCOUT_API OptimizeAreasLowZoom : public Referencable
  {
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
    TypeConfigRef                         typeConfig;    //!< Metadata information for loading the actual obejcts
    std::string                           datafile;      //!< Basename part for the data file name
    std::string                           datafilename;  //!< complete filename for data file
    mutable FileScanner                   scanner;       //!< File stream to the data file

    double                                magnification; //!< Magnification, up to which we support optimization
    std::map<TypeId,std::list<TypeData> > areaTypesData; //!< Index information for all area types

  private:
    bool ReadTypeData(FileScanner& scanner,
                      TypeData& data);

    bool GetOffsets(const TypeData& typeData,
                    double minlon,
                    double minlat,
                    double maxlon,
                    double maxlat,
                    std::vector<FileOffset>& offsets) const;

  public:
    OptimizeAreasLowZoom();
    virtual ~OptimizeAreasLowZoom();

    bool Open(const TypeConfigRef& typeConfig,
              const std::string& path);
    bool Close();

    bool HasOptimizations(double magnification) const;

    bool GetAreas(double lonMin, double latMin,
                  double lonMax, double latMax,
                  const Magnification& magnification,
                  size_t maxAreaCount,
                  TypeSet& areaTypes,
                  std::vector<AreaRef>& areas) const;
  };

  typedef Ref<OptimizeAreasLowZoom> OptimizeAreasLowZoomRef;
}

#endif
