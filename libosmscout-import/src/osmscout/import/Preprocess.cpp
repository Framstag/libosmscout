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

#include <osmscout/import/Preprocess.h>

#include <osmscout/private/Config.h>

#if defined(HAVE_LIB_XML)
  #include <osmscout/import/PreprocessOSM.h>
#endif

#if defined(HAVE_LIB_PROTOBUF)
  #include <osmscout/import/PreprocessPBF.h>
#endif

namespace osmscout {

  std::string Preprocess::GetDescription() const
  {
    return "Preprocess";
  }

  bool Preprocess::Import(const ImportParameter& parameter,
                          Progress& progress,
                          const TypeConfig& typeConfig)
  {
#if defined(HAVE_LIB_XML)
    if (parameter.GetMapfile().length()>=4 &&
        parameter.GetMapfile().substr(parameter.GetMapfile().length()-4)==".osm")  {
      PreprocessOSM preprocess;

      return preprocess.Import(parameter,progress,typeConfig);
    }
#endif

#if defined(HAVE_LIB_PROTOBUF)
    if (parameter.GetMapfile().length()>=4 &&
             parameter.GetMapfile().substr(parameter.GetMapfile().length()-4)==".pbf") {
      PreprocessPBF preprocess;

      return preprocess.Import(parameter,progress,typeConfig);
#else
      progress.Error("Support for the PBF file format is not enabled!");
      return false;
#endif
    }

    progress.Error("Sorry, this file type is not yet supported!");
    return false;
  }
}

