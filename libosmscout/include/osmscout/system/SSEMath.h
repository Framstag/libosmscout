#ifndef OSMSCOUT_SYSTEM_SSEMATH_H
#define OSMSCOUT_SYSTEM_SSEMATH_H

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

#include <osmscout/system/Math.h>

#include <osmscout/system/SSEMathPublic.h>

#include <osmscout/system/SystemTypes.h>

#include <osmscout/private/Config.h>

namespace osmscout {

#define ARRAY2V2DF(name) * reinterpret_cast<const v2df*>(name)
#define ARRAY2V2DI(name) * reinterpret_cast<const v2di*>(name)

#define DECLARE_COEFFS(name) extern const  ALIGN16_BEG double name[]  ALIGN16_END;

#define POLY_EVAL3SINGLE_HORNER(y, x, coeff) \
  y = coeff[0]; y = y*x + coeff[2]; y = y*x + coeff[4]; y = y*x + coeff[6]

#define POLY_EVAL3_HORNER(y, x, coeff) \
  y =  ARRAY2V2DF(&coeff[0]); \
  y = _mm_add_pd(_mm_mul_pd(y, x), ARRAY2V2DF(&coeff[2])); \
  y = _mm_add_pd(_mm_mul_pd(y, x), ARRAY2V2DF(&coeff[4])); \
  y = _mm_add_pd(_mm_mul_pd(y, x), ARRAY2V2DF(&coeff[6]))

#define POLY_EVAL6SINGLE_HORNER(y, x, coeff) \
  y = coeff[0]; y = y*x + coeff[2]; y = y*x + coeff[4]; y = y*x + coeff[6]; \
  y = y*x + coeff[8];  y = y*x + coeff[10]; y = y*x + coeff[12]

#define POLY_EVAL6_HORNER(y, x, coeff) \
  y =  ARRAY2V2DF(&coeff[0]); \
  y = _mm_add_pd(_mm_mul_pd(y, x), ARRAY2V2DF(&coeff[2])); \
  y = _mm_add_pd(_mm_mul_pd(y, x), ARRAY2V2DF(&coeff[4])); \
  y = _mm_add_pd(_mm_mul_pd(y, x), ARRAY2V2DF(&coeff[6])); \
  y = _mm_add_pd(_mm_mul_pd(y, x), ARRAY2V2DF(&coeff[8])); \
  y = _mm_add_pd(_mm_mul_pd(y, x), ARRAY2V2DF(&coeff[10])); \
  y = _mm_add_pd(_mm_mul_pd(y, x), ARRAY2V2DF(&coeff[12]))

#define POLY_EVAL6SINGLE_ESTRIN(y, x, coeff) \
  y = coeff[12]; y = y*x + coeff[10]; y = y*x + coeff[8]; y = y*x + coeff[6]; \
  y = y*x + coeff[4];  y = y*x + coeff[2]; y = y*x + coeff[0]; \

#define POLY_EVAL6_ESTRIN(y, x, coeff) \
  v2df pt0 =  _mm_add_pd(ARRAY2V2DF(&coeff[0]), _mm_mul_pd(ARRAY2V2DF(&coeff[ 2]), x)); \
  v2df pt1 =  _mm_add_pd(ARRAY2V2DF(&coeff[4]), _mm_mul_pd(ARRAY2V2DF(&coeff[ 6]), x)); \
  v2df pt2 =  _mm_add_pd(ARRAY2V2DF(&coeff[8]), _mm_mul_pd(ARRAY2V2DF(&coeff[10]), x)); \
  v2df pt3 =  ARRAY2V2DF(&coeff[12]); \
  v2df ptx2 = _mm_mul_pd(x,x); \
  pt0 = _mm_add_pd(pt0, _mm_mul_pd(pt1, ptx2)); \
  pt2 = _mm_add_pd(pt2, _mm_mul_pd(pt3, ptx2)); \
  v2df ptx4 = _mm_mul_pd(ptx2,ptx2); \
  y = _mm_add_pd(pt0, _mm_mul_pd(pt2, ptx4))

#ifdef _SSE_SLOW_ //For some cpu's this is faster

//we assume the cpu can not be sped up by doing several calculations in parralel.
//using Horner's method http://en.wikipedia.org/wiki/Horner_scheme

#define POLY_EVAL3SINGLE(y, x, coeff)  POLY_EVAL3SINGLE_HORNER(y, x, coeff)
#define POLY_EVAL3(y, x, coeff) POLY_EVAL3_HORNER(y, x, coeff)
#define POLY_EVAL6SINGLE(y, x, coeff) POLY_EVAL6SINGLE_HORNER(y, x, coeff)
#define POLY_EVAL6(y, x, coeff) POLY_EVAL6_HORNER(y, x, coeff)

#else
//we assume the cpu can be sped up by doing calculations in parallel (several xmm registers)
//using Estrin's scheme http://en.wikipedia.org/wiki/Estrin%27s_scheme

#define POLY_EVAL3SINGLE(y, x, coeff)  POLY_EVAL3SINGLE_HORNER(y, x, coeff)
#define POLY_EVAL3(y, x, coeff) POLY_EVAL3_HORNER(y, x, coeff)
#define POLY_EVAL6SINGLE(y, x, coeff) POLY_EVAL6SINGLE_ESTRIN(y, x, coeff)
#define POLY_EVAL6(y, x, coeff) POLY_EVAL6_ESTRIN(y, x, coeff)

#endif

DECLARE_COEFFS(SINECOEFF_SSE)
DECLARE_COEFFS(COSINECOEFF_SSE)
DECLARE_COEFFS(LOGCOEFF)

/* declare some SSE constants */
#define _PS_CONST(Name)                                            \
  extern const  ALIGN16_BEG double _pd_##Name[2]  ALIGN16_END
#define _PS_CONST_TYPE(Name, Type)                                 \
  extern const  ALIGN16_BEG Type _pd_##Name[2]  ALIGN16_END

_PS_CONST(1);
_PS_CONST(2);
_PS_CONST(0_5);
_PS_CONST(2OPI);
_PS_CONST(PIO2);
_PS_CONST(LOG_C_2);

_PS_CONST_TYPE(sign_mask, uint64_t);
_PS_CONST_TYPE(x01_double_mask, uint64_t);
_PS_CONST_TYPE(x03FE_double_mask, uint64_t);
_PS_CONST_TYPE(1_exp, uint64_t);
_PS_CONST_TYPE(f_fraction_mask, uint64_t);
_PS_CONST_TYPE(f_exp_mask, uint64_t);
_PS_CONST_TYPE(f_one_mask, uint64_t);
_PS_CONST_TYPE(1022, uint64_t);
_PS_CONST(0_66);
_PS_CONST(0_87);
_PS_CONST(1_74);
_PS_CONST(1_32);
_PS_CONST(log_inv_1_74);
_PS_CONST(log_inv_1_32);

//approximate a sin using a 6th order polynomal function.
//Use the mirroring properties of the sine to calculate
//result from only [0..Pi/2] interval. This one can take an __m128d
inline v2df sin_pd(v2df x)
{
  //bool sign = x < 0.0;
  v2df sign_mask = ARRAY2V2DF(_pd_sign_mask);
  v2df sign = _mm_and_pd(x, sign_mask);

  //x = fabs(x);
  x = _mm_andnot_pd(sign_mask, x);

  //double modulo = x * 2.0/PI;
  v2df modulo = _mm_mul_pd(x, ARRAY2V2DF(_pd_2OPI));

  //int i =  static_cast<int>(floor(modulo));
  //double frac = modulo-i;
  v2di i = _mm_cvttpd_epi32 (modulo);
  v2df frac = _mm_sub_pd(modulo, _mm_cvtepi32_pd(i));
  i = _mm_shuffle_epi32 (i, _MM_SHUFFLE(1, 1, 0, 0)); //resuffle to align

  //if (i&2) sign = !sign;
  v2di gt3 = _mm_slli_epi32(i, 30);
  v2di mask = _mm_and_si128( gt3, _mm_castpd_si128(sign_mask));
  sign = _mm_xor_pd(sign, _mm_castsi128_pd(mask));

  //if (i & 1) frac = 1 - frac;
  v2di signmask2 = _mm_slli_epi64(i, 63); //shift lsb to msb. Is sign position for doubles
  v2di add =_mm_srli_epi32( _mm_srai_epi32(signmask2,9),2); //makes 1.0 if msb was set
  frac = _mm_add_pd(_mm_or_pd(frac, _mm_castsi128_pd(signmask2)), _mm_castsi128_pd(add)); //(where msb is set negate frac) then add 1 if msb was set.

  //double calcx = frac * PI/2;
  v2df calcx = _mm_mul_pd(frac, ARRAY2V2DF(_pd_PIO2));

  //double xx = calcx * calcx;
  v2df xx = _mm_mul_pd(calcx, calcx);

  //double y;
  v2df y;

  //y = SINECOEFF_SSE[0]; etc
  POLY_EVAL6(y, xx, SINECOEFF_SSE);
  //  y = calcx * y*xx+ calcx;
  y = _mm_mul_pd(y,xx);
  y = _mm_add_pd(_mm_mul_pd(y, calcx), calcx);

  //if (sign) y = -y;
  y = _mm_or_pd(sign, y);

  return y;
}

//an sse variant that does not check the input range.
//THIS METHOD IS ONLY VALID ON [-Pi/2,Pi/2]
//but it is 3 times faster as the one above, and more accurate.
inline v2df dangerous_sin_pd(v2df x){
  //double xx = x * x;
  v2df xx = _mm_mul_pd(x, x);

  //double y;
  v2df y;

  //y = SINECOEFF_SSE[0]; etc
  POLY_EVAL6(y, xx, SINECOEFF_SSE);
  //  y = calcx * y*xx+ calcx;
  y = _mm_mul_pd(y,xx);
  y = _mm_add_pd(_mm_mul_pd(y, x), x);

  return y;
}

//some helper functions to test
inline double sin_pd(double x){
  v2df r = sin_pd(_mm_set1_pd(x));
  _mm_storel_pd (&x, r);
  return x;
}

//THIS METHOD IS ONLY VALID ON [-Pi/2,Pi/2]
inline double dangerous_sin_pd(double x){
  v2df r = dangerous_sin_pd(_mm_set1_pd(x));
  _mm_storel_pd (&x, r);
  return x;
}

inline void sin_pd(double x, double y, double& res_x, double& res_y){
  v2df r = sin_pd(_mm_setr_pd(y, x));
  _mm_storel_pd (&res_x, r);
  _mm_storeh_pd (&res_y, r);
}

//THIS METHOD IS ONLY VALID ON [-Pi/2,Pi/2]
inline void dangerous_sin_pd(double x, double y, double& res_x, double& res_y){
  v2df r = dangerous_sin_pd(_mm_setr_pd(y, x));
  _mm_storel_pd (&res_x, r);
  _mm_storeh_pd (&res_y, r);
}

//calculate a sine and cosine
inline void sin_cos_pd(double x, double& res_sin, double& res_cos){
  v2df r = sin_pd(_mm_setr_pd( x + M_PI/2.0, x));
  _mm_storeh_pd (&res_sin, r);
  _mm_storel_pd (&res_cos, r);
}


//approximate cosine using a 6th order polynomal function.
//Use the mirroring properties of the cosine to calculate
//result from only [0..Pi/2] interval. This one can take an __m128d
inline v2df cos_pd(v2df x){
  //x = fabs(x);
  v2df sign_mask = ARRAY2V2DF(_pd_sign_mask);
  x = _mm_andnot_pd(sign_mask, x);

  //double modulo = x * 2.0/PI;
  v2df modulo = _mm_mul_pd(x, ARRAY2V2DF(_pd_2OPI));

  //int i =  static_cast<int>(floor(modulo));
  //double frac = modulo-i;
  v2di i = _mm_cvttpd_epi32 (modulo);
  v2df frac = _mm_sub_pd(modulo, _mm_cvtepi32_pd(i));
  i = _mm_shuffle_epi32 (i, _MM_SHUFFLE(1, 1, 0, 0)); //resuffle to align

  //if (i & 1) frac = 1 - frac;
  v2di signmask2 = _mm_slli_epi64(i, 63); //shift lsb to msb. Is sign position for doubles
  v2di add =_mm_srli_epi32( _mm_srai_epi32(signmask2,9),2); //makes 1.0 if msb was set
  frac = _mm_add_pd(_mm_or_pd(frac, _mm_castsi128_pd(signmask2)), _mm_castsi128_pd(add)); //(where msb is set negate frac) then add 1 if msb was set.

  //sign = ((i+1)&2)==2;
  v2di gt3 = _mm_slli_epi32(_mm_add_epi32(i, ARRAY2V2DI(_pd_x01_double_mask)), 30);
  v2df sign = _mm_and_pd( _mm_castsi128_pd(gt3), sign_mask);

  //double calcx = frac * PI/2;
  v2df calcx = _mm_mul_pd(frac, ARRAY2V2DF(_pd_PIO2));

  //double xx = calcx * calcx;
  v2df xx = _mm_mul_pd(calcx, calcx);

  //double y;
  v2df y;

  //y = COSINECOEFF[0]; etc
  POLY_EVAL6(y, xx, COSINECOEFF_SSE);
  //  y = y*xx + 1;
  y = _mm_add_pd(_mm_mul_pd(y,xx), ARRAY2V2DF(_pd_1));

  y = _mm_or_pd(sign, y);

  return y;
}

//some helper functions
inline double cos_pd(double x){
  v2df r = cos_pd(_mm_set1_pd(x));
  _mm_storel_pd (&x, r);
  return x;
}

inline double cos_pd(double x, double y, double& res_x, double& res_y){
  v2df r = cos_pd(_mm_setr_pd(y, x));
  _mm_storel_pd (&res_x, r);
  _mm_storeh_pd (&res_y, r);
  return x;
}

//calculate log in sse form.
//This is done by doing math :)

//A double is stored as: 2^exp * mantise

//using:
// ln(x) = log2(x)/ln(2);
// ln(a*b) = ln(a)+ln(b)
// log2(2^exp) = exp
//
// we can simplefy to ln(2^exp * mantise) = ln(2^exp) * ln(mantise) = log2(2^exp)/ln(2) *ln(mantise) = exp * ln(2) * ln(mantise)
// mantise is in range [0.5, 1.0[

//by scaling this range so that is is around 1, we can use this method: http://en.wikipedia.org/wiki/Natural_logarithm#Numerical_value
//that is fast converging around 1.

inline v2df log_pd(v2df x)
{
  //x = frexp( x, &e );
  v2di e_int = _mm_and_si128(_mm_castpd_si128(x), ARRAY2V2DI(_pd_f_exp_mask));
  e_int = _mm_srli_epi64(e_int,52);
  e_int = _mm_or_si128(_mm_srli_si128(e_int,4), e_int);
  e_int = _mm_sub_epi32(e_int, ARRAY2V2DI(_pd_x03FE_double_mask));
  v2df e = _mm_cvtepi32_pd( e_int);
  x = _mm_or_pd(_mm_and_pd(x, ARRAY2V2DF(_pd_f_fraction_mask)), ARRAY2V2DF(_pd_f_one_mask));

  //double ex = 0.0;
  //v = x;

  //if (x < 0.66) {
  //    ex = l1;
  //    v = x*1.74;
  //}
  v2df mask =_mm_cmplt_pd(x, ARRAY2V2DF(_pd_0_87));
  v2df ex = _mm_and_pd(mask, ARRAY2V2DF(_pd_log_inv_1_32));
  v2df mulx = _mm_mul_pd(x,ARRAY2V2DF(_pd_1_32));
  v2df v =_mm_or_pd( _mm_and_pd(mask, mulx), _mm_andnot_pd(mask, x));

  //else if (x < 0.87) {
  //  ex = l2;
  //  v = x* 1.32;
  //}
  mask =_mm_cmplt_pd(x, ARRAY2V2DF(_pd_0_66));
  ex = _mm_or_pd(_mm_and_pd(mask, ARRAY2V2DF(_pd_log_inv_1_74)), _mm_andnot_pd(mask, ex));
  mulx = _mm_mul_pd(x,ARRAY2V2DF(_pd_1_74));
  v =_mm_or_pd( _mm_and_pd(mask, mulx), _mm_andnot_pd(mask, v));

  //double term = (v-1)/(v+1);
  v2df ones = ARRAY2V2DF(_pd_1);
  v2df term = _mm_div_pd(_mm_sub_pd(v, ones), _mm_add_pd(v,ones));

  //double termsquared = term*term;
  v2df termsquared = _mm_mul_pd(term, term);

  //double z = term;

  //double res = 1.0/9.0;
  v2df res;
  POLY_EVAL3(res, termsquared , LOGCOEFF);
  //res *= z * termsquared;
  res = _mm_mul_pd(_mm_mul_pd(res,term), termsquared);
  //res += z;
  res = _mm_add_pd(res, term);

  //return (e * _pd_LOG_C_2[0])+ex+2* res;
  v2df r1 = _mm_mul_pd(e, ARRAY2V2DF(_pd_LOG_C_2));
  v2df r2 = _mm_mul_pd( _mm_add_pd(ones,ones), res);
  return _mm_add_pd(r1, _mm_add_pd(r2, ex));
}


//help functions
inline double log_pd(double x){
  v2df r = log_pd(_mm_set1_pd(x));
  _mm_storel_pd (&x, r);
  return x;
}

inline double log_pd(double x, double y, double& res_x, double& res_y){
  v2df r = log_pd(_mm_setr_pd(y, x));
  _mm_storel_pd (&res_x, r);
  _mm_storeh_pd (&res_y, r);
  return x;
}

//calculate atanh
inline v2df atanh_pd(v2df x){
  v2df ones = ARRAY2V2DF(_pd_1);
  v2df param = _mm_div_pd( _mm_add_pd(ones,x), _mm_sub_pd(ones,x));
  v2df logres=log_pd(param);
  return _mm_mul_pd(ARRAY2V2DF(_pd_0_5), logres);
}

inline double atanh_pd(double x){
  v2df r = atanh_pd(_mm_set1_pd(x));
  _mm_storel_pd (&x, r);
  return x;
}

inline double atanh_pd(double x, double y, double& res_x, double& res_y){
  v2df r = atanh_pd(_mm_setr_pd(y, x));
  _mm_storel_pd (&res_x, r);
  _mm_storeh_pd (&res_y, r);
  return x;
}

//calculate atan(sin(x))
//note that this is only good between 0.944*-Pi/2 < x <  0.944* pi/2 which is up to 85 degrees
inline v2df atanh_sin_pd(v2df x){
  return atanh_pd(dangerous_sin_pd(x));
}

inline double atanh_sin_pd(double x){
  v2df r = atanh_sin_pd(_mm_set1_pd(x));
  _mm_storel_pd (&x, r);
  return x;
}

//calculate atan(sin(x)) and atan(sin(y))
inline double atanh_sin_pd(double x, double y, double& res_x, double& res_y){
  v2df r = atanh_sin_pd(_mm_setr_pd(y, x));
  _mm_storel_pd (&res_x, r);
  _mm_storeh_pd (&res_y, r);
  return x;
}

}
#endif
