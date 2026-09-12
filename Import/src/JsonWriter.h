#ifndef OSMSCOUT_IMPORT_JSONWRITER_H
#define OSMSCOUT_IMPORT_JSONWRITER_H

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

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace osmscout {

  /**
   * Minimal write-only JSON writer with pretty printing (2-space indent).
   *
   * Emits valid JSON. Strings are escaped (quotes, backslash, control
   * characters); non-ASCII characters pass through as UTF-8. Non-finite
   * double values are emitted as null, since JSON forbids NaN/Infinity.
   *
   * Usage:
   *   JsonWriter writer(out);
   *   writer.BeginObject();
   *   writer.Key("name");
   *   writer.Value("value");
   *   writer.EndObject();
   */
  class JsonWriter
  {
  private:
    enum class Context : std::uint8_t {
      object,
      array
    };

    struct Frame
    {
      Context context;
      size_t  childCount;
    };

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members) writer holds the output stream
    std::ostream       & out;
    std::vector<Frame> stack;
    bool               afterKey=false;

  private:
    void WriteIndent(size_t depth);

    void WriteSeparator();

    void WriteString(const std::string& value);

    void WriteValueStart();

  public:
    explicit JsonWriter(std::ostream& out);

    void BeginObject();

    void EndObject();

    void BeginArray();

    void EndArray();

    void Key(const std::string& name);

    void Value(const std::string& value);

    void Value(const char* value);

    void Value(int64_t value);

    void Value(uint64_t value);

    void Value(double value);

    void Value(bool value);

    void Null();
  };
}

#endif //OSMSCOUT_IMPORT_JSONWRITER_H
