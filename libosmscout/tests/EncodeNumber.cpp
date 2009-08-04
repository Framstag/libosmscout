#include <iostream>

#include <osmscout/Util.h>

int errors=0;

bool CheckData(unsigned long value,
               const char* expected, size_t expectedLength)
{
  const size_t bufferLength=5;
  char         buffer[bufferLength];
  size_t       bytes;

  if (!EncodeNumber(value,bufferLength,buffer,bytes)) {
    std::cerr << "Encoding of '" << value << "' failed!" << std::endl;
    return false;
  }

  if (expectedLength!=bytes) {
    std::cerr << "Encoding of '" << value << "' returned wrong length - Expected " << expectedLength << " actual " << bytes << std::endl;
    return false;
  }
  else {
    for (size_t i=0; i<bytes; i++) {
      if (expected[i]!=buffer[i]) {
        std::cerr << "Encoding of '" << value << "' returned wrong data at offset " << i << " - expected " << std::hex << (unsigned short int)expected[i] << " actual " << std::hex << (unsigned short int)buffer[i] << std::endl;
        return false;
      }
    }
  }

  return true;
}

int main()
{
  if (!CheckData(0,"\0",1)) {
    errors++;
  }

  if (!CheckData(1,"\x01",1)) {
    errors++;
  }

  if (!CheckData(127,"\x7f",1)) {
    errors++;
  }

  if (!CheckData(128,"\x80\x01",2)) {
    errors++;
  }

  if (!CheckData(255,"\xff\x01",2)) {
    errors++;
  }

  if (!CheckData(256,"\x80\x02",2)) {
    errors++;
  }

  if (!CheckData(65535,"\xff\xff\x03",3)) {
    errors++;
  }

  if (errors!=0) {
    return 1;
  }
  else {
    return 0;
  }
}
