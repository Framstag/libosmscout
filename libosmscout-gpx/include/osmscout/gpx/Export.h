#ifndef LIBOSMSCOUT_GPX_EXPORT_H
#define LIBOSMSCOUT_GPX_EXPORT_H

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
#include <osmscout/gpx/Import.h>
#include <osmscout/private/GPXImportExport.h>

#include <osmscout/util/File.h>
#include <osmscout/util/Exception.h>
#include <osmscout/util/Breaker.h>

#include <libxml/parser.h>

#include <cstdio>
#include <string>

namespace osmscout {
namespace gpx {

extern OSMSCOUT_GPX_API bool ExportGpx(const GpxFile &gpxFile,
                                       const std::string &filePath,
                                       BreakerRef breaker = NULL,
                                       ProcessCallbackRef callback = std::make_shared<ProcessCallback>());
}
}

#endif //LIBOSMSCOUT_GPX_EXPORT_H
