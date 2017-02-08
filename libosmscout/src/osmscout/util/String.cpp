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
#include <iomanip>
#include <locale>
#include <sstream>

#include <osmscout/system/Math.h>

#include <osmscout/private/Config.h>

#if defined(HAVE_CODECVT)
#include <codecvt>
#endif
#include <iostream>
namespace osmscout {


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

  std::string StringListToString(const std::list<std::string>& list,
                                 const std::string& separator)
  {
    std::string result;

    for (std::list<std::string>::const_iterator element=list.begin();
        element!=list.end();
        ++element) {
      if (element==list.begin()) {
       result.append(*element);
      }
      else {
        result.append(separator);
        result.append(*element);
      }
    }

    return result;
  }

  std::string ByteSizeToString(FileOffset value)
  {
    return ByteSizeToString((double)value);
  }

  std::string ByteSizeToString(double value)
  {
    std::stringstream buffer;

    buffer.setf(std::ios::fixed);
    buffer << std::setprecision(1);

    if (value<1.0 && value>-1) {
      buffer << "0 B";
    }
    else if (ceil(value)>=1024.0*1024*1024*1024) {
      buffer << value/(1024.0*1024*1024*1024) << " TiB";
    }
    else if (ceil(value)>=1024.0*1024*1024) {
      buffer << value/(1024.0*1024*1024) << " GiB";
    }
    else if (ceil(value)>=1024.0*1024) {
      buffer << value/(1024.0*1024) << " MiB";
    }
    else if (ceil(value)>=1024.0) {
      buffer << value/1024.0 << " KiB";
    }
    else {
      buffer << std::setprecision(0);
      buffer << value << " B";
    }

    return buffer.str();
  }

  void SplitStringAtSpace(const std::string& input,
                          std::list<std::string>& tokens)
  {
    std::string::size_type wordBegin=0;
    std::string::size_type wordEnd=0;

    while (wordBegin<input.length()) {
      while (wordBegin<input.length() &&
             std::isspace(input[wordBegin])) {
        wordBegin++;
      }

      if (wordBegin>=input.length()) {
        return;
      }

      wordEnd=wordBegin;

      while (wordEnd+1<input.length() &&
             !std::isspace((unsigned char)input[wordEnd + 1])) {
        wordEnd++;
      }

      std::string token=input.substr(wordBegin,wordEnd-wordBegin+1);

      tokens.push_back(token);

      wordBegin=wordEnd+1;
    }
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
    else {
      return stringList.substr(0,pos);
    }
  }

  void TokenizeString(const std::string& input,
                      std::list<std::string>& tokens)
  {
    std::string::size_type wordBegin=0;
    std::string::size_type wordEnd=0;

    while (wordBegin<input.length()) {
      while (wordBegin<input.length() &&
             (std::isspace(input[wordBegin]) ||
              input[wordBegin]==',')) {
        wordBegin++;
      }

      if (wordBegin>=input.length()) {
        return;
      }

      wordEnd=wordBegin;

      while (wordEnd+1<input.length() &&
             (!std::isspace(input[wordEnd+1]) &&
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
    std::list<std::string>::iterator current=tokens.begin();
    std::list<std::string>::iterator next=current;

    next++;
    while (next!=tokens.end()) {
      if (std::isupper(current->at(0)) &&
          std::islower(next->at(0))) {
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
      size_t                                 count=0;
      std::string                            value;
      std::list<std::string>::const_iterator t=token;

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

#if defined(HAVE_CODECVT)
  std::wstring UTF8StringToWString(const std::string& text)
  {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;

    return conv.from_bytes(text);
  }

  std::string WStringToUTF8String(const std::wstring& text)
  {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;

    return conv.to_bytes(text);
  }
#else
  /**
    The following string conversion code is a modified version of code copied
    from the source of the ConvertUTF tool, as can be found for example at
    http://www.unicode.org/Public/PROGRAMS/CVTUTF/

    It is free to copy and use.
  */

  #define UNI_SUR_HIGH_START  0xD800
  #define UNI_SUR_HIGH_END    0xDBFF
  #define UNI_SUR_LOW_START   0xDC00
  #define UNI_SUR_LOW_END     0xDFFF
  #define UNI_MAX_BMP         0x0000FFFF
  #define UNI_MAX_UTF16       0x0010FFFF
  #define UNI_MAX_LEGAL_UTF32 0x0010FFFF

  static const unsigned long offsetsFromUTF8[6] = {
    0x00000000UL, 0x00003080UL, 0x000E2080UL,
    0x03C82080UL, 0xFA082080UL, 0x82082080UL
  };

  static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

  static const char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
  };

  std::string WStringToUTF8String(const std::wstring& text)
  {
    std::string         result;
    char                buffer[4];
    const unsigned long byteMask=0xBF;
    const unsigned long byteMark=0x80;

    result.reserve(text.length()*4);

#if SIZEOF_WCHAR_T==4

    for (size_t i=0; i<text.length(); i++) {
      wchar_t             ch=text[i];
      unsigned short      bytesToWrite = 0;

      /* UTF-16 surrogate values are illegal in UTF-32 */
      if (ch>=UNI_SUR_HIGH_START && ch<=UNI_SUR_LOW_END) {
        return result;
      }

      /*
       * Figure out how many bytes the result will require. Turn any
       * illegally large UTF32 things (> Plane 17) into replacement chars.
      */
      if (ch<0x80) {
        bytesToWrite=1;
      }
      else if (ch<0x800) {
        bytesToWrite=2;
      }
      else if (ch<0x10000) {
        bytesToWrite=3;
      }
      else if (ch<=UNI_MAX_LEGAL_UTF32) {
        bytesToWrite=4;
      }
      else {
        return result;
      }

      switch (bytesToWrite) { /* note: everything falls through. */
      case 4:
        buffer[3]=(char)((ch | byteMark) & byteMask);
        ch>>=6;
      case 3:
        buffer[2]=(char)((ch | byteMark) & byteMask);
        ch>>=6;
      case 2:
        buffer[1]=(char)((ch | byteMark) & byteMask);
        ch>>=6;
      case 1:
        buffer[0]=(char)(ch | firstByteMark[bytesToWrite]);
      }

      result.append(buffer,bytesToWrite);
    }

#elif SIZEOF_WCHAR_T==2

    static const int halfShift=10; /* used for shifting by 10 bits */
    static const unsigned long halfBase=0x0010000UL;

    const wchar_t* source=text.c_str();

    while (source!=text.c_str()+text.length()) {
      wchar_t             ch;
      unsigned short      bytesToWrite=0;

      ch=*source++;

      /* If we have a surrogate pair, convert to UTF32 first. */
      if (ch>=UNI_SUR_HIGH_START && ch<=UNI_SUR_HIGH_END) {
        if (source>=text.c_str()+text.length()) {
          return result;
        }
        /* If the 16 bits following the high surrogate are in the source buffer... */
        unsigned long ch2 = *source++;
        /* If it's a low surrogate, convert to UTF32. */
        if (ch2>=UNI_SUR_LOW_START && ch2<=UNI_SUR_LOW_END) {
          ch=(wchar_t)(((ch-UNI_SUR_HIGH_START) << halfShift) + (ch2-UNI_SUR_LOW_START) + halfBase);
        }
        else { /* it's an unpaired high surrogate */
          return result;
        }
      }
      else {
        /* UTF-16 surrogate values are illegal in UTF-32 */
        if (ch>=UNI_SUR_LOW_START && ch<=UNI_SUR_LOW_END) {
          return result;
        }
      }

      /* Figure out how many bytes the result will require */
      if (ch<0x80) {
        bytesToWrite=1;
      }
      else if (ch<0x800) {
        bytesToWrite=2;
      }
      else if (ch<0x10000) {
        bytesToWrite=3;
      }
      else if (ch<0x110000) {
        bytesToWrite=4;
      }
      else {
        return result;
      }

      switch (bytesToWrite) { /* note: everything falls through. */
      case 4:
        buffer[3]=(char)((ch | byteMark) & byteMask);
        ch>>=6;
      case 3:
        buffer[2]=(char)((ch | byteMark) & byteMask);
        ch>>=6;
      case 2:
        buffer[1]=(char)((ch | byteMark) & byteMask);
        ch>>=6;
      case 1:
        buffer[0]=(char)(ch | firstByteMark[bytesToWrite]);
      }

      result.append(buffer,bytesToWrite);
    }
#endif

    return result;
  }

  std::wstring UTF8StringToWString(const std::string& text)
  {
    std::wstring result;

    result.reserve(text.length());

#if SIZEOF_WCHAR_T==4
    const char* source=text.c_str();

    while (source!=text.c_str()+text.length()) {
      unsigned long  ch = 0;
      unsigned short extraBytesToRead=trailingBytesForUTF8[(unsigned char)*source];

      /*
       * The cases all fall through. See "Note A" below.
       */
      switch (extraBytesToRead) {
      case 5:
        ch+=(unsigned char)(*source);
        source++;
        ch<<=6;
      case 4:
        ch+=(unsigned char)(*source);
        source++;
        ch<<=6;
      case 3:
        ch+=(unsigned char)(*source);
        source++;
        ch<<=6;
      case 2:
        ch+=(unsigned char)(*source);
        source++;
        ch<<=6;
      case 1:
        ch+=(unsigned char)(*source);
        source++;
        ch<<=6;
      case 0:
        ch+=(unsigned char)(*source);
        source++;
      }
      ch-=offsetsFromUTF8[extraBytesToRead];

      if (ch<=UNI_MAX_LEGAL_UTF32) {
        /*
         * UTF-16 surrogate values are illegal in UTF-32, and anything
         * over Plane 17 (> 0x10FFFF) is illegal.
         */
        if (ch>=UNI_SUR_HIGH_START && ch<=UNI_SUR_LOW_END) {
          return result;
        }
        else {
          result.append(1,(wchar_t)ch);
        }
      }
      else { /* i.e., ch > UNI_MAX_LEGAL_UTF32 */
        return result;
      }
    }
#elif SIZEOF_WCHAR_T==2
    static const int halfShift=10; /* used for shifting by 10 bits */
    static const unsigned long halfBase=0x0010000UL;
    static const unsigned long halfMask=0x3FFUL;

    size_t idx=0;

    while (idx<text.length()) {
      unsigned long  ch=0;
      unsigned short extraBytesToRead=trailingBytesForUTF8[(unsigned char)text[idx]];

      /*
       * The cases all fall through. See "Note A" below.
       */

      switch (extraBytesToRead) {
      case 5:
        ch+=(unsigned char)text[idx];
        idx++;
        ch<<=6; /* remember, illegal UTF-8 */
      case 4:
        ch+=(unsigned char)text[idx];
        idx++;
        ch<<=6; /* remember, illegal UTF-8 */
      case 3:
        ch+=(unsigned char)text[idx];
        idx++;
        ch<<=6;
      case 2:
        ch+=(unsigned char)text[idx];
        idx++;
        ch<<=6;
      case 1:
        ch+=(unsigned char)text[idx];
        idx++;
        ch<<=6;
      case 0:
        ch+=(unsigned char)text[idx];
        idx++;
      }
      ch-=offsetsFromUTF8[extraBytesToRead];

      if (ch <= UNI_MAX_BMP) { /* Target is a character <= 0xFFFF */
        /* UTF-16 surrogate values are illegal in UTF-32 */
        if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) {
          return result;
        }
        else {
          result.append(1,(wchar_t)ch); /* normal case */
        }
      }
      else if (ch > UNI_MAX_UTF16) {
        return result;
      }
      else {
        /* target is a character in range 0xFFFF - 0x10FFFF. */
        ch -= halfBase;
        result.append(1,(wchar_t)((ch >> halfShift) + UNI_SUR_HIGH_START));
        result.append(1,(wchar_t)((ch & halfMask) + UNI_SUR_LOW_START));
      }
    }
#endif

    return result;
  }
#endif

  std::string UTF8StringToUpper(const std::string& text)
  {
    std::cout << "* O \"" << text << std::endl;

    std::wstring wstr=UTF8StringToWString(text);
    std::wcout << "* w \"" << wstr << std::endl;

    auto& f=std::use_facet<std::ctype<wchar_t>>(std::locale());

    f.toupper(&wstr[0],&wstr[0]+wstr.size());
    std::wcout << "* c \"" << wstr << std::endl;

    return WStringToUTF8String(wstr);
  }

  std::string UTF8StringToLower(const std::string& text)
  {
    std::cout << "* O \"" << text << std::endl;

    std::wstring wstr=UTF8StringToWString(text);
    std::wcout << "* w \"" << wstr << std::endl;

    auto& f=std::use_facet<std::ctype<wchar_t>>(std::locale());

    f.tolower(&wstr[0],&wstr[0]+wstr.size());
    std::wcout << "* c \"" << wstr << std::endl;

    return WStringToUTF8String(wstr);
  }
}
