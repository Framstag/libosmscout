/*
  This source is part of the libosmscout-map library
  Copyright (C) 2025  Tim Teulings

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
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <core/SkBitmap.h>
#include <core/SkData.h>
#include <core/SkImage.h>
#include <core/SkImageInfo.h>
#include <core/SkSurface.h>

#include <osmscout/io/File.h>
#include <osmscoutmapskia/MapPainterSkia.h>

// nanosvg headers (vendored in libosmscout-map-skia)
#define NANOSVG_IMPLEMENTATION
#include "../../../libosmscout-map-skia/src/osmscoutmapskia/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "../../../libosmscout-map-skia/src/osmscoutmapskia/nanosvgrast.h"

namespace {

// Create a minimal but valid SVG file
std::string CreateTestSVGContent() {
  return R"(<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="2" y="2" width="20" height="20" rx="3" fill="#3388ff"/>
  <circle cx="12" cy="12" r="6" fill="#ffffff"/>
</svg>)";
}

// Write content to a temp file, return path
std::string WriteTempFile(const std::string& content, const std::string& suffix) {
  char tmpDir[] = "/tmp/osmscout_test_XXXXXX";
  char* dir = mkdtemp(tmpDir);
  REQUIRE(dir != nullptr);

  std::string path = std::string(dir) + "/test" + suffix;
  std::ofstream file(path, std::ios::binary);
  file << content;
  file.close();
  return path;
}

// Clean up a temp file
void CleanupTempFile(const std::string& path) {
  std::filesystem::remove(path);
  std::filesystem::remove(std::filesystem::path(path).parent_path());
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// nanosvg parsing test
// ---------------------------------------------------------------------------
TEST_CASE("nanosvg parses valid SVG content", "[SVGIcon]") {
  std::string svgContent = CreateTestSVGContent();
  std::string svgPath = WriteTempFile(svgContent, ".svg");

  NSVGimage* image = nsvgParseFromFile(svgPath.c_str(), "px", 96.0f);
  REQUIRE(image != nullptr);
  REQUIRE(image->width > 0);
  REQUIRE(image->height > 0);

  nsvgDelete(image);
  CleanupTempFile(svgPath);
}

TEST_CASE("nanosvg returns null for invalid file path", "[SVGIcon]") {
  NSVGimage* image = nsvgParseFromFile("/nonexistent/path/test.svg", "px", 96.0f);
  REQUIRE(image == nullptr);
}

TEST_CASE("nanosvg handles invalid SVG content gracefully", "[SVGIcon]") {
  std::string svgPath = WriteTempFile("not valid svg content", ".svg");

  NSVGimage* image = nsvgParseFromFile(svgPath.c_str(), "px", 96.0f);
  // nanosvg is lenient and may return non-null even for invalid input;
  // the important thing is it doesn't crash
  if (image) {
    nsvgDelete(image);
  }

  CleanupTempFile(svgPath);
}

// ---------------------------------------------------------------------------
// nanosvg rasterization test
// ---------------------------------------------------------------------------
TEST_CASE("nanosvg rasterizes SVG to RGBA buffer", "[SVGIcon]") {
  std::string svgContent = CreateTestSVGContent();
  std::string svgPath = WriteTempFile(svgContent, ".svg");

  NSVGimage* svgImage = nsvgParseFromFile(svgPath.c_str(), "px", 96.0f);
  REQUIRE(svgImage != nullptr);

  int w = static_cast<int>(svgImage->width);
  int h = static_cast<int>(svgImage->height);
  REQUIRE(w > 0);
  REQUIRE(h > 0);

  unsigned char* rgba = static_cast<unsigned char*>(malloc(static_cast<size_t>(w) * h * 4));
  REQUIRE(rgba != nullptr);

  NSVGrasterizer* rast = nsvgCreateRasterizer();
  REQUIRE(rast != nullptr);

  nsvgRasterize(rast, svgImage, 0, 0, 1.0f, rgba, w, h, w * 4);

  // Verify non-transparent pixels exist (the rect and circle should produce some colored pixels)
  bool hasNonTransparent = false;
  for (int i = 0; i < w * h * 4; i += 4) {
    if (rgba[i + 3] > 0) { // alpha > 0
      hasNonTransparent = true;
      break;
    }
  }
  REQUIRE(hasNonTransparent);

  nsvgDeleteRasterizer(rast);
  nsvgDelete(svgImage);
  free(rgba);
  CleanupTempFile(svgPath);
}

// ---------------------------------------------------------------------------
// nanosvg → SkImage conversion test
// ---------------------------------------------------------------------------
TEST_CASE("nanosvg rasterizer output converts to valid SkImage", "[SVGIcon]") {
  std::string svgContent = CreateTestSVGContent();
  std::string svgPath = WriteTempFile(svgContent, ".svg");

  NSVGimage* svgImage = nsvgParseFromFile(svgPath.c_str(), "px", 96.0f);
  REQUIRE(svgImage != nullptr);

  int w = static_cast<int>(svgImage->width);
  int h = static_cast<int>(svgImage->height);

  unsigned char* rgba = static_cast<unsigned char*>(malloc(static_cast<size_t>(w) * h * 4));
  REQUIRE(rgba != nullptr);

  NSVGrasterizer* rast = nsvgCreateRasterizer();
  nsvgRasterize(rast, svgImage, 0, 0, 1.0f, rgba, w, h, w * 4);
  nsvgDeleteRasterizer(rast);
  nsvgDelete(svgImage);

  sk_sp<SkData> pixelData = SkData::MakeFromMalloc(rgba,
      static_cast<size_t>(w) * h * 4);
  SkImageInfo info = SkImageInfo::Make(w, h,
      kRGBA_8888_SkColorType, kPremul_SkAlphaType);
  sk_sp<SkImage> image = SkImages::RasterFromData(info, pixelData, w * 4);

  REQUIRE(image != nullptr);
  REQUIRE(image->width() == w);
  REQUIRE(image->height() == h);

  CleanupTempFile(svgPath);
}

// ---------------------------------------------------------------------------
// Full pipeline: SVG file → nanosvg → SkImage → draw to surface
// ---------------------------------------------------------------------------
TEST_CASE("SVG icon renders to SkSurface via nanosvg pipeline", "[SVGIcon]") {
  std::string svgContent = CreateTestSVGContent();
  std::string svgPath = WriteTempFile(svgContent, ".svg");

  // Parse SVG
  NSVGimage* svgImage = nsvgParseFromFile(svgPath.c_str(), "px", 96.0f);
  REQUIRE(svgImage != nullptr);

  int w = static_cast<int>(svgImage->width);
  int h = static_cast<int>(svgImage->height);

  // Rasterize
  unsigned char* rgba = static_cast<unsigned char*>(malloc(static_cast<size_t>(w) * h * 4));
  NSVGrasterizer* rast = nsvgCreateRasterizer();
  nsvgRasterize(rast, svgImage, 0, 0, 1.0f, rgba, w, h, w * 4);
  nsvgDeleteRasterizer(rast);
  nsvgDelete(svgImage);

  // Convert to SkImage
  sk_sp<SkData> pixelData = SkData::MakeFromMalloc(rgba, static_cast<size_t>(w) * h * 4);
  SkImageInfo info = SkImageInfo::Make(w, h, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
  sk_sp<SkImage> image = SkImages::RasterFromData(info, pixelData, w * 4);
  REQUIRE(image != nullptr);

  // Draw to surface
  auto surface = SkSurfaces::Raster(info);
  REQUIRE(surface != nullptr);
  surface->getCanvas()->clear(SK_ColorWHITE);

  SkRect destRect = SkRect::MakeXYWH(0, 0, (SkScalar)w, (SkScalar)h);
  SkRect srcRect = SkRect::MakeWH((SkScalar)w, (SkScalar)h);
  surface->getCanvas()->drawImageRect(image, srcRect, destRect,
                                       SkSamplingOptions(), nullptr,
                                       SkCanvas::kFast_SrcRectConstraint);

  // Verify pixels: read back and check non-white pixels exist (the SVG rendered something)
  SkBitmap bitmap;
  bitmap.allocPixels(info);
  surface->readPixels(bitmap, 0, 0);

  bool hasNonWhite = false;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      SkColor pixel = bitmap.getColor(x, y);
      if (pixel != SK_ColorWHITE) {
        hasNonWhite = true;
        break;
      }
    }
    if (hasNonWhite) break;
  }
  REQUIRE(hasNonWhite);

  CleanupTempFile(svgPath);
}

// ---------------------------------------------------------------------------
// Test HasIcon with actual SVG file via AppendFileToDir
// ---------------------------------------------------------------------------
TEST_CASE("AppendFileToDir produces correct path for SVG lookup", "[SVGIcon]") {
  std::string svgContent = CreateTestSVGContent();
  std::string svgPath = WriteTempFile(svgContent, ".svg");

  std::string dir = std::filesystem::path(svgPath).parent_path().string();
  std::string filename = std::filesystem::path(svgPath).filename().string();

  // Remove .svg suffix to get the icon name
  std::string iconName = filename.substr(0, filename.size() - 4);
  std::string expectedPath = dir + "/" + iconName + ".svg";

  std::string result = osmscout::AppendFileToDir(dir, iconName + ".svg");

  REQUIRE(result == expectedPath);
  REQUIRE(std::filesystem::exists(result));

  CleanupTempFile(svgPath);
}

// ---------------------------------------------------------------------------
// Verify that a real SVG icon file from the project loads correctly
// ---------------------------------------------------------------------------
TEST_CASE("Real project SVG icon loads via nanosvg", "[SVGIcon]") {
  // Look for the project's SVG icons directory relative to the test binary
  std::string iconDir;
  const char* topDir = std::getenv("TESTS_TOP_DIR");
  if (topDir) {
    iconDir = std::string(topDir) + "/libosmscout/data/icons/svg/standard";
  }

  if (iconDir.empty() || !std::filesystem::exists(iconDir)) {
    // Try common relative paths
    for (const auto& candidate : {"../libosmscout/data/icons/svg/standard",
                                   "../../libosmscout/data/icons/svg/standard",
                                   "../../../libosmscout/data/icons/svg/standard"}) {
      if (std::filesystem::exists(candidate)) {
        iconDir = std::filesystem::absolute(candidate).string();
        break;
      }
    }
  }

  if (iconDir.empty() || !std::filesystem::exists(iconDir)) {
    // Skip test if icon directory not found
    SUCCEED("SVG icon directory not found, skipping real-icon test");
    return;
  }

  // Try loading each SVG file in the directory
  int loaded = 0;
  int failed = 0;
  for (const auto& entry : std::filesystem::directory_iterator(iconDir)) {
    if (entry.path().extension() != ".svg") continue;

    std::string filePath = entry.path().string();
    NSVGimage* svgImage = nsvgParseFromFile(filePath.c_str(), "px", 96.0f);
    if (svgImage) {
      loaded++;
      nsvgDelete(svgImage);
    } else {
      failed++;
      FAIL("Failed to parse: " + filePath);
    }
  }

  REQUIRE(loaded > 0);
  REQUIRE(failed == 0);
}
