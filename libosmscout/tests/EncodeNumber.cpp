#include <iostream>

#include <osmscout/Util.h>

int errors=0;

bool CheckEncode(unsigned long value,
                 const char* expected, size_t expectedLength)
{
  const size_t bufferLength=5;
  char         buffer[bufferLength];
  size_t       bytes;

  if (!osmscout::EncodeNumber(value,bufferLength,buffer,bytes)) {
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

bool CheckDecode(const char* buffer, unsigned long expected, size_t bytesExpected)
{
  uint32_t value;
  size_t   bytes;

  if (!osmscout::DecodeNumber(buffer,value,bytes)) {
    std::cerr << "Decoding of '" << expected << "' failed!" << std::endl;
    return false;
  }

  if (value!=expected) {
    std::cerr << "Error in decoding: expected " << expected << " actual " << value << std::endl;
    return false;
  }

  if (bytes!=bytesExpected) {
    std::cerr << "Error in decoding: expected " << bytesExpected << "bytes read, actual " << bytes << std::endl;
    return false;
  }

  return true;
}

int main()
{
  if (!CheckEncode(0,"\0",1)) {
    errors++;
  }

  if (!CheckDecode("\0",0,1)) {
    errors++;
  }

  if (!CheckEncode(1,"\x01",1)) {
    errors++;
  }

  if (!CheckDecode("\x01",1,1)) {
    errors++;
  }

  if (!CheckEncode(127,"\x7f",1)) {
    errors++;
  }

  if (!CheckDecode("\x7f",127,1)) {
    errors++;
  }

  if (!CheckEncode(128,"\x80\x01",2)) {
    errors++;
  }

  if (!CheckDecode("\x80\x01",128,2)) {
    errors++;
  }

  if (!CheckEncode(213,"\xd5\x01",2)) {
    errors++;
  }

  if (!CheckDecode("\xd5\x01",213,2)) {
    errors++;
  }

  if (!CheckEncode(255,"\xff\x01",2)) {
    errors++;
  }

  if (!CheckDecode("\xff\x01",255,2)) {
    errors++;
  }

  if (!CheckEncode(256,"\x80\x02",2)) {
    errors++;
  }

  if (!CheckDecode("\x80\x02",256,2)) {
    errors++;
  }

  if (!CheckEncode(65535,"\xff\xff\x03",3)) {
    errors++;
  }

  if (!CheckDecode("\xff\xff\x03",65535,3)) {
    errors++;
  }

  if (errors!=0) {
    return 1;
  }
  else {
    return 0;
  }
}
