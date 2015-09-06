#include <iostream>

#include <osmscout/util/Number.h>

int errors=0;

bool CheckBitsNeededToEncodeNumber(unsigned long number, uint8_t expectedBits)
{
  uint8_t actualBits=osmscout::BitsNeededToEncodeNumber(number);

  if (actualBits!=expectedBits) {
    std::cerr << "Error in 'BitsNeededToEncodeNumber' expected " << (size_t)expectedBits << " actual " << (size_t)actualBits << std::endl;
    return false;
  }

  return true;
}

bool CheckBytesNeededToEncodeNumber(unsigned long number, uint8_t expectedBytes)
{
  uint8_t actualBytes=osmscout::BytesNeededToEncodeNumber(number);

  if (actualBytes!=expectedBytes) {
    std::cerr << "Error in 'BitsNeededToEncodeNumber' expected " << (size_t)expectedBytes << " actual " << (size_t)actualBytes << std::endl;
    return false;
  }

  return true;
}

int main()
{
  // Bits

  if (!CheckBitsNeededToEncodeNumber(0,1)) {
    errors++;
  }

  if (!CheckBitsNeededToEncodeNumber(1,1)) {
    errors++;
  }

  if (!CheckBitsNeededToEncodeNumber(7,3)) {
    errors++;
  }

  if (!CheckBitsNeededToEncodeNumber(8,4)) {
    errors++;
  }

  if (!CheckBitsNeededToEncodeNumber(255,8)) {
    errors++;
  }

  if (!CheckBitsNeededToEncodeNumber(256,9)) {
    errors++;
  }

  if (!CheckBitsNeededToEncodeNumber(65535,16)) {
    errors++;
  }

  if (!CheckBitsNeededToEncodeNumber(65536,17)) {
    errors++;
  }

  // Bytes

  if (!CheckBytesNeededToEncodeNumber(0,1)) {
    errors++;
  }

  if (!CheckBytesNeededToEncodeNumber(1,1)) {
    errors++;
  }

  if (!CheckBytesNeededToEncodeNumber(255,1)) {
    errors++;
  }

  if (!CheckBytesNeededToEncodeNumber(256,2)) {
    errors++;
  }

  if (!CheckBytesNeededToEncodeNumber(65535,2)) {
    errors++;
  }

  if (!CheckBytesNeededToEncodeNumber(65536,3)) {
    errors++;
  }

  if (errors!=0) {
    return 1;
  }
  else {
    return 0;
  }
}
