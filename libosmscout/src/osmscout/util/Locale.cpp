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

#include <osmscout/util/Locale.h>
#include <osmscout/util/String.h>

namespace osmscout {

  namespace { // anonymous namespace

  std::string GetCountryCode(std::string localeName) {
    if (localeName.size()>=5 && localeName[2]=='_'  &&
        localeName[3]>='A' && localeName[3]<='Z' &&
        localeName[4]>='A' && localeName[4]<='Z'){
      // locale starts with lang_COUNTRY
      return localeName.substr(3,2);
    }
    return "";
  }
  }

  Locale Locale::ByEnvironment(std::locale cppLocale)
  {
    Locale locale;

    std::string country=GetCountryCode(cppLocale.name());
    if (country=="US" || // USA
        country=="GB" || // Britain
        country=="LR" || // Liberia
        country=="MM"  // Myanmar
    ){
      locale.SetDistanceUnits(Units::Imperial);
    }

    using Facet = std::numpunct<wchar_t>;

    wchar_t thousandSep = std::use_facet<Facet>(cppLocale).thousands_sep();
    wchar_t decimalSep = std::use_facet<Facet>(cppLocale).decimal_point();

    locale.SetThousandsSeparator(WStringToUTF8String(std::wstring() + thousandSep));
    locale.SetDecimalSeparator(WStringToUTF8String(std::wstring() + decimalSep));

    return locale;
  }
}
