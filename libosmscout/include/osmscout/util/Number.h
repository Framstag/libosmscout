#ifndef OSMSCOUT_NUMBER_H
#define OSMSCOUT_NUMBER_H

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

#include <osmscout/CoreImportExport.h>

#include <limits>
#include <cstddef>

#include <osmscout/system/OSMScoutTypes.h>

namespace osmscout {

  /**
   * \ingroup Util
   * Encode a signed number into the given buffer using some variable length encoding.
   *
   * The first bit (if set) signals a negative number. The highest bit in a byte
   * is set, if there is an additional byte following. So we use one bit
   * for signaling signess and use 7 of 8 bytes per byte for data.
   *
   * The current implementation thus requires the buffer to have at least space
   * for sizeof(N)*8/7 + 1/8 bytes:
   *
   * This are 5 bytes for a 32bit value and 10 bytes for a 64bit value.
   *
   * The methods returns the number of bytes written.
   */
  template<typename N>
  inline unsigned int EncodeNumberSigned(N number,
                                         char* buffer)
  {
    unsigned int bytes=1;
    char         val;

    if (number<0) {
      number^=static_cast<N>(-1);
      val=static_cast<char>((number & 0x3f) << 1 | 0x01);
    }
    else {
      val=static_cast<char>((number & 0x3f) << 1);
    }

    number>>=6;

    while (number!=0) {
      *(buffer++)=val | 0x80;
      val=static_cast<char>(number & 0x7f);
      number>>=7;
      bytes++;
    }

    *buffer=val;
    return bytes;
  }

  /**
   * \ingroup Util
   * Encode an unsigned number into the given buffer using some variable length encoding.
   *
   * The highest bit in a byte is set, if there is an additional byte following.
   * So we use 7 of 8 bytes per byte for data.
   *
   * The current implementation requires the buffer to have at least space
   * for sizeof(N)*8/7 bytes:
   *
   * This are 5 bytes for a 32bit value and 10 bytes for a 64bit value.
   *
   * The methods returns the number of bytes written.
   */
  template<typename N>
  inline unsigned int EncodeNumberUnsigned(N number,
                                           char* buffer)
  {
    unsigned int bytes=0;

    while (number>0x7f) {
      buffer[bytes]=static_cast<char>((number & 0x7f) | 0x80);
      number>>=7;
      bytes++;
    }

    buffer[bytes]=static_cast<char>(number);
    bytes++;

    return bytes;
  }

  template<bool is_signed, typename N>
  struct EncodeNumberTemplated
  {
  };

  template<typename N>
  struct EncodeNumberTemplated<true, N>
  {
    static inline unsigned int f(N number,char* buffer)
    {
      return EncodeNumberSigned<N>(number,buffer);
    }
  };

  template<typename N>
  struct EncodeNumberTemplated<false, N>
  {
    static inline unsigned int f(N number,char* buffer)
    {
      return EncodeNumberUnsigned<N>(number,buffer);
    }
  };

  /**
   * \ingroup Util
   * Encode a number into the given buffer using some variable length encoding.
   *
   * The current implementation requires the buffer to have at least space
   * for sizeof(N)*8/7 bytes for an unsigned number
   * and sizeof(N)*8/7 + 1/8 bytes for a signed number
   *
   * This are 5 bytes for a 32bit value and 10 bytes for a64bit value.
   *
   * The methods returns the number of bytes written.
   */
  template<typename N>
  inline unsigned int EncodeNumber(N number,
                                   char* buffer)
  {
    return EncodeNumberTemplated<std::numeric_limits<N>::is_signed, N>
      ::f(number,buffer);
  }

  /**
   * \ingroup Util
   * Decode a signed variable length encoded number from the buffer back to
   * the variable.
   *
   * The methods returns the number of bytes read.
   */
  template<typename N>
  inline unsigned int DecodeNumberSigned(const char* buffer,
                                         N& number)
  {
    unsigned int shift=0;
    unsigned int nextShift=0;
    unsigned int bytes=1;

    // negative form
    if ((*buffer & 0x01)!=0) {
      N val=(*buffer & 0x7e) >> 1;

      number=-1;
      nextShift=6;

      while ((*(buffer++) & 0x80)!=0) {
        number^=(val << shift);
        val=*buffer & 0x7f;
        shift=nextShift;
        nextShift+=7;
        bytes++;
      }

      number^=static_cast<N>(val) << shift;
    }
    else {
      N val=(*buffer & 0x7e) >> 1;

      number=0;
      nextShift=6;

      while ((*(buffer++) & 0x80)!=0) {
        number|=(val << shift);
        val=*buffer & 0x7f;
        shift=nextShift;
        nextShift+=7;
        bytes++;
      }

      number|=static_cast<N>(val) << shift;
    }

    return bytes;
  }

  /**
   * \ingroup Util
   * Decode an unsigned variable length encoded number from the buffer back to
   * the variable.
   *
   * The methods returns the number of bytes read.
   */
  template<typename N>
  inline unsigned int DecodeNumberUnsigned(const char* buffer,
                                           N& number)
  {
    unsigned int shift=0;
    unsigned int bytes=1;

    number=0;

    while (true) {
      number|=static_cast<N>(*buffer & 0x7f) << shift;

      if (((*buffer) & 0x80)==0) {
        return bytes;
      }

      bytes++;
      buffer++;
      shift+=7;
    }

    // Only for the compiler...
    return bytes;
  }

  template<bool is_signed, typename N>
  struct DecodeNumberTemplated
  {
  };

  template<typename N>
  struct DecodeNumberTemplated<true, N>
  {
    static inline unsigned int f(const char* buffer, N& number)
    {
      return DecodeNumberSigned<N>(buffer,number);
    }
  };

  template<typename N>
  struct DecodeNumberTemplated<false, N>
  {
    static inline unsigned int f(const char* buffer, N& number)
    {
      return DecodeNumberUnsigned<N>(buffer,number);
    }
  };

  /**
   * \ingroup Util
   * Decode a variable length encoded number from the buffer back to
   * the variable.
   *
   * The methods returns the number of bytes read.
   */
  template<typename N>
  inline unsigned int DecodeNumber(const char* buffer,
                                   N& number)
  {
    return DecodeNumberTemplated<std::numeric_limits<N>::is_signed, N>
      ::f(buffer,number);
  }

  /**
   * \ingroup Util
   * Returns the number of bytes needed to encode the given number of bits.
   */
  template<typename N>
  inline N BitsToBytes(N bits)
  {
    return bits%8==0 ? bits/8 : bits/8+1;
  }

  /**
   * \ingroup Util
   * Returns the number of bytes needed to encode the number. The function calculates
   * the number of bytes that contain information, dropping leading bytes that only
   * contain zero.
   */
  template<typename N>
  uint8_t BytesNeededToEncodeNumber(N number)
  {
    uint8_t bytes=0;

    while (number!=0) {
      number=number/256;
      bytes++;
    }

    if (bytes==0) {
      bytes=1;
    }

    return bytes;
  }

  /**
   * \ingroup Util
   * Returns the number of bytes needed to encode the number. The function calculates
   * the number of bytes that contain information, dropping leading bytes that only
   * contain zero.
   */
  template<typename N>
  uint8_t BitsNeededToEncodeNumber(N number)
  {
    uint8_t bits=0;

    while (number!=0) {
      number=number/2;
      bits++;
    }

    if (bits==0) {
      bits=1;
    }

    return bits;
  }

  /**
   * \ingroup Util
   * Encodes the given numbers into a new number of twice the size that has
   * the bits of each number alternating interleaved.
   *
   * This can be used to convert two dimensional coordinates into
   * one number, where coordinates close in 2D are close in the
   * one dimensional projection, too.
   */
  extern OSMSCOUT_API uint64_t InterleaveNumbers(uint32_t a,
                                                 uint32_t b);
}

#endif
