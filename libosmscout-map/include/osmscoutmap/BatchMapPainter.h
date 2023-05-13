#ifndef OSMSCOUT_MAP_BATCH_MAP_PAINTER_H
#define OSMSCOUT_MAP_BATCH_MAP_PAINTER_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2023  Tim Teulings

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

#include <vector>

#include <osmscoutmap/MapImportExport.h>

#include <osmscout/projection/Projection.h>

#include <osmscoutmap/MapData.h>
#include <osmscoutmap/MapPainter.h>
#include <osmscoutmap/MapParameter.h>

namespace osmscout {

  /**
   * \ingroup Renderer
   *
   * Batch renderer helps to render map based on multiple databases
   * - map data and corresponding MapPainter
   */
  template <class PainterType>
  class BatchMapPainter {
  protected:
    std::vector<MapDataRef> data;
    std::vector<PainterType> painters;

  protected:

    /**
     * Render bach of multiple databases, step by step (\see RenderSteps).
     * All painters should have initialised its (backend specific) state.
     *
     * @param projection
     * @param parameter
     * @return false on error, true otherwise
     */
    bool BatchPaintInternal(const Projection& projection,
                            const MapParameter& parameter)
    {
      bool success=true;
      for (auto step=static_cast<size_t>(osmscout::RenderSteps::FirstStep);
           step<=static_cast<size_t>(osmscout::RenderSteps::LastStep);
           ++step){

        for (size_t i=0;i<data.size(); ++i){
          const MapData &d=*(data[i]);
          auto renderStep=static_cast<RenderSteps>(step);
          if (!painters[i]->Draw(projection,
                                 parameter,
                                 d,
                                 renderStep,
                                 renderStep)) {
            success=false;
          }
        }
      }
      return success;
    }

  public:
    explicit BatchMapPainter(size_t expectedCount)
    {
      data.reserve(expectedCount);
      painters.reserve(expectedCount);
    }

    virtual ~BatchMapPainter() = default;

    void AddData(const MapDataRef &d, PainterType &painter)
    {
      data.push_back(d);
      painters.push_back(painter);
    }
  };

  /**
   * \defgroup Renderer Map rendering
   *
   * Classes and methods related to rendering of maps.
   */
}

#endif
