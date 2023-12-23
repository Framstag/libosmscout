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
#include <memory>
#include <string>
#include <chrono>
#include <optional>
#include <utility>

#include <osmscout/lib/CoreFeatures.h>
#include <osmscout/lib/CoreImportExport.h>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Time.h>
#include <osmscout/util/Locale.h>

#include <osmscout/OSMScoutTypes.h>

namespace osmscout {

  /**
   * \defgroup Util Utility stuff
   *
   * General utility stuff like enhanced string operations, special data structures...
   */

  /**
   * \ingroup Util
   * Convert the given string to a boolean value
   *
   * @param string
   *    string with a potential boolean value (either 'true' or 'false')
   * @param value
   *    value to copy the result to if no error occurred
   * @return
   *    'true' if the value was parsed, else 'false'
   */
  extern OSMSCOUT_API bool StringToBool(const char* string, bool& value);

  /**
   * Returns a string representation of the given boolean value (either 'true' or 'false')
   *
   * @param value
   *    value to return
   * @return
   *    result of the conversion
   */
  extern OSMSCOUT_API const char* BoolToString(bool value);

  /**
   * Returns locale-aware string representation of number
   *
   * @param value
   * @param locale
   * @return UTF-8 string
   */
  extern OSMSCOUT_API std::string NumberToString(long value, const Locale &locale);

  /**
   * Returns locale-aware string representation of number
   *
   * @param value
   * @param locale
   * @param precision
   * @return UTF-8 string
   */
  extern OSMSCOUT_API std::string FloatToString(double value, const Locale &locale, uint32_t precision = 3);

  /**
   * \ingroup Util
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
      value=value/(N)base;
    }

    return res;
  }

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

      if (!GetDigitValue(string[pos],
                         digitValue)) {
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
        number=(N)(base/2);
      }
      else {
        number=(N)digitValue;
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

      number=(N)(number*base+digitValue);

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

    number=(N)digitValue;

    pos=1;

    while (pos<string.length()) {
      size_t digitValue;

      if (!GetDigitValue(string[pos],
                         digitValue)) {
        return false;
      }

      if (digitValue>=base) {
        return false;
      }

      if (std::numeric_limits<N>::max()/(N)base-(N)digitValue<number) {
        return false;
      }

      number=(N)(number*base+digitValue);

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
    static inline bool f(const std::string& string,
                                 N& number,
                                 size_t base=10)
    {
      return StringToNumberSigned<N>(string,number,base);
    }
  };

  template<typename N>
  struct StringToNumberTemplated<false, N>
  {
    static inline bool f(const std::string& string,
                                 N& number,
                                 size_t base=10)
    {
      return StringToNumberUnsigned<N>(string,number,base);
    }
  };

  /**
   * \ingroup Util
   * Converts a string holding a (possibly negative) numerical
   * value of the given base to the numerical value itself.
   *
   * Example:
   *  "-13" => -13
   */
  template<typename N>
  inline bool StringToNumber(const std::string& string,
                             N& number,
                             size_t base=10)
  {
    return StringToNumberTemplated<std::numeric_limits<N>::is_signed, N>
      ::f(string,number,base);
  }

  /**
   * \ingroup Util
   */
  extern OSMSCOUT_API bool StringToNumber(const char* string, double& value);

  /**
   * \ingroup Util
   */
  extern OSMSCOUT_API bool StringToNumber(const std::string& string, double& value);

  /**
   * \ingroup Util
   *
   */

  extern OSMSCOUT_API size_t CountWords(const std::string& text);

  /**
   * \ingroup Util
   * Converts the given string into a list of whitespace separated (std::isspace()) strings.
   */
  extern OSMSCOUT_API std::list<std::string> SplitStringAtSpace(const std::string& input);

  /**
   * \ingroup Util
   * Split string by separator. For arguments "asphalt;ground;gravel" and ";" return list of three...
   *
   * \note when stringList is empty, result is empty list
   * \note separator must not be empty
   * \note when string ends with separator, last (empty) element is omitted
   * \note when maxSize is negative, list will contains all elements
   */
  extern OSMSCOUT_API std::list<std::string> SplitString(const std::string& stringList,
                                                         const std::string& separator,
                                                         int maxSize=-1);

  /**
   * \ingroup Util
   * Replace all occurrences of search in input string by some other string.
   * When search is empty, unchanged input string is returned.
   *
   * @param in - input string
   * @param search
   * @param replacement
   * @return
   */
  extern OSMSCOUT_API std::string ReplaceString(const std::string &in,
                                                const std::string &search,
                                                const std::string &replacement);

  /**
   * Split string by separator to two parts. Unlike SplitString with maxSize=2,
   * second element contains the rest of the string after first separator.
   * When no separator found, nullopt is returned.
   *
   * \note when str is empty nullopt is returned
   * \note separator must not be empty
   * \note when string ends with separator, and there is just one, second element is empty
   */
  extern OSMSCOUT_API std::optional<std::pair<std::string,std::string>> SplitStringToPair(const std::string& str,
                                                                                          const std::string& separator);

  /**
   * \ingroup Util
   * Assumes that the string consists of a number of values separated by one of the given divider.
   * If the list consists of one entry, no divider is used.
   *
   * Returns the first entry in the list
   *
   * \note stringList must not be empty
   * \note at least one divider must be given
   */
  extern OSMSCOUT_API std::string GetFirstInStringList(const std::string& stringList,
                                                       const std::string& divider);

  /**
   * \ingroup Util
   * Converts the given string into a list of whitespace or colon-separated strings.
   */
  extern OSMSCOUT_API void TokenizeString(const std::string& input,
                                          std::list<std::string>& tokens);

  /**
   * \ingroup Util
   * Simplifying a token list by merging tokens that start with an
   * upper case letter followed by a token starting with a lower
   * case letter.
   */
  extern OSMSCOUT_API void SimplifyTokenList(std::list<std::string>& tokens);

  extern OSMSCOUT_API std::string GetTokensFromStart(const std::list<std::string>& tokens,
                                                    size_t count);

  extern OSMSCOUT_API std::string GetTokensFromEnd(const std::list<std::string>& tokens,
                                                   size_t count);

  /**
   * \ingroup Util
   * Given a list of strings, individual strings will be combined into a given
   * number of sub groups (individual string concatenated and separated by a space).
   *
   * If you pass a list of 5 strings to be divided into 3 parts, a list of string-list will
   * be returned, where each list contains exactly 3 strings.
   */
  extern OSMSCOUT_API void GroupStringListToStrings(std::list<std::string>::const_iterator token,
                                                    size_t listSize,
                                                    size_t parts,
                                                    std::list<std::list<std::string> >& lists);


  /**
   * \ingroup Util
   *
   * Prints byte size with short, human readable form by ISO/IEC 80000 standard.
   * It means that KiB stands for 1024 bytes, MiB for 1024^2, GiB 1024^3...
   *
   * Returned string is locale aware, UTF-8 encoded
   */
  extern OSMSCOUT_API std::string ByteSizeToString(FileOffset size, const Locale &locale = Locale::ByEnvironmentSafe());
  extern OSMSCOUT_API std::string ByteSizeToString(double size, const Locale &locale = Locale::ByEnvironmentSafe());

  /**
   * \ingroup Util
   *
   * Converts the given std::string with content in the current locale to a std::wstring
   *
   * @param text
   *    String to get converted
   * @return
   *    corresponding std::wstring
   */
  extern OSMSCOUT_API std::wstring LocaleStringToWString(const std::string& text);

  /**
   * \ingroup Util
   *
   * Converts the given std::wstring to a std::string with content in the current locale
   *
   * @param text
   *    String to get converted
   * @return
   *    corresponding std::string
   */
  extern OSMSCOUT_API std::string WStringToLocaleString(const std::wstring& text);

  /**
   * \ingroup Util
   *
   * Convert the given std::string containign a UTF8 character sequence to a std::wstring
   *
   * @param text
   *    String to get converted
   * @return
   *    corresponding std::wstring
   */
  extern OSMSCOUT_API std::wstring UTF8StringToWString(const std::string& text);

  /**
   * \ingroup Util
   *
   * Convert the given std::string containign a UTF8 character sequence to a std::u32string
   *
   * @param text
   *    String to get converted
   * @return
   *    corresponding std::wstring
   */
  extern OSMSCOUT_API std::u32string UTF8StringToU32String(const std::string& text);

  /**
   * \ingroup Util
   *
   * Convert the given std::wstring to a std::string containing a corresponding UTF8 character sequence
   *
   * @param text
   *    the std::wstring to get converted
   * @return
   *    the converted std::string
   */
  extern OSMSCOUT_API std::string WStringToUTF8String(const std::wstring& text);

  /**
   * \ingroup Util
   *
   * Convert the given std::string in the current locale to a std::string containing a corresponding
   * UTF8 character sequence
   *
   * @param text
   *    the std::wstring to get converted
   * @return
   *    the converted std::string
   */
  extern OSMSCOUT_API std::string LocaleStringToUTF8String(const std::string& text);

  /**
   * \ingroup Util
   *
   * Convert the given std::string in UTF-8 a std::string containing to corresponding string in the
   * current locale.
   *
   * @param text
   *    the std::wstring to get converted
   * @return
   *    the converted std::string
   */
  extern OSMSCOUT_API std::string UTF8StringToLocaleString(const std::string& text);

  /**
   * Convert the given std::string containing a UTF8 character sequence to upper case using
   * translation table implementation.
   *
   * @param text
   *    Text to get converted
   * @return
   *    Converted text
   *
   */
  extern OSMSCOUT_API std::string UTF8StringToUpper(const std::string& text);

  /**
   * Convert the given std::string containing a UTF8 character sequence to lower case using
   * the translation table implementation.
   *
   * @param text
   *    Text to get converted
   * @return
   *    Converted text
   *
   */
  extern OSMSCOUT_API std::string UTF8StringToLower(const std::string& text);

  /**
   * Normalise the given std::string containing a UTF8 character sequence
   * for tolerant comparison. It may be used for string lookup typed by human,
   * for example street name, where string are not binary equals,
   * but are "same" for human - for example "Baker Street" and "Baker  street"
   *
   * Normalized string is converted to lowercase, all whitespaces are converted
   * to standard space and multiple following spaces are collapsed to one.
   *
   * @param text
   *    Text to get converted
   * @return
   *    Converted text
   */
  extern OSMSCOUT_API std::string UTF8NormForLookup(const std::string& text);

  /**
   * Transliterate non-ascii characters to one or more characters that are similar
   * to the original character. When there is no transformation available, original
   * character is keep in place (translation table implementation).
   *
   * @param text
   *    Text to get converted
   * @return
   *    Converted text
   *
   */
  extern OSMSCOUT_API std::string UTF8Transliterate(const std::string& text);

  /**
   * Parse time string in ISO 8601 format "2017-11-26T13:46:12.124Z" (UTC timezone)
   * to Timestamp (std::chrono::time_point with millisecond accuracy).
   *
   * Note that format is not locale specific
   *
   * @param timeStr
   * @param timestamp
   * @return true on success
   */
  extern OSMSCOUT_API bool ParseISO8601TimeString(const std::string &timeStr, Timestamp &timestamp);

  /**
   * Format Timestamp to string in ISO8601 format "2017-11-26T13:46:12.124Z"
   * for UTC timezone.
   *
   * @param timestamp
   * @return time string
   */
  extern OSMSCOUT_API std::string TimestampToISO8601TimeString(const Timestamp &timestamp);

  extern OSMSCOUT_API std::string DurationString(const Duration &duration);

  /**
   * Trim trimmedChar from begin and end of str.
   */
  extern OSMSCOUT_API std::string Trim(const std::string &str, char trimmedChar=' ');
}

#endif
