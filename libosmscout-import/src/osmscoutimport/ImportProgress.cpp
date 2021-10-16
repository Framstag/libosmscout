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

#include <osmscoutimport/ImportProgress.h>
#include <osmscout/util/String.h>
#include <osmscout/util/File.h>

#include <algorithm>
#include <iomanip>

namespace osmscout {

void ImportProgress::StartImport(const ImportParameter& /*param*/)
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

std::ostream& operator<<(std::ostream& stream, const std::chrono::steady_clock::duration &d)
{
  using namespace std::chrono;
  double seconds=duration_cast<duration<double>>(d).count();

  stream.setf(std::ios::fixed);
  stream << std::setprecision(3) << seconds;

  return stream;
}

bool StatImportProgress::DumpDotStats(const std::string &filename)
{
  std::ofstream out;

  //out.imbue(std::locale::classic());
  out.open(filename.c_str(),
                   std::ios::out|std::ios::trunc);

  if (!out.is_open()) {
    Error("Cannot open '"+filename+"'");
    return false;
  }

  out << "// Use dot tool (Graphviz) for converting to image: " << std::endl
      << "// dot -T svg -o stats.svg stats.dot" << std::endl
      << "digraph Import {" << std::endl;

  auto fileToId = [](std::string filename) -> std::string {
    // we need string copy
    std::replace(filename.begin(), filename.end(), '.', '_');
    return filename;
  };

  auto addFile = [&](const std::string &filename, const std::string &color){
    auto size = fileSizes.find(filename);
    assert(size!=fileSizes.end());
    out << "  " << fileToId(filename) << " [color=\"#b2ab9c\"," << std::endl
        << "    fillcolor=\"" << color << "\"," << std::endl
        << "    style=filled,"
        << "    fontsize=12," << std::endl
        << "    height=1.0," << std::endl
        << "    label=<" << "<b>" << filename << "</b><br/>"
                         << ByteSizeToString(size->second)
                         << ">," << std::endl
        << "    shape=ellipse," << std::endl
        << "    width=3];" << std::endl;
  };

  auto addFiles = [&](const std::string &moduleName, const std::list<std::string> &files, const std::string &color){
    for (const auto &f: files) {
      addFile(f, color);
      // link to file
      out << "  " << moduleName << " -> " << fileToId(f) << std::endl;
    }
  };

  size_t currentStep=1;
  for (const auto &moduleStat: moduleStats){
    out << "  " << moduleStat.description.GetName() << " [color=\"#b2ab9c\"," << std::endl
        << "    fillcolor=\"#edecea\"," << std::endl
        << "    fontsize=14," << std::endl
        << "    height=1.1528," << std::endl
        << "    label=<" << "<b>Step #" << currentStep << " - " <<  moduleStat.description.GetName() << "</b><br/>"
                         << "<i>" << moduleStat.description.GetDescription() << "</i><br/>"
                         << moduleStat.duration << " s; " << ByteSizeToString(moduleStat.residentSet) << " RSS"
                         << ">," << std::endl
        << "    shape=box," << std::endl
        << "    width=3];" << std::endl;


    addFiles(moduleStat.description.GetName(), moduleStat.description.GetProvidedFiles(), "#a2ff94");
    addFiles(moduleStat.description.GetName(), moduleStat.description.GetProvidedOptionalFiles(), "#ccffbe");
    addFiles(moduleStat.description.GetName(), moduleStat.description.GetProvidedDebuggingFiles(), "#ffb4be");
    addFiles(moduleStat.description.GetName(), moduleStat.description.GetProvidedTemporaryFiles(), "#ffffac");
    addFiles(moduleStat.description.GetName(), moduleStat.description.GetProvidedAnalysisFiles(), "#ffaeb0");

    // required files
    for (const std::string &f : moduleStat.description.GetRequiredFiles()){
      out << "  " << fileToId(f) << " -> " << moduleStat.description.GetName() << std::endl;
    }

    currentStep++;
  }


  out << "}" << std::endl;

  out.close();

  return true;
}

}
