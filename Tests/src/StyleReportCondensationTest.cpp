/*
  This source is part of the libosmscout library
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

#include <osmscout/db/Database.h>
#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/StyleError.h>

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/**
 * A stylesheet that references type names the database's type configuration does
 * not carry is reported in ONE line per parsed style file - the style file, the
 * exact number of distinct unresolved names and a bounded sample - instead of one
 * line per reference. A stale map install emitted thousands of such lines per
 * start, which drowned the on-device log.
 *
 * Every reference is still recorded as a finding with its own position, the load
 * outcome is unchanged, the finding stays a warning, and the complete distinct
 * name list stays obtainable (from the findings, and in the log with debug logging).
 *
 * Test data: a database directory (argv[1] or TESTS_TOP_DIR/data/testregion).
 */
namespace {

std::filesystem::path TestsTopDir()
{
  const char *topDir = std::getenv("TESTS_TOP_DIR");

  REQUIRE(topDir != nullptr);

  return topDir;
}

std::filesystem::path TestDataDir()
{
  return TestsTopDir() / "data" / "testregion";
}

/**
 * Captures every line the parser logs, with the level it was logged at, so a case
 * can assert both the number of lines and that the condensed report stays a warning.
 */
class CapturingLogger : public osmscout::Logger
{
public:
  struct Entry
  {
    Level       level;
    std::string text;
  };

  class CaptureDestination : public osmscout::Logger::Destination
  {
  private:
    CapturingLogger &owner;

  public:
    explicit CaptureDestination(CapturingLogger &owner)
      : owner(owner)
    {
      // no code
    }

    void Print(const std::string& value) override
    {
      owner.current+=value;
    }

    void Print(const std::string_view& value) override
    {
      owner.current.append(value);
    }

    void Print(const char* value) override
    {
      owner.current+=value;
    }

    void Print(bool value) override
    {
      owner.current+=(value ? "true" : "false");
    }

    void Print(short value) override
    {
      owner.current+=std::to_string(value);
    }

    void Print(unsigned short value) override
    {
      owner.current+=std::to_string(value);
    }

    void Print(int value) override
    {
      owner.current+=std::to_string(value);
    }

    void Print(unsigned int value) override
    {
      owner.current+=std::to_string(value);
    }

    void Print(long value) override
    {
      owner.current+=std::to_string(value);
    }

    void Print(unsigned long value) override
    {
      owner.current+=std::to_string(value);
    }

    void Print(long long value) override
    {
      owner.current+=std::to_string(value);
    }

    void Print(unsigned long long value) override
    {
      owner.current+=std::to_string(value);
    }

    void PrintLn() override
    {
      owner.entries.push_back(CapturingLogger::Entry{owner.currentLevel,owner.current});

      owner.current.clear();
    }
  };

public:
  CaptureDestination                destination;
  std::vector<Entry>                entries;
  std::string                       current;
  Level                             currentLevel=Level::DEBUG;

  CapturingLogger()
    : destination(*this)
  {
    // no code
  }

  /** The lines that carry the condensed report (a warning, one per style file). */
  std::vector<Entry> AggregateEntries() const
  {
    std::vector<Entry> result;

    for (const auto& entry : entries) {
      if (entry.level==Level::WARN &&
          entry.text.find("Unknown types in '")!=std::string::npos) {
        result.push_back(entry);
      }
    }

    return result;
  }

  /** The lines that carry the complete name list (debug logging). */
  std::vector<Entry> DetailEntries() const
  {
    std::vector<Entry> result;

    for (const auto& entry : entries) {
      if (entry.level==Level::DEBUG &&
          entry.text.find("Unknown types in '")!=std::string::npos) {
        result.push_back(entry);
      }
    }

    return result;
  }

protected:
  Line Log(Level level) override
  {
    currentLevel=level;

    return Line(destination);
  }
};

/**
 * A log the case owns: nothing reaches the console and every line stays readable.
 */
struct CapturedLog
{
  osmscout::Log                     log;
  std::shared_ptr<CapturingLogger>  logger=std::make_shared<CapturingLogger>();

  CapturedLog()
  {
    log.SetLogger(logger);
  }

  CapturingLogger& Capture() const
  {
    return *logger;
  }
};

struct TestDatabase
{
  osmscout::DatabaseRef    database;
  osmscout::TypeConfigRef  typeConfig;

  TestDatabase()
  {
    osmscout::DatabaseParameter parameter;

    database=std::make_shared<osmscout::Database>(parameter);
    REQUIRE(database->Open(TestDataDir().string()));

    typeConfig=database->GetTypeConfig();
    REQUIRE(typeConfig != nullptr);
  }
};

std::filesystem::path WriteStyleSheet(const std::string& name,
                                      const std::string& content)
{
  std::filesystem::path file=std::filesystem::temp_directory_path()/name;
  std::ofstream         stream(file);

  stream << content;
  stream.close();

  return file;
}

/**
 * One rule referencing [typeNames] once, e.g. "condensation_a" or "a, b, c".
 * A way rule needs no style-kind suffix, unlike a node rule ("NODE.TEXT"/
 * "NODE.ICON"); the type filter is what this case is about.
 */
std::string TypeFilterRule(const std::string& typeNames)
{
  return "STYLE\n  [TYPE "+typeNames+"] WAY { color: #ff0000ff; }\n\n";
}

/** One rule referencing a known way type: this file reports nothing. */
std::string ResolvableRule()
{
  return "STYLE\n  [TYPE _route] WAY { color: #ff0000ff; }\n\n";
}

std::string StyleSheet(const std::string& body)
{
  return "OSS\n\n"+body+"END\n";
}

bool LoadStyleSheet(const osmscout::TypeConfigRef& typeConfig,
                    const std::filesystem::path& styleSheet,
                    osmscout::StyleConfigRef& config,
                    osmscout::Log& log)
{
  config=std::make_shared<osmscout::StyleConfig>(typeConfig);

  bool success=config->Load(styleSheet.string(),nullptr,false,log);

  // A rejected load is the interesting case in several of the cases below, and the
  // reason is only visible in the recorded errors.
  if (!success) {
    for (const auto& error : config->GetErrors()) {
      fprintf(stderr,"%s(%d,%d): %s\n",
              styleSheet.filename().string().c_str(),
              error.GetLine(),
              error.GetColumn(),
              error.GetDescription().c_str());
    }
  }

  return success;
}

std::size_t CountOccurrences(const std::string& haystack,
                             const std::string& needle)
{
  std::size_t count=0;
  std::size_t pos=haystack.find(needle);

  while (pos!=std::string::npos) {
    count++;
    pos=haystack.find(needle,pos+needle.size());
  }

  return count;
}

}

TEST_CASE("Unresolved type names are reported once per style file")
{
  TestDatabase testDatabase;

  SECTION("Three names referenced five hundred times")
  {
    CapturedLog capturedLog;

    std::string body;

    for (std::size_t i=0; i<500; i++) {
      body+=TypeFilterRule("condensation_repeat_"+std::to_string(i%3));
    }

    auto styleSheet=WriteStyleSheet("osmscout_condensation_repeat.oss",
                                    StyleSheet(body));
    osmscout::StyleConfigRef config;

    REQUIRE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,config,capturedLog.log));

    auto reports=capturedLog.Capture().AggregateEntries();

    REQUIRE(reports.size()==1);
    REQUIRE(reports[0].text.find(": 3 (")!=std::string::npos);

    // Every reference keeps its own finding, even though only one line is logged.
    REQUIRE(config->GetWarnings().size()==500);

    std::filesystem::remove(styleSheet);
  }

  SECTION("One name referenced a thousand times")
  {
    CapturedLog capturedLog;

    std::string body;

    for (std::size_t i=0; i<1000; i++) {
      body+=TypeFilterRule("condensation_thousand");
    }

    auto styleSheet=WriteStyleSheet("osmscout_condensation_thousand.oss",
                                    StyleSheet(body));
    osmscout::StyleConfigRef config;

    REQUIRE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,config,capturedLog.log));

    auto reports=capturedLog.Capture().AggregateEntries();

    REQUIRE(reports.size()==1);
    REQUIRE(reports[0].text.find(": 1 (")!=std::string::npos);

    std::filesystem::remove(styleSheet);
  }

  SECTION("Fifty distinct names in one rule")
  {
    CapturedLog capturedLog;

    std::string names;

    for (std::size_t i=0; i<50; i++) {
      if (i>0) {
        names+=", ";
      }

      names+="condensation_fifty_"+std::to_string(i);
    }

    auto styleSheet=WriteStyleSheet("osmscout_condensation_fifty.oss",
                                    StyleSheet(TypeFilterRule(names)));
    osmscout::StyleConfigRef config;

    REQUIRE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,config,capturedLog.log));

    auto reports=capturedLog.Capture().AggregateEntries();

    REQUIRE(reports.size()==1);
    REQUIRE(reports[0].text.find(": 50 (")!=std::string::npos);

    std::filesystem::remove(styleSheet);
  }

  SECTION("A stylesheet whose names all resolve reports nothing")
  {
    CapturedLog capturedLog;

    auto styleSheet=WriteStyleSheet("osmscout_condensation_clean.oss",
                                    StyleSheet(ResolvableRule()));
    osmscout::StyleConfigRef config;

    REQUIRE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,config,capturedLog.log));

    REQUIRE(capturedLog.Capture().AggregateEntries().empty());
    REQUIRE(config->GetWarnings().empty());

    std::filesystem::remove(styleSheet);
  }

  SECTION("The sample is bounded and says how many names it left out")
  {
    CapturedLog capturedLog;

    std::string names;

    for (std::size_t i=0; i<200; i++) {
      if (i>0) {
        names+=", ";
      }

      names+="condensation_sample_"+std::to_string(i);
    }

    auto styleSheet=WriteStyleSheet("osmscout_condensation_sample.oss",
                                    StyleSheet(TypeFilterRule(names)));
    osmscout::StyleConfigRef config;

    REQUIRE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,config,capturedLog.log));

    auto reports=capturedLog.Capture().AggregateEntries();

    REQUIRE(reports.size()==1);

    const std::string& report=reports[0].text;

    // The exact count is always reported.
    REQUIRE(report.find(": 200 (")!=std::string::npos);

    // The sample is bounded and names the number it left out.
    REQUIRE(report.find(" more)")!=std::string::npos);

    std::size_t listed=CountOccurrences(report,"condensation_sample_");

    REQUIRE(listed>0);
    REQUIRE(listed<200);

    std::filesystem::remove(styleSheet);
  }

  SECTION("The sample order is deterministic")
  {
    CapturedLog capturedLog;

    std::string body;

    for (std::size_t i=0; i<12; i++) {
      body+=TypeFilterRule("condensation_order_"+std::to_string(i));
    }

    auto styleSheet=WriteStyleSheet("osmscout_condensation_order.oss",
                                    StyleSheet(body));

    osmscout::StyleConfigRef firstConfig;
    osmscout::StyleConfigRef secondConfig;

    REQUIRE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,firstConfig,capturedLog.log));
    REQUIRE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,secondConfig,capturedLog.log));

    auto reports=capturedLog.Capture().AggregateEntries();

    REQUIRE(reports.size()==2);
    REQUIRE(reports[0].text==reports[1].text);

    std::filesystem::remove(styleSheet);
  }

  SECTION("Both kinds of unresolved type are reported together")
  {
    CapturedLog capturedLog;

    std::string body="ORDER WAYS\n  GROUP condensation_way_type\n\n";

    body+=TypeFilterRule("condensation_node_type");

    auto styleSheet=WriteStyleSheet("osmscout_condensation_both.oss",
                                    StyleSheet(body));
    osmscout::StyleConfigRef config;

    REQUIRE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,config,capturedLog.log));

    auto reports=capturedLog.Capture().AggregateEntries();

    REQUIRE(reports.size()==1);
    REQUIRE(reports[0].text.find(": 2 (")!=std::string::npos);
    REQUIRE(reports[0].text.find("condensation_way_type")!=std::string::npos);
    REQUIRE(reports[0].text.find("condensation_node_type")!=std::string::npos);

    std::filesystem::remove(styleSheet);
  }

  SECTION("An included module reports its own line")
  {
    CapturedLog capturedLog;

    auto moduleSheet=WriteStyleSheet("osmscout_condensation_module.oss",
                                     StyleSheet(TypeFilterRule("condensation_module_type")));
    auto styleSheet=WriteStyleSheet("osmscout_condensation_module_user.oss",
                                    StyleSheet("MODULE \"osmscout_condensation_module\"\n\n"+
                                               ResolvableRule()));
    osmscout::StyleConfigRef config;

    REQUIRE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,config,capturedLog.log));

    auto reports=capturedLog.Capture().AggregateEntries();

    REQUIRE(reports.size()==1);
    REQUIRE(reports[0].text.find("osmscout_condensation_module.oss")!=std::string::npos);
    REQUIRE(reports[0].text.find(": 1 (")!=std::string::npos);

    std::filesystem::remove(styleSheet);
    std::filesystem::remove(moduleSheet);
  }
}

TEST_CASE("The condensed report preserves findings and the load outcome")
{
  TestDatabase testDatabase;

  SECTION("Three references of one name keep three findings with their positions")
  {
    CapturedLog capturedLog;

    std::string body;

    body+=TypeFilterRule("condensation_positions");
    body+=TypeFilterRule("condensation_positions");
    body+=TypeFilterRule("condensation_positions");

    auto styleSheet=WriteStyleSheet("osmscout_condensation_positions.oss",
                                    StyleSheet(body));
    osmscout::StyleConfigRef config;

    REQUIRE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,config,capturedLog.log));

    auto reports=capturedLog.Capture().AggregateEntries();

    REQUIRE(reports.size()==1);
    REQUIRE(reports[0].text.find(": 1 (")!=std::string::npos);

    const auto& warnings=config->GetWarnings();

    REQUIRE(warnings.size()==3);

    std::vector<int> lines;

    for (const auto& warning : warnings) {
      REQUIRE(warning.GetType()==osmscout::StyleError::Warning);

      lines.push_back(warning.GetLine());
    }

    REQUIRE(lines[0]<lines[1]);
    REQUIRE(lines[1]<lines[2]);

    std::filesystem::remove(styleSheet);
  }

  SECTION("A stylesheet with unresolved names still loads")
  {
    CapturedLog capturedLog;

    auto styleSheet=WriteStyleSheet("osmscout_condensation_adopted.oss",
                                    StyleSheet(TypeFilterRule("condensation_adopted")+
                                               ResolvableRule()));
    osmscout::StyleConfigRef config;

    REQUIRE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,config,capturedLog.log));
    REQUIRE(config->GetErrors().empty());
    REQUIRE(config->GetWarnings().size()==1);

    std::filesystem::remove(styleSheet);
  }

  SECTION("A stylesheet with a hard error still fails")
  {
    CapturedLog capturedLog;

    // Unresolved names plus a rule that never closes: the load must stay rejected.
    auto styleSheet=WriteStyleSheet("osmscout_condensation_harderror.oss",
                                    StyleSheet(TypeFilterRule("condensation_hard_error")+
                                               "STYLE\n  [TYPE _route] WAY { color: #ff0000ff;\n"));
    osmscout::StyleConfigRef config;

    REQUIRE_FALSE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,config,capturedLog.log));

    std::filesystem::remove(styleSheet);
  }

  SECTION("The condensed report is a warning")
  {
    CapturedLog capturedLog;

    auto styleSheet=WriteStyleSheet("osmscout_condensation_level.oss",
                                    StyleSheet(TypeFilterRule("condensation_level")));
    osmscout::StyleConfigRef config;

    REQUIRE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,config,capturedLog.log));

    auto reports=capturedLog.Capture().AggregateEntries();

    REQUIRE(reports.size()==1);
    REQUIRE(reports[0].level==osmscout::Logger::WARN);

    std::filesystem::remove(styleSheet);
  }
}

TEST_CASE("The complete unresolved name list stays obtainable")
{
  TestDatabase testDatabase;

  std::string body;

  for (std::size_t i=0; i<3; i++) {
    body+=TypeFilterRule("condensation_detail_"+std::to_string(i));
  }

  auto styleSheet=WriteStyleSheet("osmscout_condensation_detail.oss",
                                  StyleSheet(body));

  SECTION("Debug logging disabled: only the condensed line")
  {
    CapturedLog capturedLog;

    osmscout::StyleConfigRef config;

    REQUIRE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,config,capturedLog.log));

    REQUIRE(capturedLog.Capture().AggregateEntries().size()==1);
    REQUIRE(capturedLog.Capture().DetailEntries().empty());
  }

  SECTION("Debug logging enabled: every distinct name is logged as well")
  {
    CapturedLog capturedLog;

    capturedLog.log.Debug(true);

    osmscout::StyleConfigRef config;

    REQUIRE(LoadStyleSheet(testDatabase.typeConfig,styleSheet,config,capturedLog.log));

    auto reports=capturedLog.Capture().AggregateEntries();

    REQUIRE(reports.size()==1);
    REQUIRE(reports[0].level==osmscout::Logger::WARN);

    auto details=capturedLog.Capture().DetailEntries();

    REQUIRE(details.size()==1);

    for (std::size_t i=0; i<3; i++) {
      REQUIRE(details[0].text.find("condensation_detail_"+std::to_string(i))!=std::string::npos);
    }
  }

  std::filesystem::remove(styleSheet);
}

TEST_CASE("Consecutive loads report independently")
{
  TestDatabase testDatabase;

  CapturedLog capturedLog;

  auto unknownSheet=WriteStyleSheet("osmscout_condensation_consecutive.oss",
                                    StyleSheet(TypeFilterRule("condensation_consecutive")));
  auto cleanSheet=WriteStyleSheet("osmscout_condensation_consecutive_clean.oss",
                                  StyleSheet(ResolvableRule()));

  osmscout::StyleConfigRef firstConfig;
  osmscout::StyleConfigRef secondConfig;
  osmscout::StyleConfigRef thirdConfig;

  // The same name in two consecutive loads is reported by both.
  REQUIRE(LoadStyleSheet(testDatabase.typeConfig,unknownSheet,firstConfig,capturedLog.log));
  REQUIRE(LoadStyleSheet(testDatabase.typeConfig,unknownSheet,secondConfig,capturedLog.log));

  auto reports=capturedLog.Capture().AggregateEntries();

  REQUIRE(reports.size()==2);
  REQUIRE(reports[0].text.find(": 1 (")!=std::string::npos);
  REQUIRE(reports[1].text.find(": 1 (")!=std::string::npos);

  // A load whose names all resolve adds nothing, even after an unclean one.
  REQUIRE(LoadStyleSheet(testDatabase.typeConfig,cleanSheet,thirdConfig,capturedLog.log));

  REQUIRE(capturedLog.Capture().AggregateEntries().size()==2);

  std::filesystem::remove(unknownSheet);
  std::filesystem::remove(cleanSheet);
}
