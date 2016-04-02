#include <iostream>

#include <osmscout/util/Number.h>

int errors=0;

bool CheckEncode(uint64_t value,
                 const char* expected, size_t expectedLength)
{
  const size_t bufferLength=10;
  char         buffer[bufferLength];
  size_t       bytes;

  bytes=osmscout::EncodeNumber(value,buffer);

  if (expectedLength!=bytes) {
    std::cerr << "Encoding of '" << value << "' returned wrong length - Expected " << expectedLength << " actual " << bytes << std::endl;
    return false;
  }
  else {
    for (size_t i=0; i<bytes; i++) {
      if (expected[i]!=buffer[i]) {
        std::cerr << "Encoding of '" << value << "' returned wrong data at offset " << i << " - expected " << std::hex << (unsigned short int)expected[i] << " actual " << std::hex << (unsigned short int)buffer[i] << std::endl;

        for (size_t i=0; i<bytes; i++) {
          std::cerr << std::hex << (unsigned int)buffer[i] << " ";
        }
        std::cerr << std::endl;

        return false;
      }
    }
  }

  return true;
}

bool CheckDecode(const char* buffer, uint64_t expected, size_t bytesExpected)
{
  uint64_t value;
  size_t   bytes;

  bytes=osmscout::DecodeNumber(buffer,value);

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

  if (!CheckEncode(3622479373539965697,"\x81\xf6\x9d\xd0\xc2\xb9\xe8\xa2\x32",9)) {
    errors++;
  }

  if (!CheckDecode("\x81\xf6\x9d\xd0\xc2\xb9\xe8\xa2\x32", 3622479373539965697,9)) {
    errors++;
  }

  if (!CheckEncode(3627142814677078785,"\x81\xa6\xc0\xd3\xc2\xe5\x8c\xab\x32",9)) {
    errors++;
  }

  if (!CheckDecode("\x81\xa6\xc0\xd3\xc2\xe5\x8c\xab\x32",3627142814677078785,9)) {
    errors++;
  }

  if (errors!=0) {
    return 1;
  }
  else {
    return 0;
  }
}
