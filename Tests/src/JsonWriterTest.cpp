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

#include <cmath>
#include <limits>
#include <sstream>

#include <JsonWriter.h>

namespace {

  // Writes one JSON object whose content is produced by the given fill
  // callback and returns the serialized text.
  template<class F>
  std::string WriteObject(F fill)
  {
    std::ostringstream   out;
    osmscout::JsonWriter writer(out);

    writer.BeginObject();
    fill(writer);
    writer.EndObject();

    return out.str();
  }
}

TEST_CASE("JsonWriter writes empty object", "[JsonWriter]")
{
  REQUIRE(WriteObject([](osmscout::JsonWriter&){}) == "{}");
}

TEST_CASE("JsonWriter writes empty array", "[JsonWriter]")
{
  std::ostringstream   out;
  osmscout::JsonWriter writer(out);

  writer.BeginArray();
  writer.EndArray();

  REQUIRE(out.str()=="[]");
}

TEST_CASE("JsonWriter writes object with string value", "[JsonWriter]")
{
  REQUIRE(WriteObject([](osmscout::JsonWriter& writer){
    writer.Key("name");
    writer.Value("value");
  })=="{\n  \"name\": \"value\"\n}");
}

TEST_CASE("JsonWriter writes multiple keys with comma separation", "[JsonWriter]")
{
  REQUIRE(WriteObject([](osmscout::JsonWriter& writer){
    writer.Key("a");
    writer.Value(static_cast<int64_t>(1));
    writer.Key("b");
    writer.Value(static_cast<int64_t>(2));
  })=="{\n  \"a\": 1,\n  \"b\": 2\n}");
}

TEST_CASE("JsonWriter escapes quotes and backslashes", "[JsonWriter]")
{
  REQUIRE(WriteObject([](osmscout::JsonWriter& writer){
    writer.Key("path");
    writer.Value("a\"b\\c");
  })=="{\n  \"path\": \"a\\\"b\\\\c\"\n}");
}

TEST_CASE("JsonWriter escapes control characters", "[JsonWriter]")
{
  REQUIRE(WriteObject([](osmscout::JsonWriter& writer){
    writer.Key("text");
    writer.Value(std::string("a\nb\tc\rd\be\ff\x01g"));
  })=="{\n  \"text\": \"a\\nb\\tc\\rd\\be\\ff\\u0001g\"\n}");
}

TEST_CASE("JsonWriter passes through non-ASCII UTF-8", "[JsonWriter]")
{
  REQUIRE(WriteObject([](osmscout::JsonWriter& writer){
    writer.Key("name");
    writer.Value("München");
  })=="{\n  \"name\": \"München\"\n}");
}

TEST_CASE("JsonWriter writes nested structures", "[JsonWriter]")
{
  REQUIRE(WriteObject([](osmscout::JsonWriter& writer){
    writer.Key("outer");
    writer.BeginObject();
    writer.Key("list");
    writer.BeginArray();
    writer.Value(static_cast<int64_t>(1));
    writer.Value(static_cast<int64_t>(2));
    writer.EndArray();
    writer.EndObject();
  })=="{\n  \"outer\": {\n    \"list\": [\n      1,\n      2\n    ]\n  }\n}");
}

TEST_CASE("JsonWriter writes unsigned values", "[JsonWriter]")
{
  REQUIRE(WriteObject([](osmscout::JsonWriter& writer){
    writer.Key("size");
    writer.Value(static_cast<uint64_t>(18446744073709551615ULL));
  })=="{\n  \"size\": 18446744073709551615\n}");
}

TEST_CASE("JsonWriter writes doubles with full precision", "[JsonWriter]")
{
  REQUIRE(WriteObject([](osmscout::JsonWriter& writer){
    writer.Key("value");
    writer.Value(0.1);
  })=="{\n  \"value\": 0.10000000000000001\n}");
}

TEST_CASE("JsonWriter writes non-finite doubles as null", "[JsonWriter]")
{
  REQUIRE(WriteObject([](osmscout::JsonWriter& writer){
    writer.Key("nan");
    writer.Value(std::nan(""));
    writer.Key("inf");
    writer.Value(std::numeric_limits<double>::infinity());
  })=="{\n  \"nan\": null,\n  \"inf\": null\n}");
}

TEST_CASE("JsonWriter writes booleans and null", "[JsonWriter]")
{
  REQUIRE(WriteObject([](osmscout::JsonWriter& writer){
    writer.Key("yes");
    writer.Value(true);
    writer.Key("no");
    writer.Value(false);
    writer.Key("nothing");
    writer.Null();
  })=="{\n  \"yes\": true,\n  \"no\": false,\n  \"nothing\": null\n}");
}
