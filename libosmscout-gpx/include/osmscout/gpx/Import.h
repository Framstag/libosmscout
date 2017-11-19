#ifndef LIBOSMSCOUT_GPX_IMPORT_H
#define LIBOSMSCOUT_GPX_IMPORT_H

/*
  This source is part of the libosmscout-gpx library
  Copyright (C) 2017 Lukas Karas

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

#include <osmscout/gpx/GpxFile.h>

#include <osmscout/private/GPXImportExport.h>

#include <string>
#include <osmscout/util/Breaker.h>

namespace osmscout {

class OSMSCOUT_GPX_API ImportCallback {
public:
  /**
   * Callback for reporting import progress
   * @param p - progress in range 0.0 .. 1.0
   */
  virtual void progress(double p);

  /**
   * Error while importing gpx
   * @param error
   */
  virtual void error(std::string error);
};

typedef std::shared_ptr<ImportCallback> ImportCallbackRef;

class OSMSCOUT_GPX_API Import {
public:
  static bool ImportGpx(const std::string &filePath,
                        GpxFile &output,
                        BreakerRef breaker=NULL,
                        ImportCallbackRef callback=std::make_shared<ImportCallback>());
};
}


#endif //LIBOSMSCOUT_GPX_IMPORT_H
