#include <iostream>

#include <osmscout/util/String.h>

int errors=0;

bool Check(const std::string& oText)
{
  std::cout << "Original: \"" << oText << "\"" << std::endl;
  // Convert to std::wstring
  std::wstring wText=osmscout::UTF8StringToWString(oText);
  std::wcout << "WString: \"" << wText << "\"" << std::endl;
  // ...and convert it back to UTF8
  std::string  uText=osmscout::WStringToUTF8String(wText);
  std::cout << "UTF8String: \"" << uText << "\"" << std::endl;
  
  return oText==uText;
}

int main()
{
  // ANSI
  if (!Check("abcABC")) {
    errors++;
  }

  // German umlauts upper and lower case and euro-sign
  if (!Check("\xc3\x84\xc3\x96\xc3\x9c\xc3\xa4\xc3\xb6\xc3\xbc\xe2\x82\xac")) {
    errors++;
  }



  if (errors!=0) {
    return 1;
  }
  else {
    return 0;
  }
}
