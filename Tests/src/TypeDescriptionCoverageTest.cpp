/*
  TypeDescriptionCoverageTest - a test program for libosmscout
  Copyright (C) 2026  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * Every type and every feature of the shipped type configuration is either described in both
 * shipped languages or listed, with a reason, in Tests/data/TypeDescriptionOmissions.txt.
 *
 * The check reads the type definition files as source, because a parked definition is not part of
 * a loaded type configuration at all: a definition commented out below a PARKED-NEW-TYPE marker is
 * retained in the repository but is invisible to TypeConfig. A source-level check is the only one
 * that sees the complete set, and it can report the file and the line of a gap.
 *
 * The rule this test enforces is section 6 of guidelines/TypeDescriptions.md: a description is
 * left out deliberately when the user-facing name of a type cannot be established, and the
 * omission list is the reviewed record of those decisions.
 */
namespace {

  /** A line of a type definition file, with a leading comment marker removed. */
  struct Line
  {
    std::string text;         //!< the line without a leading '//' marker and without leading space
    size_t      indent{0};    //!< the column the text starts at
    bool        commented{false}; //!< true if the line carried a '//' comment marker
    bool        empty{true};
  };

  /** A type or feature definition with the descriptions it carries. */
  struct Definition
  {
    std::string file;
    size_t      line{0};
    std::string name;
    bool        parked{false};
    bool        hasDesc{false};
    bool        hasEn{false};
    bool        hasDe{false};
  };

  std::string Location(const Definition& definition)
  {
    std::ostringstream text;

    text << definition.name << " (" << definition.file << ":" << definition.line
         << (definition.parked ? ", parked" : "") << ")";

    return text.str();
  }

  std::string Describe(const Definition& definition,
                       const std::string& what)
  {
    return Location(definition)+what;
  }

  const char* const typeDefinitionFiles[]{
    "map.ost",
    "motorways.ost",
    "basemap.ost",
    "contour_lines.ost",
  };

  Line ParseLine(const std::string& raw)
  {
    Line   line;
    size_t pos=raw.find_first_not_of(' ');

    if (pos==std::string::npos) {
      return line;
    }

    if (raw.compare(pos,2,"//")==0) {
      line.commented=true;

      pos=raw.find_first_not_of(' ',pos+2);

      if (pos==std::string::npos) {
        return line;
      }
    }

    line.text=raw.substr(pos);
    line.indent=pos;
    line.empty=false;

    return line;
  }

  std::vector<Definition> ReadDefinitions(const std::string& path)
  {
    const std::string fileName=std::filesystem::path(path).filename().string();

    std::ifstream     file(path);

    if (!file.is_open()) {
      throw std::runtime_error("Cannot read type definition file "+path);
    }

    std::vector<Definition> definitions;

    Definition              current;
    bool                    inDefinition=false;
    size_t                  descIndent=0;

    std::string             raw;
    size_t                  lineNo=0;

    while (std::getline(file,raw)) {
      lineNo++;

      Line line=ParseLine(raw);

      if (line.empty) {
        continue;
      }

      std::istringstream words(line.text);

      std::string        keyword;
      std::string        name;

      words >> keyword;
      words >> name;

      if (keyword=="TYPE" || keyword=="FEATURE") {
        if (inDefinition) {
          definitions.push_back(current);
        }

        current=Definition();

        current.file=fileName;
        current.line=lineNo;
        current.name=name;
        current.parked=line.commented;
        inDefinition=true;

        continue;
      }

      if (!inDefinition) {
        continue;
      }

      if (line.text=="DESC") {
        current.hasDesc=true;
        descIndent=line.indent;

        continue;
      }

      if (current.hasDesc && line.indent>descIndent) {
        if (line.text.starts_with("en:")) {
          current.hasEn=true;
        }

        if (line.text.starts_with("de:")) {
          current.hasDe=true;
        }
      }
    }

    if (inDefinition) {
      definitions.push_back(current);
    }

    return definitions;
  }

  std::vector<Definition> ReadShippedDefinitions(const std::string& topDir)
  {
    std::vector<Definition> definitions;

    for (const auto* fileName : typeDefinitionFiles) {
      const std::vector<Definition> fileDefinitions=ReadDefinitions(topDir+"/../stylesheets/"+fileName);

      definitions.insert(definitions.end(),
                         fileDefinitions.begin(),
                         fileDefinitions.end());
    }

    return definitions;
  }

  /** The names of the omission list, in file order. */
  std::vector<std::string> ReadOmissions(const std::string& path)
  {
    std::ifstream file(path);

    if (!file.is_open()) {
      std::string message="Cannot read the omission list ";

      message+=path;

      throw std::runtime_error(message);
    }

    std::vector<std::string> names;

    std::string              line;

    while (std::getline(file,line)) {
      if (line.empty() || line[0]=='#') {
        continue;
      }

      size_t end=line.find('\t');

      if (end==std::string::npos || end==0 || line.substr(end+1).empty()) {
        std::string message="An entry of ";

        message+=path;
        message+=" is not '<type-name><TAB><reason>': '";
        message+=line;
        message+="'";

        throw std::runtime_error(message);
      }

      names.push_back(line.substr(0,end));
    }

    return names;
  }
}

TEST_CASE("Every shipped type and feature is described or deliberately omitted", "[TypeDescription]")
{
  const char *topDir=std::getenv("TESTS_TOP_DIR");

  if (topDir==nullptr || *topDir=='\0') {
    SKIP("TESTS_TOP_DIR is not set, cannot locate the type definition files");
  }

  std::vector<Definition>  definitions;
  std::vector<std::string> omissions;

  try {
    definitions=ReadShippedDefinitions(topDir);
    omissions=ReadOmissions(std::string(topDir)+"/data/TypeDescriptionOmissions.txt");
  }
  catch (const std::exception& e) {
    FAIL(e.what());
  }

  REQUIRE(!definitions.empty());

  const std::set<std::string> omitted(omissions.begin(),
                                      omissions.end());

  for (const auto& definition : definitions) {
    if (omitted.count(definition.name)!=0) {
      continue;
    }

    if (!definition.hasDesc) {
      FAIL_CHECK(Describe(definition," carries no DESC block"));
    }

    if (!definition.hasEn) {
      FAIL_CHECK(Describe(definition," carries no English description"));
    }

    if (!definition.hasDe) {
      FAIL_CHECK(Describe(definition," carries no German description"));
    }
  }
}

TEST_CASE("The omission list names only definitions that are left undescribed", "[TypeDescription]")
{
  const char *topDir=std::getenv("TESTS_TOP_DIR");

  if (topDir==nullptr || *topDir=='\0') {
    SKIP("TESTS_TOP_DIR is not set, cannot locate the type definition files");
  }

  std::vector<Definition>  definitions;
  std::vector<std::string> omissions;

  try {
    definitions=ReadShippedDefinitions(topDir);
    omissions=ReadOmissions(std::string(topDir)+"/data/TypeDescriptionOmissions.txt");
  }
  catch (const std::exception& e) {
    FAIL(e.what());
  }

  std::set<std::string> seen;

  for (const auto& name : omissions) {
    if (!seen.insert(name).second) {
      FAIL_CHECK(name+" is listed more than once in the omission list");
    }

    const auto definition=std::find_if(definitions.begin(),
                                       definitions.end(),
                                       [&name](const Definition& definition) {
                                         return definition.name==name;
                                       });

    if (definition==definitions.end()) {
      FAIL_CHECK(name+" is listed in the omission list but is not defined by the shipped type configuration");

      continue;
    }

    for (const auto& candidate : definitions) {
      if (candidate.name!=name) {
        continue;
      }

      if (candidate.hasEn || candidate.hasDe) {
        FAIL_CHECK(Describe(candidate," is listed in the omission list but carries a description"));
      }
    }
  }
}
