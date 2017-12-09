#ifndef OSMSCOUT_IMPORT_PREPROCESS_OSM_H
#define OSMSCOUT_IMPORT_PREPROCESS_OSM_H

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

#include <osmscout/import/Preprocessor.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class PreprocessOSM CLASS_FINAL : public Preprocessor
  {
  private:
    PreprocessorCallback& callback;

  public:
    explicit PreprocessOSM(PreprocessorCallback& callback);

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress,
                const std::string& filename);
  };
}

#endif
