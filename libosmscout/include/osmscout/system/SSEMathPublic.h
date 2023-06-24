#ifndef OSMSCOUT_SYSTEM_SSEMATH_PUBLIC_H
#define OSMSCOUT_SYSTEM_SSEMATH_PUBLIC_H

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

#include <osmscout/lib/CoreFeatures.h>

#if defined(OSMSCOUT_HAVE_SSE2)

//include sse and sse2 headers
#include <xmmintrin.h>
#include <emmintrin.h>

//create alignment types
#ifdef _MSC_VER /* visual c++ */
# define ALIGN16_BEG __declspec(align(16))
# define ALIGN16_END
#else /* gcc or icc */
# define ALIGN16_BEG
# define ALIGN16_END __attribute__((aligned(16)))
#endif

#else
#include <osmscout/system/Math.h>
#endif

namespace osmscout {

#if defined(OSMSCOUT_HAVE_SSE2)
  #define sincos sin_cos_pd

  /* __m128 is ugly to write */
  typedef __m128d v2df;  // vector of 2 double (sse2)
  typedef __m128i v2di;  // vector of 2 int (sse2)
#else
  inline void sincos(double x, double& resSin, double& resCos)
  {
    resSin = sin(x);
    resCos = cos(x);
  }
#endif
}

#endif
