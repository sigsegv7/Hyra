/*
 * Copyright (c) 2023-2025 Ian Marco Moffett and the Osmora Team.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Hyra nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MATH_H_
#define _MATH_H_ 1

#define M_E             2.7182818284590452354
#define M_LOG2E         1.4426950408889634074
#define M_LOG10E        0.43429448190325182765
#define M_LN2           0.69314718055994530942
#define M_LN10          2.30258509299404568402
#define M_PI            3.14159265358979323846
#define M_PI_2          1.57079632679489661923
#define M_PI_4          0.78539816339744830962
#define M_1_PI          0.31830988618379067154
#define M_2_PI          0.63661977236758134308
#define M_2_SQRTPI      1.12837916709551257390
#define M_SQRT2         1.41421356237309504880
#define M_SQRT1_2       0.70710678118654752440
#define M_PIl           3.141592653589793238462643383279502884L

#define FP_ILOGBNAN (-1 - (int)(((unsigned)-1) >> 1))
#define FP_ILOGB0 FP_ILOGBNAN
#define FP_INFINITE 1
#define FP_NAN 2
#define FP_NORMAL 4
#define FP_SUBNORMAL 8
#define FP_ZERO 16

#define isfinite(x) (fpclassify(x) & (FP_NORMAL | FP_SUBNORMAL | FP_ZERO))
#define isnan(x) (fpclassify(x) == FP_NAN)
#define isinf(x) (fpclassify(x) == FP_INFINITE)
#define isnormal(x) (fpclassify(x) == FP_NORMAL)
#define signbit(x) (__builtin_signbit(x))

#define INFINITY (__builtin_inff())
#define NAN (__builtin_nanf(""))

int __fpclassify(double __x);
int __fpclassifyf(float __x);
int __fpclassifyl(long double __x);

#define fpclassify(x) \
	(sizeof(x) == sizeof(double) ? __fpclassify(x) : \
	(sizeof(x) == sizeof(float) ? __fpclassifyf(x) : \
	(sizeof(x) == sizeof(long double) ? __fpclassifyl(x) : \
	0)))

typedef double double_t;
typedef float float_t;

double exp10(double __x);
float exp10f(float __x);
long double exp10l(long double __x);

double exp(double __x);
float expf(float __x);
long double expl(long double __x);

double exp2(double __x);
float exp2f(float __x);
long double exp2l(long double __x);

double expm1(double __x);
float expm1f(float __x);
long double expm1l(long double __x);

double frexp(double __x, int *__power);
float frexpf(float __x, int *__power);
long double frexpl(long double __x, int *__power);

int ilogb(double __x);
int ilogbf(float __x);
int ilogbl(long double __x);

double ldexp(double __x, int __power);
float ldexpf(float __x, int __power);
long double ldexpl(long double __x, int __power);

double log(double __x);
float logf(float __x);
long double logl(long double __x);

double log10(double __x);
float log10f(float __x);
long double log10l(long double __x);

double log1p(double __x);
float log1pf(float __x);
long double log1pl(long double __x);

double log2(double __x);
float log2f(float __x);
long double log2l(long double __x);

double logb(double __x);
float logbf(float __x);
long double logbl(long double __x);

double modf(double __x, double *__integral);
float modff(float __x, float *__integral);
long double modfl(long double __x, long double *__integral);

double scalbn(double __x, int __power);
float scalbnf(float __x, int __power);
long double scalbnl(long double __x, int __power);

double scalbln(double __x, long __power);
float scalblnf(float __x, long __power);
long double scalblnl(long double __x, long __power);

double cbrt(double __x);
float cbrtf(float __x);
long double cbrtl(long double __x);

double fabs(double __x);
float fabsf(float __x);
long double fabsl(long double __x);

double hypot(double __x, double __y);
float hypotf(float __x, float __y);
long double hypotl(long double __x, long double __y);

double pow(double __x, double __y);
float powf(float __x, float __y);
long double powl(long double __x, long double __y);

double sqrt(double __x);
float sqrtf(float __x);
long double sqrtl(long double __x);

double erf(double __x);
float erff(float __x);
long double erfl(long double __x);

double erfc(double __x);
float erfcf(float __x);
long double erfcl(long double __x);

double lgamma(double __x);
float lgammaf(float __x);
long double lgammal(long double __x);

double tgamma(double __x);
float tgammaf(float __x);
long double tgammal(long double __x);

double ceil(double __x);
float ceilf(float __x);
long double ceill(long double __x);

double floor(double __x);
float floorf(float __x);
long double floorl(long double __x);

double nearbyint(double __x);
float nearbyintf(float __x);
long double nearbyintl(long double __x);

double rint(double __x);
float rintf(float __x);
long double rintl(long double __x);

long lrint(double __x);
long lrintf(float __x);
long lrintl(long double __x);

long long llrint(double __x);
long long llrintf(float __x);
long long llrintl(long double __x);

double round(double __x);
float roundf(float __x);
long double roundl(long double __x);

long lround(double __x);
long lroundf(float __x);
long lroundl(long double __x);

long long llround(double __x);
long long llroundf(float __x);
long long llroundl(long double __x);

double trunc(double __x);
float truncf(float __x);
long double truncl(long double __x);

double fmod(double __x, double __y);
float fmodf(float __x, float __y);
long double fmodl(long double __x, long double __y);

double remainder(double __x, double __y);
float remainderf(float __x, float __y);
long double remainderl(long double __x, long double __y);

double remquo(double __x, double __y, int *__quotient);
float remquof(float __x, float __y, int *__quotient);
long double remquol(long double __x, long double __y, int *__quotient);

double copysign(double __x, double __sign);
float copysignf(float __x, float __sign);
long double copysignl(long double __x, long double __sign);

double nan(const char *__tag);
float nanf(const char *__tag);
long double nanl(const char *__tag);

double nextafter(double __x, double __dir);
float nextafterf(float __x, float __dir);
long double nextafterl(long double __x, long double __dir);

double nexttoward(double __x, long double __dir);
float nexttowardf(float __x, long double __dir);
long double nexttowardl(long double __x, long double __dir);

double fdim(double __x, double __y);
float fdimf(float __x, float __y);
long double fdiml(long double __x, long double __y);

double fmax(double __x, double __y);
float fmaxf(float __x, float __y);
long double fmaxl(long double __x, long double __y);

double fmin(double __x, double __y);
float fminf(float __x, float __y);
long double fminl(long double __x, long double __y);

double atan(double __x);
float atanf(float __x);
long double atanl(long double __x);

double atan2(double __x, double __y);
float atan2f(float __x, float __y);
long double atan2l(long double __x, long double __y);

double cos(double __x);
float cosf(float __x);
long double cosl(long double __x);

double sin(double __x);
float sinf(float __x);
long double sinl(long double __x);

double tan(double __x);
float tanf(float __x);
long double tanl(long double __x);

#endif  /* !_MATH_H_ */
