#ifndef OSMSCOUT_UTIL_STRING_H
#define OSMSCOUT_UTIL_STRING_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <cassert>
#include <limits>
#include <list>
#include <sstream>
#include <string>

#include <osmscout/private/CoreImportExport.h>

namespace osmscout {

  extern OSMSCOUT_API bool GetDigitValue(char digit, size_t& result);

  template<typename A>
  size_t NumberDigits(const A& a,size_t base=10)
  {
    A      value(a);
    size_t res=0;

    if (value<0) {
      res++;
    }

    while (value!=0) {
      res++;
      value=value/base;
    }

    return res;
  }

  template<typename A>
  std::string NumberToString(const A& a)
  {
    std::string res;
    A           value(a);
    bool        negative=false;

    if (std::numeric_limits<A>::is_signed) {
      if (value<0) {
        negative=true;
        value=-value;
      }
    }

    res.reserve(20);

    while (value!=0) {
      res.insert(0,1,(char)('0'+value%10));
      value=value/10;
    }

    if (res.empty()) {
      res.insert(0,1,'0');
    }

    if (negative) {
      res.insert(0,1,'-');
    }

    return res;
  }

  extern OSMSCOUT_API bool StringToNumber(const char* string, double& value);
  extern OSMSCOUT_API bool StringToNumber(const std::string& string, double& value);

  template<typename A>
  bool StringToNumber(const std::string& string, A& a, size_t base=10)
  {
    assert(base<=16);

    std::string::size_type pos=0;
    bool                   minus=false;

    a=0;

    if (string.empty()) {
      return false;
    }

    if (!std::numeric_limits<A>::is_signed && string[0]=='-') {
      return false;
    }

    /*
      Special handling for the first symbol/digit (could be negative)
      */
    if (base==10 && string[0]=='-') {
      minus=true;
      pos=1;
    }
    else {
      size_t digitValue;

      if (!GetDigitValue(string[pos],digitValue)) {
        return false;
      }

      if (digitValue>=base) {
        return false;
      }

      /*
        For signed values with base!=10 we assume a negative value
      */
      if (digitValue==base-1 &&
          std::numeric_limits<A>::is_signed &&
          string.length()==NumberDigits(std::numeric_limits<A>::max())) {
        minus=true;
        a=base/2;
      }
      else {
        a=digitValue;
      }

      pos=1;
    }

    while (pos<string.length()) {
      size_t digitValue;

      if (!GetDigitValue(string[pos],digitValue)) {
        return false;
      }

      if (digitValue>=base) {
        return false;
      }

      if (std::numeric_limits<A>::max()/(A)base-(A)digitValue<a) {
        return false;
      }

      a=a*base+digitValue;

      pos++;
    }

    if (minus) {
      a=-a;
    }

    return true;
  }

  extern OSMSCOUT_API std::string StringListToString(const std::list<std::string>& list,
                                                     const std::string& separator="/");

  extern OSMSCOUT_API std::wstring UTF8StringToWString(const std::string& text);
}

#endif
