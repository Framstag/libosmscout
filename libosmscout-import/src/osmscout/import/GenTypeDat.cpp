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

#include <osmscout/import/GenTypeDat.h>

namespace osmscout {

  void TypeDataGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                  ImportModuleDescription& description) const
  {
    description.SetName("TypeDataGenerator");
    description.SetDescription("Dump *.ost information into data file");

    description.AddProvidedFile(TypeConfig::FILE_TYPES_DAT);
  }

  bool TypeDataGenerator::Import(const TypeConfigRef& typeConfig,
                                 const ImportParameter& parameter,
                                 Progress& progress)
  {
    progress.SetAction("Generate types.dat");

    progress.Info("Number of types: "+std::to_string(typeConfig->GetTypes().size()));

    if (!typeConfig->StoreToDataFile(parameter.GetDestinationDirectory())) {
      progress.Error(std::string("Cannot create file '")+TypeConfig::FILE_TYPES_DAT+"'");
      return false;
    }

    return true;
  }
}

