#ifndef OSMSCOUT_SYSTEM_MATH_H
#define OSMSCOUT_SYSTEM_MATH_H

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


#if defined(_WIN32)
  #define _USE_MATH_DEFINES
#endif

#include <cmath>

#if !defined(M_PI)
  #define M_PI 3.14159265358979323846
#endif

#if !defined(M_E)
    #define M_E 2.7182818284590452354
#endif

#endif
