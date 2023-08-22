#ifndef OSMSCOUT_STYLEERROR_H
#define OSMSCOUT_STYLEERROR_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2023  Lukas Karas

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

#include <osmscoutmap/MapImportExport.h>

#include <string>

namespace osmscout {

  /**
   * \ingroup Stylesheet
   */
  class OSMSCOUT_MAP_API StyleError
  {
  public:
    enum StyleErrorType {
      Symbol, Error, Warning, Exception
    };

  public:
    StyleError(StyleErrorType type, int line, int column, const std::string &text) :
      type(type), line(line), column(column), text(text){}

    StyleError(const StyleError&) = default;
    StyleError(StyleError&&) = default;

    StyleError& operator=(const StyleError&) = default;
    StyleError& operator=(StyleError&&) = default;

    ~StyleError() = default;

    StyleErrorType GetType() const
    {
      return type;
    }

    std::string GetTypeName() const
    {
      switch(type){
        case Symbol:
          return "Symbol";
        case Error:
          return "Error";
        case Warning:
          return "Warning";
        case Exception:
          return "Exception";
        default:
          assert(false);
          return "???";
      }
    }

    int GetLine() const
    {
      return line;
    }

    int GetColumn() const
    {
      return column;
    }

    const std::string &GetText() const
    {
      return text;
    }

    std::string GetShortDescription() const
    {
      return GetTypeName() + ": " + GetText();
    }

    std::string GetDescription() const
    {
      return std::to_string(GetLine()) + "," + std::to_string(GetColumn()) + " " + GetShortDescription();
    }

  private:
    StyleErrorType  type;
    int             line;
    int             column;
    std::string     text;
  };

}

#endif //OSMSCOUT_STYLEERROR_H
