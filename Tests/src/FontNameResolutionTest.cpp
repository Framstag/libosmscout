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

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <osmscout/system/Compiler.h>

#include <osmscoutmap/FontNameResolution.h>

namespace {

  std::string GetEnv(const char* name,
                     const std::string& fallback)
  {
    const char * value=std::getenv(name);

    return value!=nullptr ? std::string(value) : fallback;
  }

  std::filesystem::path TestsTopDir()
  {
    return std::filesystem::path(GetEnv("TESTS_TOP_DIR",
                                        ".."));
  }

  /**
   * The font file the repository ships, which the font-dependent tests measure.
   */
  std::filesystem::path RepositoryFontFile()
  {
    return TestsTopDir() / ".." / "libosmscout-map-opengl" / "data" / "fonts" / "LiberationSans-Regular.ttf";
  }

  /**
   * The family the repository font file declares. Asserted rather than read, so that a
   * resolution that names the wrong face is caught.
   */
  const char* RepositoryFontFamily="Liberation Sans";

  /**
   * Removes a file written by a test however the test ends.
   */
  class TempFile CLASS_FINAL
  {
  private:
    std::filesystem::path path;

  public:
    explicit TempFile(const std::filesystem::path& path)
    : path(path)
    {
      // no code
    }

    ~TempFile()
    {
      std::error_code error;

      std::filesystem::remove(path,
                              error);
    }

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;

    const std::filesystem::path& Path() const
    {
      return path;
    }
  };

  /**
   * A file that exists but holds no font.
   */
  TempFile WriteNotAFontFile()
  {
    std::filesystem::path directory=std::filesystem::path(GetEnv("TESTS_TMP_DIR",
                                                                 std::filesystem::temp_directory_path().string()));

    std::error_code error;

    std::filesystem::create_directories(directory,
                                        error);

    std::filesystem::path file=directory / "FontNameResolutionTest-not-a-font.bin";

    std::ofstream stream(file,
                         std::ios::binary | std::ios::trunc);

    stream << "this file holds no font";
    stream.close();

    return TempFile(file);
  }
}

TEST_CASE("A font name that names no file is passed on unchanged", "[FontNameResolution]")
{
  osmscout::FontNameResolution::Result result=osmscout::FontNameResolution::Resolve("Liberation Sans");

  REQUIRE(result.fontName=="Liberation Sans");
  REQUIRE(result.fontFile.empty());

  result=osmscout::FontNameResolution::Resolve("");

  REQUIRE(result.fontName.empty());
  REQUIRE(result.fontFile.empty());
}

TEST_CASE("A configured font file is recognised as a file", "[FontNameResolution]")
{
  std::filesystem::path fontFile=RepositoryFontFile();

  if (!std::filesystem::is_regular_file(fontFile)) {
    SKIP("Cannot locate the repository font file '" << fontFile.string() << "'");
  }

  std::filesystem::path directory=TestsTopDir();

  REQUIRE(std::filesystem::is_directory(directory));

  // A directory is not a font file
  osmscout::FontNameResolution::Result directoryResult=osmscout::FontNameResolution::Resolve(directory.string());

  REQUIRE(directoryResult.fontFile.empty());
  REQUIRE(directoryResult.fontName==directory.string());

  osmscout::FontNameResolution::Result result=osmscout::FontNameResolution::Resolve(fontFile.string());

  REQUIRE(result.fontFile==fontFile.string());
  REQUIRE_FALSE(result.fontName.empty());

  // A path that does not exist is not a font file either
  result=osmscout::FontNameResolution::Resolve("/no/such/directory/font.ttf");

  REQUIRE(result.fontFile.empty());
  REQUIRE(result.fontName=="/no/such/directory/font.ttf");
}

TEST_CASE("The family of a configured font file is read out of the file", "[FontNameResolution]")
{
  std::filesystem::path fontFile=RepositoryFontFile();

  if (!std::filesystem::is_regular_file(fontFile)) {
    SKIP("Cannot locate the repository font file '" << fontFile.string() << "'");
  }

  if (!osmscout::FontNameResolution::CanReadFamilyFromFile()) {
    // This build cannot read a font file: the configured name stays, and the file is still
    // reported as the face to serve
    osmscout::FontNameResolution::Result result=osmscout::FontNameResolution::Resolve(fontFile.string());

    REQUIRE(result.fontName==fontFile.string());
    REQUIRE(result.fontFile==fontFile.string());

    std::string family;
    std::string error;

    REQUIRE_FALSE(osmscout::FontNameResolution::ReadFamilyFromFile(fontFile.string(),
                                                                  family,
                                                                  error));
    REQUIRE(family.empty());
    REQUIRE_FALSE(error.empty());

    SKIP("This build cannot read the family out of a font file");
  }

  osmscout::FontNameResolution::Result result=osmscout::FontNameResolution::Resolve(fontFile.string());

  REQUIRE(result.fontFile==fontFile.string());
  REQUIRE(result.fontName==RepositoryFontFamily);

  std::string family;
  std::string error;

  REQUIRE(osmscout::FontNameResolution::ReadFamilyFromFile(fontFile.string(),
                                                          family,
                                                          error));
  REQUIRE(family==RepositoryFontFamily);
  REQUIRE(error.empty());
}

TEST_CASE("A file that cannot be read as a font is reported", "[FontNameResolution]")
{
  if (!osmscout::FontNameResolution::CanReadFamilyFromFile()) {
    SKIP("This build cannot read a font file");
  }

  TempFile file=WriteNotAFontFile();

  REQUIRE(std::filesystem::is_regular_file(file.Path()));

  std::string family;
  std::string error;

  REQUIRE_FALSE(osmscout::FontNameResolution::ReadFamilyFromFile(file.Path().string(),
                                                                family,
                                                                error));
  REQUIRE(family.empty());
  REQUIRE_FALSE(error.empty());

  // The file is still the face the configuration names; no other face is named in its place
  osmscout::FontNameResolution::Result result=osmscout::FontNameResolution::Resolve(file.Path().string());

  REQUIRE(result.fontFile==file.Path().string());
  REQUIRE(result.fontName==file.Path().string());
}
