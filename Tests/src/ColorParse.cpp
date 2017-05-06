#include <iostream>

#include <osmscout/util/Color.h>

bool CheckConversion(const std::string& colorString, const osmscout::Color& expectedColor)
{
  osmscout::Color parsedColor=osmscout::Color::FromHexString(colorString);

  std::string printedColor=parsedColor.ToHexString();

  if (colorString==printedColor && parsedColor==expectedColor) {
    return true;
  }
  else {
   std::cerr << "Failure: original: " << colorString << " RGBA: " << parsedColor.GetR() << " " << parsedColor.GetG() << " " << parsedColor.GetB() << " " << parsedColor.GetA() << " printed: " << printedColor << " expected: " << expectedColor.ToHexString() << std::endl;
    return false;
  }
}

int main()
{
  int errors=0;

  osmscout::Color color=osmscout::Color::FromHexString("#4440ec");
  osmscout::Color darkenColor=color.Darken(0.4);

  std::cout << color.ToHexString() << " " << darkenColor.ToHexString() << std::endl;

  if (!CheckConversion("#000000",
                       osmscout::Color::BLACK)) {
    errors++;
  }

  if (!CheckConversion("#ffffff",
                       osmscout::Color::WHITE)) {
    errors++;
  }

  if (!CheckConversion("#ff0000",
                       osmscout::Color::RED)) {
    errors++;
  }

  if (!CheckConversion("#00ff00",
                       osmscout::Color::GREEN)) {
    errors++;
  }

  if (!CheckConversion("#0000ff",
                       osmscout::Color::BLUE)) {
    errors++;
  }

  if (errors!=0) {
    return 1;
  }
  else {
    return 0;
  }
}
