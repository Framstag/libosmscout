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

#include <osmscout/util/NumberSet.h>

#include <stdint.h>
#include <stdio.h>

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
}
