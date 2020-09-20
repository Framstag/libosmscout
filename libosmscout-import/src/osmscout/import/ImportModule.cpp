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

#include <osmscout/import/ImportModule.h>

namespace osmscout {

void ImportModuleDescription::SetName(const std::string& name)
{
  this->name=name;
}

void ImportModuleDescription::SetDescription(const std::string& description)
{
  this->description=description;
}

void ImportModuleDescription::AddProvidedFile(const std::string& providedFile)
{
  providedFiles.push_back(providedFile);
}

void ImportModuleDescription::AddProvidedOptionalFile(const std::string& providedFile)
{
  providedOptionalFiles.push_back(providedFile);
}

void ImportModuleDescription::AddProvidedDebuggingFile(const std::string& providedFile)
{
  providedDebuggingFiles.push_back(providedFile);
}

void ImportModuleDescription::AddProvidedTemporaryFile(const std::string& providedFile)
{
  providedTemporaryFiles.push_back(providedFile);
}

void ImportModuleDescription::AddProvidedAnalysisFile(const std::string& providedFile)
{
  providedAnalysisFiles.push_back(providedFile);
}

void ImportModuleDescription::AddRequiredFile(const std::string& requiredFile)
{
  requiredFiles.push_back(requiredFile);
}

void ImportModule::GetDescription(const ImportParameter& /*parameter*/,
                                  ImportModuleDescription& /*description*/) const
{
  // no code
}

}
