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

#include <limits>
#include <list>
#include <sstream>
#include <string>

#include <osmscout/CoreFeatures.h>

#include <osmscout/system/Assert.h>

#include <osmscout/private/CoreImportExport.h>

namespace osmscout {

  /**
   * Returns the numerical value of the given character, if the character
   * is a digit in a numerical value. The current code allows digits
   * in the range from 0-9 and a-f and A-F. And thus supports
   * numerical bases from 1-16.
   */
  extern OSMSCOUT_API bool GetDigitValue(char digit,
                                         size_t& result);

  template<typename N>
  size_t NumberDigits(const N& number,
                      size_t base=10)
  {
    N      value(number);
    size_t res=0;

    if (std::numeric_limits<N>::is_signed) {
      if (value<0) {
        res++;
      }
    }

    while (value!=0) {
      res++;
      value=value/base;
    }

    return res;
  }

  template<typename N>
  std::string NumberToStringSigned(const N& number)
  {
    std::string res;
    N           value(number);
    bool        negative=false;

    if (value<0) {
      negative=true;
      value=-value;
    }

    res.reserve(20);

    while (value!=0) {
      res.insert(0,1,(char)('0'+value%10));
      value=value/10;
    }

    if (res.empty()) {
      res.insert(0,1,'0');
    }
    else if (negative) {
      res.insert(0,1,'-');
    }

    return res;
  }

  template<typename N>
  std::string NumberToStringUnsigned(const N& number)
  {
    std::string res;
    N           value(number);

    res.reserve(20);

    while (value!=0) {
      res.insert(0,1,(char)('0'+value%10));
      value=value/10;
    }

    if (res.empty()) {
      res.insert(0,1,'0');
    }

    return res;
  }

  template<bool is_signed, typename N>
  struct NumberToStringTemplated
  {
  };

  template<typename N>
  struct NumberToStringTemplated<true, N>
  {
    static inline std::string f(const N& number)
    {
      return NumberToStringSigned<N>(number);
    }
  };

  template<typename N>
  struct NumberToStringTemplated<false, N>
  {
    static inline std::string f(const N& number)
    {
      return NumberToStringUnsigned<N>(number);
    }
  };

  /**
   * Converts the given (possibly negative) decimal number to a std::string.
   */
  template<typename N>
  inline std::string NumberToString(const N& number)
  {
    return NumberToStringTemplated<std::numeric_limits<N>::is_signed, N>
      ::f(number);
  }

  extern OSMSCOUT_API bool StringToNumber(const char* string, double& value);
  extern OSMSCOUT_API bool StringToNumber(const std::string& string, double& value);

  template<typename N>
  bool StringToNumberSigned(const std::string& string,
                            N& number,
                            size_t base=10)
  {
    assert(base<=16);

    std::string::size_type pos=0;
    bool                   minus=false;

    number=0;

    if (string.empty()) {
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
          string.length()==NumberDigits(std::numeric_limits<N>::max())) {
        minus=true;
        number=base/2;
      }
      else {
        number=digitValue;
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

      if (std::numeric_limits<N>::max()/(N)base-(N)digitValue<number) {
        return false;
      }

      number=number*base+digitValue;

      pos++;
    }

    if (minus) {
      number=-number;
    }

    return true;
  }

  template<typename N>
  bool StringToNumberUnsigned(const std::string& string,
                              N& number,
                              size_t base=10)
  {
    assert(base<=16);

    std::string::size_type pos=0;

    number=0;

    if (string.empty()) {
      return false;
    }

    if (string[0]=='-') {
      return false;
    }

    /*
      Special handling for the first symbol/digit (could be negative)
      */
    size_t digitValue;

    if (!GetDigitValue(string[pos],digitValue)) {
      return false;
    }

    if (digitValue>=base) {
      return false;
    }

    number=digitValue;

    pos=1;

    while (pos<string.length()) {
      size_t digitValue;

      if (!GetDigitValue(string[pos],digitValue)) {
        return false;
      }

      if (digitValue>=base) {
        return false;
      }

      if (std::numeric_limits<N>::max()/(N)base-(N)digitValue<number) {
        return false;
      }

      number=number*base+digitValue;

      pos++;
    }

    return true;
  }

  template<bool is_signed, typename N>
  struct StringToNumberTemplated
  {
  };

  template<typename N>
  struct StringToNumberTemplated<true, N>
  {
    static inline unsigned int f(const std::string& string,
                                 N& number,
                                 size_t base=10)
    {
      return StringToNumberSigned<N>(string,number,base);
    }
  };

  template<typename N>
  struct StringToNumberTemplated<false, N>
  {
    static inline unsigned int f(const std::string& string,
                                 N& number,
                                 size_t base=10)
    {
      return StringToNumberUnsigned<N>(string,number,base);
    }
  };

  /**
   * Converts a string holding a (possibly negative) numerical
   * value of the given base to the numerical value itself.
   *
   * Example:
   *  "-13" => -13
   */
  template<typename N>
  inline unsigned int StringToNumber(const std::string& string,
                                     N& number,
                                     size_t base=10)
  {
    return StringToNumberTemplated<std::numeric_limits<N>::is_signed, N>
      ::f(string,number,base);
  }

  extern OSMSCOUT_API std::string StringListToString(const std::list<std::string>& list,
                                                     const std::string& separator="/");

  extern OSMSCOUT_API std::string ByteSizeToString(double size);

#if defined(OSMSCOUT_HAVE_STD__WSTRING)
  extern OSMSCOUT_API std::wstring UTF8StringToWString(const std::string& text);
#endif
}

#endif
