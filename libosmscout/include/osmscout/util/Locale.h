#ifndef OSMSCOUT_LOCALE_H
#define OSMSCOUT_LOCALE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2019 Lukáš Karas

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

#include <osmscout/CoreImportExport.h>
#include <osmscout/system/Compiler.h>

#include <osmscout/util/Distance.h>

#include <string>

namespace osmscout {

  class OSMSCOUT_API Locale CLASS_FINAL
  {
  private:
    Units distanceUnits{Units::Metrics};
    std::string decimalSeparator{"."}; //!<  UTF-8 encoded string with decimal separator
    std::string thousandsSeparator; //!< UTF-8 encoded string with thousands separator

    // use tiny, unbreakable space between value and unit.
    // U+202F unicode is encoded as 0xE280AF in utf-8
    // because unicode support in C++ is poor and platform dependent,
    // we will use utf-8 bytes directly
    std::string unitsSeparator{"\xE2\x80\xAF"}; //!< UTF-8 encoded string with unit separator (character between number and unit). Unbreakable space by default

  public:
    /**
     * Default constructor initialise locale with default locale.
     * It is equivalent to LC_ALL=C
     *
     * To initialise locale by environment, use static method ByEnvironment
     */
    Locale() = default;

    Locale(const Units &distanceUnits,
           const std::string &decimalSeparator,
           const std::string &thousandsSeparator);

    Locale(const Locale&) = default;
    Locale(Locale&&) = default;
    ~Locale() = default;
    Locale &operator=(const Locale &) = default;
    Locale &operator=(Locale &&) = default;

    Units GetDistanceUnits() const
    {
      return distanceUnits;
    }

    void SetDistanceUnits(const Units &units)
    {
      this->distanceUnits = units;
    }

    std::string GetDecimalSeparator() const
    {
      return decimalSeparator;
    }

    void SetDecimalSeparator(const std::string &separator)
    {
      this->decimalSeparator = separator;
    }

    std::string GetThousandsSeparator() const
    {
      return thousandsSeparator;
    }

    void SetThousandsSeparator(const std::string &separator)
    {
      this->thousandsSeparator = separator;
    }

    std::string GetUnitsSeparator() const
    {
      return unitsSeparator;
    }

    void SetUnitsSeparator(const std::string &separator)
    {
      this->unitsSeparator = separator;
    }

  public:
    static Locale ByEnvironment(std::locale locale = std::locale(""));
  };
}

#endif // OSMSCOUT_LOCALE_H
