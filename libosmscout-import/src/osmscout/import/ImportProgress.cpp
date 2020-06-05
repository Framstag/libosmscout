/*
  This source is part of the libosmscout library
  Copyright (C) 2020  Lukas Karas

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

#include <osmscout/import/ImportProgress.h>
#include <osmscout/util/String.h>
#include <osmscout/util/File.h>

namespace osmscout {

void ImportProgress::StartImport(const ImportParameter &)
{

}

void ImportProgress::FinishedImport()
{

}

void ImportProgress::DumpModuleDescription(const ImportModuleDescription& description)
{
  auto listFiles = [this](const std::string &msg, const std::list<std::string> &files){
    for (const auto& filename : files) {
      Info(msg + " '" + filename + "'");
    }
  };

  listFiles("Module requires file", description.GetRequiredFiles());
  listFiles("Module provides file", description.GetProvidedFiles());
  listFiles("Module provides optional file", description.GetProvidedOptionalFiles());
  listFiles("Module provides debugging file", description.GetProvidedDebuggingFiles());
  listFiles("Module provides temporary file", description.GetProvidedTemporaryFiles());
  listFiles("Module provides analysis file", description.GetProvidedAnalysisFiles());
}

void ImportProgress::StartModule(size_t currentStep, const ImportModuleDescription& moduleDescription)
{
  SetStep("Step #" +
          std::to_string(currentStep) +
          " - " +
          moduleDescription.GetName());
  Info("Module description: "+moduleDescription.GetDescription());

  DumpModuleDescription(moduleDescription);
}

void ImportProgress::FinishedModule()
{

}

void StatImportProgress::StartImport(const ImportParameter &param)
{
  destinationDirectory=param.GetDestinationDirectory();
  overAllTimer=StopClock();
  monitor.Reset();
  maxVMUsage=0.0;
  maxResidentSet=0.0;
  moduleStats.clear();
}

void StatImportProgress::FinishedImport()
{
  overAllTimer.Stop();

  if (maxVMUsage!=0.0 || maxResidentSet!=0.0) {
    Info(std::string("Overall ")+overAllTimer.ResultString()+"s, RSS "+ByteSizeToString(maxResidentSet)+", VM "+ByteSizeToString(maxVMUsage));
  }
  else {
    Info(std::string("Overall ")+overAllTimer.ResultString()+"s");
  }
}

void StatImportProgress::StartModule(size_t currentStep, const ImportModuleDescription& moduleDescription)
{
  ImportProgress::StartModule(currentStep, moduleDescription);
  timer=StopClock();
  currentModule=moduleDescription;
}

void StatImportProgress::FinishedModule()
{
  double vmUsage;
  double residentSet;

  timer.Stop();

  monitor.GetMaxValue(vmUsage,residentSet);
  monitor.Reset();

  maxVMUsage=std::max(maxVMUsage,vmUsage);
  maxResidentSet=std::max(maxResidentSet,residentSet);

  if (vmUsage!=0.0 || residentSet!=0.0) {
    Info(std::string("=> ")+timer.ResultString()+"s, RSS "+ByteSizeToString(residentSet)+", VM "+ByteSizeToString(vmUsage));
  }
  else {
    Info(std::string("=> ")+timer.ResultString()+"s");
  }

  moduleStats.emplace_back(ModuleStat{
    currentModule,
    timer.GetDuration(),
    vmUsage,
    residentSet});

  auto addFileStat = [this](const std::list<std::string> &files){
    for (const auto& filename : files) {
      std::string filePath = osmscout::AppendFileToDir(destinationDirectory, filename);
      osmscout::FileOffset fileSize = osmscout::GetFileSize(filePath);
      fileSizes[filename] = fileSize;
    }
  };

  addFileStat(currentModule.GetProvidedFiles());
  addFileStat(currentModule.GetProvidedAnalysisFiles());
  addFileStat(currentModule.GetProvidedDebuggingFiles());
  addFileStat(currentModule.GetProvidedOptionalFiles());
  addFileStat(currentModule.GetProvidedTemporaryFiles());
}

}
