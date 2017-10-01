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

#include <osmscout/system/Math.h>

#include <osmscout/util/Logger.h>

#include <osmscout/private/Config.h>

#if defined(HAVE_CODECVT)
#include <codecvt>
#endif

#if defined(HAVE_ICONV)
#include <iconv.h>
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

      result=result+*currentToken;

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

#if defined(HAVE_ICONV)
  std::wstring UTF8StringToWString(const std::string& text)
  {
    std::wstring res;
    iconv_t      handle;

    handle=iconv_open("WCHAR_T","UTF-8");
    if (handle==(iconv_t)-1) {
      log.Error() << "Error in UTF8StringToWString()" << strerror(errno);
      return L"";
    }

    // length+1+1 to handle a potential BOM if ICONV_WCHAR_T is UTF-16 and to convert of \0
    size_t inCount=text.length()+1;
    size_t outCount=(text.length()+2)*sizeof(wchar_t);

    char    *in=const_cast<char*>(text.data());
    wchar_t *out=new wchar_t[text.length()+2];

    char *tmpOut=(char*)out;
    size_t tmpOutCount=outCount;
    if (iconv(handle,(ICONV_CONST char**)&in,&inCount,&tmpOut,&tmpOutCount)==(size_t)-1) {
      iconv_close(handle);
      delete [] out;
      log.Error() << "Error in UTF8StringToWString()" << strerror(errno);
      return L"";
    }

    iconv_close(handle);

    // remove potential byte order marks
    if (sizeof(wchar_t)==4) {
      // strip off potential BOM if ICONV_WCHAR_T is UTF-32
      if (out[0]==0xfeff) {
        res=std::wstring(out+1,(outCount-tmpOutCount)/sizeof(wchar_t)-2);
      }
      else {
        res=std::wstring(out,(outCount-tmpOutCount)/sizeof(wchar_t)-1);
      }
    }
    else if (sizeof(wchar_t)==2) {
      // strip off potential BOM if ICONV_WCHAR_T is UTF-16
      if (out[0]==0xfeff || out[0]==0xfffe) {
        res=std::wstring(out+1,(outCount-tmpOutCount)/sizeof(wchar_t)-2);
      }
      else {
        res=std::wstring(out,(outCount-tmpOutCount)/sizeof(wchar_t)-1);
      }
    }
    else {
      res=std::wstring(out,(outCount-tmpOutCount)/sizeof(wchar_t)-1);
    }

    delete [] out;

    return res;
  }
#elif defined(HAVE_CODECVT)
  std::wstring UTF8StringToWString(const std::string& text)
  {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;

    return conv.from_bytes(text);
  }
#else
  #error "Missing implementation for std::wstring UTF8StringToWString(const std::string& text)"
#endif

#if defined(HAVE_ICONV)
  std::u32string UTF8StringToU32String(const std::string& text)
  {
    std::u32string res;
    iconv_t        handle;

    handle=iconv_open("UTF-32","UTF-8");
    if (handle==(iconv_t)-1) {
      log.Error() << "Error returned by iconv_open() in UTF8StringToU32String(): " << strerror(errno);
      return std::u32string();
    }

    // length+1+1 to handle a potential BOM and to convert of \0
    size_t inCount=text.length()+1;
    size_t outCount=(text.length()+2)*sizeof(char32_t);

    char     *in=const_cast<char*>(text.data());
    char32_t *out=new char32_t[text.length()+2];

    char *tmpOut=(char*)out;
    size_t tmpOutCount=outCount;
    if (iconv(handle,(ICONV_CONST char**)&in,&inCount,&tmpOut,&tmpOutCount)==(size_t)-1) {
      iconv_close(handle);
      delete [] out;
      log.Error() << "Error returned by iconv() in UTF8StringToU32String():" << strerror(errno);
      return std::u32string();
    }

    iconv_close(handle);

    // remove potential byte order marks
    if (out[0]==0xfeff) {
      res=std::u32string(out+1,(outCount-tmpOutCount)/sizeof(char32_t)-2);
    }
    else {
      res=std::u32string(out,(outCount-tmpOutCount)/sizeof(char32_t)-1);
    }

    delete [] out;

    return res;
  }
#elif defined(HAVE_CODECVT)
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
#else
  #error "Missing implementation for std::wstring UTF8StringToU32String(const std::string& text)"
#endif

#if defined(HAVE_ICONV)
  std::string WStringToUTF8String(const std::wstring& text)
  {
    iconv_t handle;

    handle=iconv_open("UTF-8","WCHAR_T");
    if (handle==(iconv_t)-1) {
      log.Error() << "Error iconv_open in WStringToUTF8String(): " << strerror(errno);
      return "";
    }

    // length+1 to get the result '\0'-terminated
    size_t inCount=(text.length()+1)*sizeof(wchar_t);
    size_t outCount=text.length()*4+1; // Up to 4 bytes for one UTF-8 character

    char *in=const_cast<char*>((const char*)text.c_str());
    char *out=new char[outCount];

    char   *tmpOut=out;
    size_t tmpOutCount=outCount;

    if (iconv(handle,(ICONV_CONST char**)&in,&inCount,&tmpOut,&tmpOutCount)==(size_t)-1) {
      iconv_close(handle);
      delete [] out;
      log.Error() << "Error iconv in WStringToUTF8String() " << strerror(errno);
      return "";
    }

    std::string res=out;

    delete [] out;

    iconv_close(handle);

    return res;
  }
#elif defined(HAVE_CODECVT)
  std::string WStringToUTF8String(const std::wstring& text)
  {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;

    return conv.to_bytes(text);
  }
#else
  #error "Missing implementation for std::string WStringToUTF8String(const std::wstring& text)"
#endif

#if defined(HAVE_ICONV)
  std::string LocaleStringToUTF8String(const std::string& text)
  {
    iconv_t handle;

    handle=iconv_open("UTF-8","");
    if (handle==(iconv_t)-1) {
      log.Error() << "Error iconv_open in LocaleStringToUTF8String() " << strerror(errno);
      return "";
    }

    // length+1 to get the result '\0'-terminated
    size_t inCount=text.length()+1;
    size_t outCount=text.length()*4+1; // Up to 4 bytes for one UTF-8 character

    char *in=const_cast<char*>(text.data());
    char *out=new char[outCount];

    char   *tmpOut=out;
    size_t tmpOutCount=outCount;

    if (iconv(handle,(ICONV_CONST char**)&in,&inCount,&tmpOut,&tmpOutCount)==(size_t)-1) {
      iconv_close(handle);
      delete [] out;
      log.Error() << "Error iconv in LocaleStringToUTF8String() " << strerror(errno);
      return "";
    }

    std::string res=out;

    delete [] out;

    iconv_close(handle);

    return res;
  }
#else
  std::string LocaleStringToUTF8String(const std::string& text)
  {
    return WStringToUTF8String(LocaleStringToWString(text));
  }
#endif

#if defined(HAVE_ICONV)
  std::string UTF8StringToLocaleString(const std::string& text)
  {
    iconv_t handle;

    handle=iconv_open("","UTF-8");
    if (handle==(iconv_t)-1) {
      log.Error() << "Error iconv_open in UTF8StringToLocaleString() " << strerror(errno);
      return "";
    }

    // length+1 to get the result '\0'-terminated
    size_t inCount=text.length()+1;
    size_t outCount=text.length()*4+2; // Up to 4 bytes for one UTF-8 character and space for a byte order mark

    char *in=const_cast<char*>(text.data());
    char *out=new char[outCount];

    char   *tmpOut=out;
    size_t tmpOutCount=outCount;

    if (iconv(handle,(ICONV_CONST char**)&in,&inCount,&tmpOut,&tmpOutCount)==(size_t)-1) {
      iconv_close(handle);
      delete [] out;
      log.Error() << "Error iconv in UTF8StringToLocaleString() " << strerror(errno);
      return "";
    }

    std::string res=out;

    delete [] out;

    iconv_close(handle);

    return res;
  }
#else
  std::string UTF8StringToLocaleString(const std::string& text)
  {
    return WStringToLocaleString(UTF8StringToWString(text));
  }
#endif

  std::string UTF8StringToUpper(const std::string& text)
  {
    //std::cout << "* O \"" << text << std::endl;

    std::wstring wstr=UTF8StringToWString(text);
    //std::wcout << "* w \"" << wstr << std::endl;

    auto& f=std::use_facet<std::ctype<wchar_t>>(std::locale());

    f.toupper(&wstr[0],&wstr[0]+wstr.size());
    //std::wcout << "* c \"" << wstr << std::endl;

    return WStringToUTF8String(wstr);
  }

  std::string UTF8StringToLower(const std::string& text)
  {
    //std::cout << "* O \"" << text << std::endl;

    std::wstring wstr=UTF8StringToWString(text);
    //std::wcout << "* w \"" << wstr << std::endl;

    auto& f=std::use_facet<std::ctype<wchar_t>>(std::locale());

    f.tolower(&wstr[0],&wstr[0]+wstr.size());
    //std::wcout << "* c \"" << wstr << std::endl;

    return WStringToUTF8String(wstr);
  }

  std::string UTF8NormForLookup(const std::string& text)
  {
    std::wstring wstr=UTF8StringToWString(text);

    auto& f=std::use_facet<std::ctype<wchar_t>>(std::locale());

    // convert to lower and replace all whitespaces with simple space
    std::transform(wstr.begin(), wstr.end(), wstr.begin(),
                   [&f](wchar_t c) -> wchar_t {
                     // std::iswspace don't recognize no-break space and others
                     // so-space characters as space -> we are using switch here...
                     switch(c){
                       case ' ':
                       case L'\u0009': // tabular
                       case L'\u00A0': // no-break space (&nbsp;)
                       case L'\u2007': // figure space
                       case L'\u202F': // narrow no-break space
                         return ' ';
                       default:
                         return f.tolower(c); // to lower
                     }});

    // TODO: remove multiple following spaces by one

    return WStringToUTF8String(wstr);
  }
}
