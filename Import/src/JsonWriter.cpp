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

#include "JsonWriter.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <limits>
#include <locale>
#include <ostream>
#include <sstream>
#include <string>

namespace osmscout {

  JsonWriter::JsonWriter(std::ostream& out)
  : out(out)
  {
    // JSON requires '.' as decimal separator and no thousands grouping,
    // independent of the process locale
    out.imbue(std::locale::classic());
  }

  void JsonWriter::WriteIndent(size_t depth)
  {
    for (size_t i=0; i<depth; i++) {
      out << "  ";
    }
  }

  void JsonWriter::WriteSeparator()
  {
    if (!stack.empty() && stack.back().childCount>0) {
      out << ",";
    }

    out << "\n";
    WriteIndent(stack.size());

    if (!stack.empty()) {
      stack.back().childCount++;
    }
  }

  void JsonWriter::WriteString(const std::string& value)
  {
    constexpr unsigned char controlCharLimit=0x20;

    out << "\"";

    for (const char c : value) {
      switch (c) {
      case '"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c)<controlCharLimit) {
          std::ostringstream hex;

          hex << "\\u" << std::hex << std::setw(4) << std::setfill('0')
              << static_cast<unsigned int>(static_cast<unsigned char>(c));
          out << hex.str();
        }
        else {
          out << c;
        }
        break;
      }
    }

    out << "\"";
  }

  void JsonWriter::WriteValueStart()
  {
    if (afterKey) {
      afterKey=false;

      return;
    }

    if (stack.empty()) {
      return;
    }

    WriteSeparator();
  }

  void JsonWriter::BeginObject()
  {
    WriteValueStart();
    out << "{";
    stack.push_back(Frame {.context=Context::object,.childCount=0});
  }

  void JsonWriter::EndObject()
  {
    if (stack.empty() || stack.back().context!=Context::object) {
      return;
    }

    if (stack.back().childCount>0) {
      out << "\n";
      WriteIndent(stack.size()-1);
    }

    out << "}";
    stack.pop_back();
  }

  void JsonWriter::BeginArray()
  {
    WriteValueStart();
    out << "[";
    stack.push_back(Frame {.context=Context::array,.childCount=0});
  }

  void JsonWriter::EndArray()
  {
    if (stack.empty() || stack.back().context!=Context::array) {
      return;
    }

    if (stack.back().childCount>0) {
      out << "\n";
      WriteIndent(stack.size()-1);
    }

    out << "]";
    stack.pop_back();
  }

  void JsonWriter::Key(const std::string& name)
  {
    if (stack.empty() || stack.back().context!=Context::object) {
      return;
    }

    WriteSeparator();
    WriteString(name);
    out << ": ";
    afterKey=true;
  }

  void JsonWriter::Value(const std::string& value)
  {
    WriteValueStart();
    WriteString(value);
  }

  void JsonWriter::Value(const char* value)
  {
    Value(std::string(value));
  }

  void JsonWriter::Value(int64_t value)
  {
    WriteValueStart();
    out << value;
  }

  void JsonWriter::Value(uint64_t value)
  {
    WriteValueStart();
    out << value;
  }

  void JsonWriter::Value(double value)
  {
    WriteValueStart();

    if (!std::isfinite(value)) {
      out << "null";

      return;
    }

    out << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
  }

  void JsonWriter::Value(bool value)
  {
    WriteValueStart();
    out << (value ? "true" : "false");
  }

  void JsonWriter::Null()
  {
    WriteValueStart();
    out << "null";
  }
}
