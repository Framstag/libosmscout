#include <iostream>

#include <osmscout/util/NumberSet.h>

int errors=0;

int main()
{
  osmscout::NumberSet set;

  if (set.IsSet(0)) {
    std::cerr << "0 found in set!" << std::endl;
    errors++;
  }

  if (set.IsSet(1)) {
    std::cerr << "1 found in set!" << std::endl;
    errors++;
  }

  set.Insert(1);

  if (!set.IsSet(1)) {
    std::cerr << "1 not found in set!" << std::endl;
    errors++;
  }

  set.Insert(255);

  if (!set.IsSet(255)) {
    std::cerr << "255 not found in set!" << std::endl;
    errors++;
  }

  set.Insert(256);

  if (!set.IsSet(256)) {
    std::cerr << "256 not found in set!" << std::endl;
    errors++;
  }

  for (size_t i=256; i<256*256; i++) {
    set.Insert(i);

    if (!set.IsSet(i)) {
      std::cerr << i << " not found in set!" << std::endl;
      errors++;
    }
  }

  if (errors!=0) {
    return 1;
  }
  else {
    return 0;
  }
}
