#include <exception>
#include <iostream>

#include <osmscout/util/String.h>

int errors=0;

bool CheckCharsetConversion(const std::string& oText)
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

bool CheckToUpper(const std::string& oText,
                  const std::string& eText)
{
  std::cout << "Original: \"" << oText << "\"" << std::endl;
  // Convert to upper
  std::string uText=osmscout::UTF8StringToUpper(oText);
  std::cout << "Upper: \"" << uText << "\" vs \"" << eText << "\"" << std::endl;

  return uText==eText;
}

bool CheckToLower(const std::string& oText,
                  const std::string& eText)
{
  std::cout << "Original: \"" << oText << "\"" << std::endl;
  // Convert to lower
  std::string lText=osmscout::UTF8StringToLower(oText);
  std::cout << "Lower: \"" << lText << "\" vs \"" << eText << "\"" << std::endl;

  return lText==eText;
}

int main()
{
  try {
    std::locale::global(std::locale(""));

    std::cout << "Current locale activated" << std::endl;
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: Cannot set locale: " << e.what() << std::endl;
  }

  // ANSI
  if (!CheckCharsetConversion("abcABC")) {
    errors++;
  }

  // German umlauts upper and lower case and euro-sign
  if (!CheckCharsetConversion("\xc3\x84\xc3\x96\xc3\x9c\xc3\xa4\xc3\xb6\xc3\xbc\xe2\x82\xac")) {
    errors++;
  }

  // ANSI
  if (!CheckToLower("abcABC","abcabc")) {
    errors++;
  }

  // ANSI
  if (!CheckToUpper("abcABC","ABCABC")) {
    errors++;
  }

  // German umlauts upper
  if (!CheckToUpper("\xc3\x84\xc3\x96\xc3\x9c\xc3\xa4\xc3\xb6\xc3\xbc",
                    "\xc3\x84\xc3\x96\xc3\x9c\xc3\x84\xc3\x96\xc3\x9c")) {
    errors++;
  }

  // German umlauts upper and lower case and euro-sign
  if (!CheckToUpper("\xc3\x84\xc3\x96\xc3\x9c\xc3\xa4\xc3\xb6\xc3\xbc\xe2\x82\xac",
                    "\xc3\x84\xc3\x96\xc3\x9c\xc3\x84\xc3\x96\xc3\x9c\xe2\x82\xac")) {
    errors++;
  }

  if (!CheckToLower("\xc3\x84\xc3\x96\xc3\x9c\xc3\xa4\xc3\xb6\xc3\xbc\xe2\x82\xac",
                    "\xc3\xa4\xc3\xb6\xc3\xbc\xc3\xa4\xc3\xb6\xc3\xbc\xe2\x82\xac")) {
    errors++;
  }

  if (errors!=0) {
    return 1;
  }
  else {
    return 0;
  }
}
