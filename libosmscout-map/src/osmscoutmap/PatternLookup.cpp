/*
  This source is part of the libosmscout-map library
  Copyright (C) 2026  Tim Teulings

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

#include <osmscoutmap/PatternLookup.h>

#include <filesystem>
#include <list>
#include <string>
#include <system_error>

namespace osmscout {

  namespace {

    /**
     * File the lookup would read for the pattern in the given directory. The separator is the
     * native one, so that the file can be handed to the platform file APIs of the backends.
     */
    std::string GetCandidateFile(const std::string& directory,
                                 const std::string& patternName,
                                 const std::string& extension)
    {
      return (std::filesystem::path(directory) /
              (patternName+extension)).string();
    }
  }

  PatternLookup::Status PatternLookup::Resolve(const std::list<std::string>& patternPaths,
                                               const std::string& patternName,
                                               const std::string& extension,
                                               std::string& filename)
  {
    filename.clear();

    if (patternPaths.empty()) {
      return Status::NoSourceConfigured;
    }

    for (const auto& path : patternPaths) {
      std::string candidate=GetCandidateFile(path,
                                             patternName,
                                             extension);
      std::error_code error;

      if (std::filesystem::exists(candidate,error) &&
          std::filesystem::is_regular_file(candidate,error)) {
        filename=candidate;

        return Status::Found;
      }
    }

    return Status::NotFound;
  }

  std::string PatternLookup::Describe(const std::list<std::string>& patternPaths,
                                      const std::string& patternName,
                                      const std::string& extension,
                                      Status status)
  {
    switch (status) {
    case Status::Found:
      return "";
    case Status::NoSourceConfigured:
      return "No pattern image source is configured, pattern '" + patternName + "' cannot be resolved";
    case Status::NotFound:
      break;
    }

    std::string message="Pattern image '" + patternName + extension + "' not found";

    if (patternPaths.empty()) {
      message+=", no pattern image source is configured";
    }
    else {
      message+=" in the configured pattern directories";

      for (const auto& path : patternPaths) {
        message+=" '"+path+"'";
      }
    }

    return message;
  }
} // namespace osmscout
