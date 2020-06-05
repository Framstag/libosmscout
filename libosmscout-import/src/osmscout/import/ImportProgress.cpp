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

namespace osmscout {

void ImportProgress::StartImport()
{

}

void ImportProgress::FinishedImport()
{

}

void ImportProgress::DumpModuleDescription(const ImportModuleDescription& description)
{
  for (const auto& filename : description.GetRequiredFiles()) {
    Info("Module requires file '"+filename+"'");
  }
  for (const auto& filename : description.GetProvidedFiles()) {
    Info("Module provides file '"+filename+"'");
  }
  for (const auto& filename : description.GetProvidedOptionalFiles()) {
    Info("Module provides optional file '"+filename+"'");
  }
  for (const auto& filename : description.GetProvidedDebuggingFiles()) {
    Info("Module provides debugging file '"+filename+"'");
  }
  for (const auto& filename : description.GetProvidedTemporaryFiles()) {
    Info("Module provides temporary file '"+filename+"'");
  }
  for (const auto& filename : description.GetProvidedAnalysisFiles()) {
    Info("Module provides analysis file '"+filename+"'");
  }
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

void StatImportProgress::StartImport()
{
  overAllTimer=StopClock();
  monitor.Reset();
  maxVMUsage=0.0;
  maxResidentSet=0.0;
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
}

}
