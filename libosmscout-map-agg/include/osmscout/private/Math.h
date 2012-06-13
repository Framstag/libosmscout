#ifndef OSMSCOUT_MAP_AGG_UTIL_MATH_H
#define OSMSCOUT_MAP_AGG_UTIL_MATH_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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


#if defined(__WIN32__) || defined(WIN32)
  #define _USE_MATH_DEFINES
#endif

#include <cmath>

#include <osmscout/private/Config.h>

#if !defined(M_PI)
  #define M_PI 3.14159265358979323846
#endif

#if !defined(HAVE_DECL_LOG2)
  inline double log2(double x)
  {
    return log(x)/log(2.0l);
  }
#endif

#if !defined(HAVE_DECL_ATANH)
  inline double atanh(double x)
    {
      return log((1.0+x)/(1.0-x))/2.0;
    }
#endif

#if !defined(HAVE_DECL_LROUND)
  inline long lround(double d)
  {
    return (long)(d>0 ? d+0.5 : ceil(d-0.5));
  }
#endif

#endif
