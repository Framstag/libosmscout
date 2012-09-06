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
#include <osmscout/private/Config.h>

#include "osmscout/CoreFeatures.h"

#ifdef OSMSCOUT_HAVE_SSE2
#include <osmscout/system/SSEMathPublic.h>
#include <osmscout/system/SSEMath.h>

/* This file is the place to store the constant arrays the math functions need */
namespace osmscout {

#define DECLARE_COEFFS3_HORNER(name, v0, v1, v2, v3) \
  const ALIGN16_BEG double name[]   ALIGN16_END =  \
    { v3, v3, v2, v2 ,v1, v1, v0, v0 }

#define DECLARE_COEFFS6_HORNER(name, v0, v1, v2, v3, v4, v5, v6) \
  const ALIGN16_BEG double name[]   ALIGN16_END =  \
    { v6, v6, v5, v5, v4, v4, v3, v3, v2, v2 ,v1, v1, v0, v0 }

#define DECLARE_COEFFS6_ESTRIN(name, v0, v1, v2, v3, v4, v5, v6) \
  const ALIGN16_BEG double name []   ALIGN16_END =  \
  { v0, v0, v1, v1 , v2, v2, v3, v3, v4, v4, v5, v5, v6, v6}

#ifdef _SSE_SLOW_
//we assume the cpu can not be sped up by doing several calculations in parralel.
//using Horner's method http://en.wikipedia.org/wiki/Horner_scheme

#define DECLARE_COEFFS3(name, v0, v1, v2, v3) DECLARE_COEFFS3_HORNER(name, v0, v1, v2, v3)
#define DECLARE_COEFFS6(name, v0, v1, v2, v3, v4, v5, v6) DECLARE_COEFFS6_HORNER(name, v0, v1, v2, v3, v4, v5, v6)

#else
//we assume the cpu can be sped up by doing calculations in parallel (several xmm registers)
//using Estrin's scheme http://en.wikipedia.org/wiki/Estrin%27s_scheme

#define DECLARE_COEFFS3(name, v0, v1, v2, v3) DECLARE_COEFFS3_HORNER(name, v0, v1, v2, v3)
#define DECLARE_COEFFS6(name, v0, v1, v2, v3, v4, v5, v6) DECLARE_COEFFS6_ESTRIN(name, v0, v1, v2, v3, v4, v5, v6)
#endif

//calculated using http://lol.zoy.org/blog/2011/12/21/better-function-approximations
DECLARE_COEFFS6(SINECOEFF_SSE,
  -1.666666666666581208932767360735836413787e-1,
   8.333333333262878969283334152712679345090e-3,
  -1.984126982009420841621862535256836970687e-4,
   2.755731607700772351872307094572902723297e-6,
  -2.505185149701259571358956642584298321640e-8,
   1.604730119668575379135607736724374349864e-10,
  -7.364646450221048096686073152326538711869e-13
);

DECLARE_COEFFS6(COSINECOEFF_SSE,
-4.999999999999968547549836553973161411245e-1,
 4.166666666654159956931695464194325344832e-2,
-1.388888888076887080081006349082478789485e-3,
 2.480158532413074128666408890381127238035e-5,
-2.755708978788414424474178112107145153072e-7,
 2.086307659146596022522689577732779722810e-9,
-1.106510595684224429210981386434495380246e-11
);

DECLARE_COEFFS3(LOGCOEFF , 1/3.0, 1/5.0, 1/7.0, 1/9.0 );

/* declare some SSE constants */
#undef _PS_CONST
#undef _PS_CONST_TYPE

#define _PS_CONST(Name, Val)                                            \
  const ALIGN16_BEG double _pd_##Name[2] ALIGN16_END = { Val, Val }
#define _PS_CONST_TYPE(Name, Type, Val)                                 \
  const ALIGN16_BEG Type _pd_##Name[2] ALIGN16_END = { Val, Val }

_PS_CONST(1  , 1.0);
_PS_CONST(2  , 2.0);
_PS_CONST(0_5  , 0.5);
_PS_CONST(2OPI , 2.0/M_PI);
_PS_CONST(PIO2 , M_PI/2.0);
_PS_CONST(LOG_C_2, log(2.0));

_PS_CONST_TYPE(sign_mask, uint64_t, 0x8000000000000000);
_PS_CONST_TYPE(x01_double_mask, uint64_t, 0x0000000100000001);
_PS_CONST_TYPE(x03FE_double_mask, uint64_t, 0x000003FE000003FE);
_PS_CONST_TYPE(1_exp, uint64_t, 1ll<<52);
_PS_CONST_TYPE(f_fraction_mask, uint64_t, 0xFFFFFFFFFFFFF);
_PS_CONST_TYPE(f_exp_mask, uint64_t, 0x7FF0000000000000);
_PS_CONST_TYPE(f_one_mask, uint64_t, 0x3FE0000000000000);
_PS_CONST_TYPE(1022, uint64_t, 1022);
_PS_CONST(0_66  , 0.66);
_PS_CONST(0_87  , 0.87);
_PS_CONST(1_74  , 1.74);
_PS_CONST(1_32  , 1.32);
_PS_CONST(log_inv_1_74  , log(1/1.74));
_PS_CONST(log_inv_1_32  , log(1/1.32));
}
#endif