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

#include <osmscout/util/String.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cwchar>
#include <iomanip>
#include <locale>
#include <sstream>
#include <array>
#include <ctime>
#include <thread>

#include <osmscout/system/Math.h>

#include <osmscout/log/Logger.h>
#include <osmscout/util/Locale.h>
#include <osmscout/util/ObjectPool.h>
#include <osmscout/util/utf8helper.h>

#include <osmscout/private/Config.h>

#if defined(HAVE_CODECVT)
#include <codecvt>
#else
static_assert(false, "Missing <codecvt> header, needed for charset conversions");
#endif

namespace osmscout {

  bool StringToBool(const char* string, bool& value)
  {
    if (std::strcmp(string,"true")==0) {
      value=true;

      return true;
    }
    else if (std::strcmp(string,"false")==0) {
      value=false;

      return true;
    }

    return false;
  }

  const char* BoolToString(bool value)
  {
    if (value) {
      return "true";
    }

    return "false";
  }

  std::string NumberToString(long value, const Locale &locale)
  {
    std::stringstream ss;
    ss.imbue(std::locale("C"));
    if (std::abs(value) < 1000 || locale.GetThousandsSeparator().empty()){
      ss << value;
    }else{
      long mag=1000;
      while (mag*1000 < std::abs(value)) {
        mag*=1000;
      }
      ss << (value/mag);
      while (mag>1) {
        ss << locale.GetThousandsSeparator();
        value = std::abs(value % mag);
        mag/=1000;
        ss << std::setw(3) << std::setfill('0') << (value/mag);
      }
    }
    return ss.str();
  }

  extern OSMSCOUT_API std::string FloatToString(double value, const Locale &locale, uint32_t precision)
  {
    std::stringstream ss;
    double order = std::pow(10, precision);
    value = std::round(value * order) / order;
    ss << NumberToString(static_cast<long>(value), locale);

    if (precision > 0) {
      ss << locale.GetDecimalSeparator();
      double fractionNum = std::abs(value) - std::floor(std::abs(value));
      std::string fraction = NumberToString(fractionNum * order, Locale());
      for (size_t i = 0; i < fraction.size(); i++) {
        if (i > 0 && i % 3 == 0 && i < fraction.size() - 1) {
          ss << locale.GetThousandsSeparator();
        }
        ss << fraction[i];
      }
    }
    return ss.str();
  }

  bool GetDigitValue(char digit, size_t& result)
  {
    switch (digit) {
    case '1':
      result=1;
      return true;
    case '2':
      result=2;
      return true;
    case '3':
      result=3;
      return true;
    case '4':
      result=4;
      return true;
    case '5':
      result=5;
      return true;
    case '6':
      result=6;
      return true;
    case '7':
      result=7;
      return true;
    case '8':
      result=8;
      return true;
    case '9':
      result=9;
      return true;
    case '0':
      result=0;
      return true;
    case 'a':
    case 'A':
      result=10;
      return true;
    case 'b':
    case 'B':
      result=11;
      return true;
    case 'c':
    case 'C':
      result=12;
      return true;
    case 'd':
    case 'D':
      result=13;
      return true;
    case 'e':
    case 'E':
      result=14;
      return true;
    case 'f':
    case 'F':
      result=15;
      return true;
    default:
      return false;
    }
  }

  bool StringToNumber(const char* string, double& value)
  {
    std::istringstream stream(string);

    stream.imbue(std::locale("C"));

    stream >> value;

    return stream.eof();
  }

  bool StringToNumber(const std::string& string, double& value)
  {
    std::istringstream stream(string);

    stream.imbue(std::locale("C"));

    stream >> value;

    return stream.eof();
  }

  size_t CountWords(const std::string& text)
  {
    size_t wordCount=0;
    bool   inWord=false;
    size_t nextPos=0;

    // Leading space
    while (nextPos<text.length() && std::isspace(text[nextPos])!=0) {
      nextPos++;
    }

    // We are counting the number of transitions from inWord=false to inWord=true
    while (nextPos<text.length()) {
      if (inWord) {
        if (std::isspace(text[nextPos])!=0) {
          inWord=false;
        }
      }
      else {
        if (std::isspace(text[nextPos])==0) {
          inWord=true;
          wordCount++;
        }
      }

      nextPos++;
    }

    return wordCount;
  }

  std::string ByteSizeToString(FileOffset value, const Locale &locale)
  {
    return ByteSizeToString(static_cast<double>(value), locale);
  }

  std::string ByteSizeToString(double value, const Locale &locale)
  {
    std::stringstream buffer;

    if (value<1.0 && value>-1) {
      buffer << "0" << locale.GetUnitsSeparator() << "B";
    }
    else if (ceil(value)>=std::pow(1024.0, 4.0)) {
      buffer << FloatToString(value/std::pow(1024.0, 4.0), locale, 1) << locale.GetUnitsSeparator()  << "TiB";
    }
    else if (ceil(value)>=std::pow(1024.0, 3.0)) {
      buffer << FloatToString(value/std::pow(1024.0, 3.0), locale, 1) << locale.GetUnitsSeparator() << "GiB";
    }
    else if (ceil(value)>=std::pow(1024.0, 2.0)) {
      buffer << FloatToString(value/std::pow(1024.0, 2.0), locale, 1) << locale.GetUnitsSeparator() << "MiB";
    }
    else if (ceil(value)>=1024.0) {
      buffer << FloatToString(value/1024.0, locale, 1) << locale.GetUnitsSeparator() << "KiB";
    }
    else {
      buffer << FloatToString(value, locale, 0) << locale.GetUnitsSeparator() << "B";
    }

    return buffer.str();
  }

  std::list<std::string> SplitStringAtSpace(const std::string& input)
  {
    std::list<std::string> tokens;
    std::string::size_type wordBegin=0;
    std::string::size_type wordEnd=0;

    // While not at end of string
    while (wordBegin<input.length()) {
      // skip all whitespace characters
      while (wordBegin<input.length() &&
             std::isspace(input[wordBegin])!=0) {
        wordBegin++;
      }

      // if we at the end => finish, else we have found the next word
      if (wordBegin>=input.length()) {
        return tokens;
      }

      wordEnd=wordBegin;

      // Collect characters until whitespace or end of string
      while (wordEnd+1<input.length() &&
             std::isspace((unsigned char)input[wordEnd + 1])==0) {
        wordEnd++;
      }

      // Copy token
      std::string token=input.substr(wordBegin,wordEnd-wordBegin+1);

      tokens.push_back(token);

      // next iteration
      wordBegin=wordEnd+1;
    }

    return tokens;
  }

  extern OSMSCOUT_API std::list<std::string> SplitString(const std::string& stringList,
                                                         const std::string& separator,
                                                         int maxSize)
  {
    assert(!separator.empty());

    std::string remaining=stringList;
    std::list<std::string> result;
    while (!remaining.empty() && (maxSize < 0 || result.size() < (size_t)maxSize)) {
      std::string::size_type pos = remaining.find(separator);

      if (pos == std::string::npos) {
        result.push_back(remaining);
        remaining.clear();
      }else{
        result.push_back(remaining.substr(0, pos));
        remaining.erase(0, pos + separator.length());
      }
    }
    return result;
  }

  std::string ReplaceString(const std::string &in,
                            const std::string &search,
                            const std::string &replacement)
  {
    if (search.empty()) {
      return in;
    }

    std::ostringstream result;
    std::string remaining=in;

    while (!remaining.empty()) {
      std::string::size_type pos = remaining.find(search);
      if (pos == std::string::npos) {
        result << remaining;
        break;
      } else {
        result << remaining.substr(0, pos);
        result << replacement;
        remaining.erase(0, pos + search.length());
      }
    }

    return result.str();
  }

  std::optional<std::pair<std::string,std::string>> SplitStringToPair(const std::string& str,
                                                                      const std::string& separator)
  {
    assert(!separator.empty());

    std::string::size_type pos = str.find(separator);

    if (pos == std::string::npos) {
      return std::nullopt;
    }
    return std::make_pair(str.substr(0, pos),
                          str.substr(pos + separator.length()));
  }

  std::string GetFirstInStringList(const std::string& stringList,
                                   const std::string& divider)
  {
    assert(!stringList.empty());
    assert(!divider.empty());

    std::string::size_type pos=stringList.find_first_of(divider);

    if (pos==std::string::npos) {
      return stringList;
    }

    return stringList.substr(0,pos);
  }

  void TokenizeString(const std::string& input,
                      std::list<std::string>& tokens)
  {
    std::string::size_type wordBegin=0;
    std::string::size_type wordEnd=0;

    while (wordBegin<input.length()) {
      while (wordBegin<input.length() &&
             (std::isspace(input[wordBegin])!=0 ||
              input[wordBegin]==',')) {
        wordBegin++;
      }

      if (wordBegin>=input.length()) {
        return;
      }

      wordEnd=wordBegin;

      while (wordEnd+1<input.length() &&
             (std::isspace(input[wordEnd+1])==0 &&
              input[wordEnd+1]!=',')) {
        wordEnd++;
      }

      std::string token=input.substr(wordBegin,wordEnd-wordBegin+1);

      tokens.push_back(token);

      wordBegin=wordEnd+1;
    }
  }

  void SimplifyTokenList(std::list<std::string>& tokens)
  {
    auto current=tokens.begin();
    auto next=current;

    next++;
    while (next!=tokens.end()) {
      if (std::isupper(current->at(0))!=0 &&
          std::islower(next->at(0))!=0) {
        current->append(" ");
        current->append(*next);
        next=tokens.erase(next);
      }
      else {
        current=next;
        ++next;
      }
    }
  }

  std::string GetTokensFromStart(const std::list<std::string>& tokens,
                                 size_t count)
  {
    assert(count<=tokens.size());

    std::string result;
    auto        startToken=tokens.begin();
    auto        currentToken=startToken;

    for (size_t i=0; i<count; i++) {
      if (currentToken!=startToken) {
        result+=' ';
      }

      result.append(*currentToken);

      ++currentToken;
    }

    return result;
  }

  std::string GetTokensFromEnd(const std::list<std::string>& tokens,
                               size_t count)
  {
    assert(count<=tokens.size());

    std::string result;
    auto        startToken=tokens.begin();

    std::advance(startToken,tokens.size()-count);

    auto        currentToken=startToken;

    for (size_t i=0; i<count; i++) {
      if (currentToken!=startToken) {
        result+=' ';
      }

      result+=*currentToken;

      ++currentToken;
    }

    return result;
  }

  void GroupStringListToStrings(std::list<std::string>::const_iterator token,
                                size_t listSize,
                                size_t parts,
                                std::list<std::list<std::string> >& lists)
  {
    if (parts==1) {
      std::string value;

      for (size_t i=1; i<=listSize; i++) {
        if (!value.empty()) {
          value+=" ";
        }

        value+=*token;

        ++token;
      }

      std::list<std::string> list;

      lists.push_back(list);
      lists.back().push_front(value);

      return;
    }

    for (size_t i=1; i<=listSize-parts+1; i++) {
      size_t      count=0;
      std::string value;
      auto        t=token;

      while (count<i) {
        if (!value.empty()) {
          value+=" ";
        }

        value+=*t;

        ++count;
        ++t;
      }

      std::list<std::list<std::string> > restTokens;

      GroupStringListToStrings(t,
                               listSize-i,
                               parts-1,
                               restTokens);

      for (std::list<std::list<std::string> >::const_iterator rest=restTokens.begin();
          rest!=restTokens.end();
          ++rest) {

        lists.push_back(*rest);
        lists.back().push_front(value);
      }
    }
  }

  std::wstring LocaleStringToWString(const std::string& text)
  {
    std::mbstate_t state;

    const char     *in=text.c_str();
    wchar_t        *out=new wchar_t[text.length()+2];
    size_t         size;
    std::wstring   result;

    memset(&state,0,sizeof(state));

    size=std::mbsrtowcs(out,&in,text.length()+2,&state);
    if (size!=(size_t)-1 && in==nullptr) {
      result=std::wstring(out,size);
    }

    delete [] out;

    return result;
  }

  std::string WStringToLocaleString(const std::wstring& text)
  {
    std::mbstate_t state;

    const wchar_t  *in=text.c_str();
    char           *out=new char[text.length()*4+1];
    size_t         size;
    std::string    result;

    memset(&state,0,sizeof(state));

    size=std::wcsrtombs(out,&in,text.length()*4+1,&state);
    if (size!=(size_t)-1 && in==nullptr) {
      result=std::string(out,size);
    }

    delete [] out;

    return result;
  }

  std::wstring UTF8StringToWString(const std::string& text)
  {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;

    return conv.from_bytes(text);
  }

  std::u32string UTF8StringToU32String(const std::string& text)
  {
#if defined(_MSC_VER) && _MSC_VER >= 1900
    // See https://stackoverflow.com/questions/30765256/linker-error-using-vs-2015-rc-cant-find-symbol-related-to-stdcodecvt
    std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t> conv;
    return reinterpret_cast<const char32_t*>(conv.from_bytes(text).c_str());
#else
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    return conv.from_bytes(text);
#endif
  }

  std::string WStringToUTF8String(const std::wstring& text)
  {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;

    return conv.to_bytes(text);
  }

  std::string LocaleStringToUTF8String(const std::string& text)
  {
    return WStringToUTF8String(LocaleStringToWString(text));
  }

  std::string UTF8StringToLocaleString(const std::string& text)
  {
    return WStringToLocaleString(UTF8StringToWString(text));
  }

  std::string UTF8StringToUpper(const std::string& text)
  {
    return utf8helper::UTF8ToUpper(text);
  }

  std::string UTF8StringToLower(const std::string& text)
  {
    return utf8helper::UTF8ToLower(text);
  }

  std::string UTF8NormForLookup(const std::string& text)
  {
    return utf8helper::UTF8Normalize(text);
  }

  std::string UTF8Transliterate(const std::string& text)
  {
    return utf8helper::UTF8Transliterate(text);
  }

  /**
   * returns the utc timezone offset
   * (e.g. -8 hours for PST)
   */
  int GetUtcOffset() {

    time_t zero = 24*60*60L;
    struct tm * timeptr;
    int gmtimeHours;

    // get the local time for Jan 2, 1900 00:00 UTC
    timeptr = localtime( &zero );
    gmtimeHours = timeptr->tm_hour;

    // if the local time is the "day before" the UTC, subtract 24 hours
    // from the hours to get the UTC offset
    if(timeptr->tm_mday < 2) {
      gmtimeHours -= 24;
    }

    return gmtimeHours;
  }

  /**
   * https://stackoverflow.com/a/9088549/1632737
   *
   * the utc analogue of mktime,
   * (much like timegm on some systems)
   */
  time_t MkTimeUTC(struct tm *timeptr) {
    // gets the epoch time relative to the local time zone,
    // and then adds the appropriate number of seconds to make it UTC
    return mktime(timeptr) + GetUtcOffset() * 3600;
  }

  bool ParseISO8601TimeString(const std::string &timeStr, Timestamp &timestamp)
  {
    using namespace std::chrono;

    // ISO 8601 allows milliseconds in date optionally
    // but std::get_time provides just second accuracy
    // so, we use sscanf for parse string and add
    // milliseconds timestamp later
    int y,M,d,h,m,s,mill;
    int ret=std::sscanf(timeStr.c_str(), "%d-%d-%dT%d:%d:%d.%dZ", &y, &M, &d, &h, &m, &s, &mill);
    if (ret<6){
      return false;
    }

    std::tm time{};

    time.tm_year = y - 1900; // Year since 1900
    time.tm_mon = M - 1;     // 0-11
    time.tm_mday = d;        // 1-31
    time.tm_hour = h;        // 0-23
    time.tm_min = m;         // 0-59
    time.tm_sec = s;         // 0-60

    std::time_t tt = MkTimeUTC(&time);

    time_point<system_clock, nanoseconds> timePoint=system_clock::from_time_t(tt);
    timestamp=time_point_cast<milliseconds,system_clock,nanoseconds>(timePoint);
    // add milliseconds
    if (ret>6) {
      timestamp += milliseconds(mill);
    }

    return true;
  }

  std::string TimestampToISO8601TimeString(const Timestamp &timestamp){
    using namespace std::chrono;

    std::ostringstream stream;

    std::time_t tt = system_clock::to_time_t(timestamp);
    std::tm tm = *std::gmtime(&tt);

    std::array<char, 64> buff;
    std::strftime(buff.data(), buff.size(), "%Y-%m-%dT%H:%M:%S", &tm);

    stream << buff.data();

    // add milliseconds
    auto millisFromEpoch = duration_cast<milliseconds>(timestamp.time_since_epoch()).count();
    stream << ".";
    stream << std::setfill('0') << std::setw(3) << (millisFromEpoch - ((millisFromEpoch / 1000) * 1000));
    stream << "Z";
    return stream.str();
  }

  std::string DurationString(const Duration &duration)
  {
    using namespace std::chrono;

    std::ostringstream stream;
    seconds::rep secondsVal = duration_cast<seconds>(duration).count();
    if (secondsVal < 60){
      stream << std::fixed << std::setprecision(1) << secondsVal << " s";
      return stream.str();
    }

    double hours   = std::floor(secondsVal / 3600);
    double rest    = secondsVal - (hours * 3600);
    double minutes = std::floor(rest / 60);
    double sec     = std::floor(rest - (minutes * 60));

    stream << std::setfill('0') << std::setw(2) << hours
           << ":"
           << std::setfill('0') << std::setw(2) << minutes
           << ":"
           << std::setfill('0') << std::setw(2) << sec;

    return stream.str();
  }

  std::string Trim(const std::string &str, char trimmedChar)
  {
    auto begin=str.begin();
    auto end=str.end();
    while (begin != end && *(end - 1) == trimmedChar) {
      --end;
    }
    while (begin != end && *begin == trimmedChar) {
      ++begin;
    }
    return std::string(begin, end);
  }
}
