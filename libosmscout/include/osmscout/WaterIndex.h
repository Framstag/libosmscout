#ifndef OSMSCOUT_WATERINDEX_H
#define OSMSCOUT_WATERINDEX_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <list>
#include <memory>
#include <string>
#include <vector>

#include <osmscout/GroundTile.h>
#include <osmscout/Types.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Magnification.h>

namespace osmscout {

  /**
   * \ingroup Database
   */
  class OSMSCOUT_API WaterIndex
  {
  private:
    struct Level
    {
      FileOffset                 offset;

      double                     cellWidth;
      double                     cellHeight;

      uint32_t                   cellXStart;
      uint32_t                   cellXEnd;
      uint32_t                   cellYStart;
      uint32_t                   cellYEnd;
      uint32_t                   cellXCount;
      uint32_t                   cellYCount;
    };

  private:
    std::string                filepart;       //!< name of the data file
    std::string                datafilename;   //!< Full path and name of the data file
    mutable FileScanner        scanner;        //!< Scanner instance for reading this file

    uint32_t                   waterIndexMinMag;
    uint32_t                   waterIndexMaxMag;
    std::vector<Level>         levels;

  private:

  public:
    WaterIndex();

    bool Open(const std::string& path);
    void Close();

    bool GetRegions(double minlon,
                    double minlat,
                    double maxlon,
                    double maxlat,
                    const Magnification& magnification,
                    std::list<GroundTile>& tiles) const;

    void DumpStatistics();
  };

  typedef std::shared_ptr<WaterIndex> WaterIndexRef;
}

#endif
