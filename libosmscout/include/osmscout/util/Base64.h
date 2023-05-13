
#ifndef LIBOSMSCOUT_BASE64_H
#define LIBOSMSCOUT_BASE64_H

/**
 * Original source: https://github.com/philipperemy/easy-encryption/blob/master/b64.h
 * licensed under terms of MIT license
 */

#include <cstdint>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>

namespace osmscout {

  constexpr const char *Base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  static inline std::string Base64Encode(const std::vector<char> &in)
  {
    std::string out;

    uint32_t val = 0;
    int32_t valb = -6;
    for (char jj : in) {
      unsigned char c = (unsigned char)jj;
      val = (val << 8) + c;
      valb += 8;
      while (valb >= 0) {
        out.push_back(Base64Chars[(val >> valb) & 0x3F]);
        valb -= 6;
      }
    }
    if (valb > -6) {
      out.push_back(Base64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    while ((out.size() % 4)!=0) {
      out.push_back('=');
    }

    return out;
  }

  static inline std::vector<char> Base64Decode(const std::string &in)
  {
    std::vector<char> out;

    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[Base64Chars[i]] = i;

    uint32_t val = 0;
    int32_t valb = -8;
    for (char c : in) {
      if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
        continue; // skip whitespaces
      }

      if (T[c] == -1) {
        break; // terminate on invalid character
      }

      val = (val << 6) + T[c];
      valb += 6;
      if (valb >= 0) {
        out.push_back(char((val >> valb) & 0xFF));
        valb -= 8;
      }
    }
    return out;
  }
}

#endif //LIBOSMSCOUT_BASE64_H
