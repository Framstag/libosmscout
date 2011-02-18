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

#include <osmscout/Util.h>

#include <cstdio>
#include <cstdlib>

namespace osmscout {

  NumberSet::Data::~Data()
  {
    // no code
  }

  NumberSet::Refs::Refs()
  {
    for (size_t i=0; i<256; i++) {
      refs[i]=NULL;
    }
  }

  NumberSet::Refs::~Refs()
  {
    for (size_t i=0; i<256; i++) {
      delete refs[i];
    }
  }

  NumberSet::Leaf::Leaf()
  {
    for (size_t i=0; i<32; i++) {
      values[i]=0;
    }
  }

  NumberSet::NumberSet()
  {
    // no code
  }

  NumberSet::~NumberSet()
  {
    // no code
  }

  void NumberSet::Insert(Number value)
  {
    unsigned char byte;
    Refs          *r=&refs;

    byte=value >> (sizeof(Number)-1)*8;

    if (r->refs[byte]==NULL) {
      r->refs[byte]=new Refs();
    }

    for (size_t i=2; i<sizeof(Number)-1; i++) {
      r=static_cast<Refs*>(r->refs[byte]);

      byte=value >> (sizeof(Number)-i)*8;

      if (r->refs[byte]==NULL) {
        r->refs[byte]=new Refs();
      }
    }

    r=static_cast<Refs*>(r->refs[byte]);

    byte=value >> 1*8;

    if (r->refs[byte]==NULL) {
      r->refs[byte]=new Leaf();
    }

    Leaf *l=static_cast<Leaf*>(r->refs[byte]);

    byte=value & 0xff;

    size_t by=byte/32;
    size_t bi=byte%8;

    l->values[by]|=(1 << bi);
  }

  bool NumberSet::IsSet(Number value) const
  {
    unsigned char byte;
    const Refs    *r=&refs;

    byte=value >> (sizeof(Number)-1)*8;

    if (r->refs[byte]==NULL) {
      return false;;
    }

    for (size_t i=2; i<sizeof(Number); i++) {
      r=dynamic_cast<const Refs*>(r->refs[byte]);

      byte=value >> (sizeof(Number)-i)*8;

      if (r->refs[byte]==NULL) {
        return false;
      }
    }

    const Leaf *l=static_cast<const Leaf*>(r->refs[byte]);

    byte=value & 0xff;

    size_t by=byte/32;
    size_t bi=byte%8;

    return l->values[by] & (1 << bi);
  }

  void GetKeysForName(const std::string& name, std::set<uint32_t>& keys)
  {
    for (size_t s=0; s==0 || s+4<=name.length(); s++) {
      uint32_t value=0;

      if (name.length()>s) {
        value=name[s];
      }
      value=value << 8;

      if (name.length()>s+1) {
        value+=name[s+1];
      }
      value=value << 8;

      if (name.length()>s+2) {
        value+=name[s+2];
      }
      value=value << 8;

      if (name.length()>s+3) {
        value+=name[s+3];
      }

      keys.insert(value);
    }
  }

  bool EncodeNumber(unsigned long number,
                    size_t bufferLength,
                    char* buffer,
                    size_t& bytes)
  {
    if (number==0) {
      if (bufferLength==0) {
        return false;
      }

      buffer[0]=0;
      bytes=1;
      return true;
    }
    else {
      bytes=0;

      while (number!=0) {
        char          byte;
        unsigned long rest;

        byte=number & 0xff;
        rest=number >> 8;

        if ((rest!=0 || byte & 0x80)!=0) {
          // If we have something to encode or the high bit is set
          // we need an additional byte and can only decode 7 bit
          byte=byte & 0x7f; // Mask out the lower 7 bytes
          byte=byte| 0x80;  // set the 8th byte to signal that more bytes will follow
          number=number >> 7;

          if (bufferLength==0) {
            return false;
          }

          buffer[bytes]=byte;
          bytes++;
          bufferLength--;
        }
        else {
          if (bufferLength==0) {
            return false;
          }

          number=0; // we are finished!

          buffer[bytes]=byte;
          bytes++;
          bufferLength--;
        }
      }
    }

    return true;
  }


  bool DecodeNumber(const char* buffer, uint32_t& number, size_t& bytes)
  {
    uint32_t mult=0;

    number=0;
    bytes=1;

    while (true) {
      uint32_t add=((*buffer) & 0x7f) << mult;

      number=number | add;

      if (((*buffer) & 0x80)==0) {
        return true;
      }

      bytes++;
      buffer++;
      mult+=7;
    }
  }

  bool GetFileSize(const std::string& filename, long& size)
  {
    FILE *file;

    file=fopen(filename.c_str(),"rb");

    if (file==NULL) {
      return false;
    }

    if (fseek(file,0L,SEEK_END)!=0) {
      fclose(file);

      return false;
    }

    size=ftell(file);

    if (size==-1) {
      fclose(file);

      return false;
    }

    fclose(file);

    return true;
  }

  std::string AppendFileToDir(const std::string& dir, const std::string& file)
  {
#if defined(__WIN32__) || defined(WIN32)
    std::string result(dir);

    if (result.length()>0 && result[result.length()-1]!='\\') {
      result.append("\\");
    }

    result.append(file);

    return result;
#else
    std::string result(dir);

    if (result.length()>0 && result[result.length()-1]!='/') {
      result.append("/");
    }

    result.append(file);

    return result;
#endif
  }
}
