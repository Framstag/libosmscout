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

#include <stddef.h>

#if defined(OSMSCOUT_HAVE_STDINT_H)
  // will be <cstdint> in c++0x
  #include <stdint.h>

  #if !defined(OSMSCOUT_HAVE_INT8_T)
    #error int8_t not defined
  #endif

  #if !defined(OSMSCOUT_HAVE_UINT8_T)
    #error uint8_t not defined
  #endif

  #if !defined(OSMSCOUT_HAVE_INT16_T)
    #error int16_t not defined
  #endif

  #if !defined(OSMSCOUT_HAVE_UINT16_T)
    #error uint16_t not defined
  #endif

  #if !defined(OSMSCOUT_HAVE_INT32_T)
    #error int32_t not defined
  #endif

  #if !defined(OSMSCOUT_HAVE_UINT32_T)
    #error uint32_t not defined
  #endif

  // We do not require int64_t and uint64_t to exist, we
  // fallback to use in32_t and uint32_t in that case

#elif defined(__WIN32__) || defined(WIN32)
  #include <stdio.h>

  #if !defined(OSMSCOUT_HAVE_INT8_T)
    typedef          __int8 int8_t;
    #define OSMSCOUT_HAVE_INT8_T 1
  #endif

  #if !defined(OSMSCOUT_HAVE_UINT8_T)
    typedef unsigned __int8 uint8_t;
    #define OSMSCOUT_HAVE_UINT8_T 1
  #endif

  #if !defined(OSMSCOUT_HAVE_INT16_T)
    typedef          __int16 int16_t;
    #define OSMSCOUT_HAVE_INT16_T 1
  #endif

  #if !defined(OSMSCOUT_HAVE_UINT16_T)
    typedef unsigned __int16 uint16_t;
    #define OSMSCOUT_HAVE_UINT16_T 1
  #endif

  #if !defined(OSMSCOUT_HAVE_INT32_T)
    typedef          __int32 int32_t;
    #define OSMSCOUT_HAVE_INT32_T 1
  #endif

  #if !defined(OSMSCOUT_HAVE_UINT32_T)
    typedef unsigned __int32 uint32_t;
    #define OSMSCOUT_HAVE_UINT32_T 1
  #endif

  #if !defined(OSMSCOUT_HAVE_INT64_T)
    typedef          __int64 int64_t;
    #define OSMSCOUT_HAVE_INT64_T 1
  #endif

  #if !defined(OSMSCOUT_HAVE_UINT64_T)
    typedef unsigned __int64 uint64_t;
    #define OSMSCOUT_HAVE_UINT64_T 1
  #endif
#endif
#endif
