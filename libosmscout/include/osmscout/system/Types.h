#ifndef OSMSCOUT_SYSTEM_TYPES_H
#define OSMSCOUT_SYSTEM_TYPES_H

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

#include <osmscout/CoreFeatures.h>

#if defined(OSMSCOUT_HAVE_STDINT_H)
  // will be <cstdint> in c++0x
  #include <stdint.h>
#elif defined(__WIN32__) || defined(WIN32)
  #include <stdio.h>

  typedef          __int8 int8_t;
  typedef unsigned __int8 uint8_t;

  typedef          __int16 int16_t;
  typedef unsigned __int16 uint16_t;

  typedef          __int32 int32_t;
  typedef unsigned __int32 uint32_t;
#endif  

#endif
