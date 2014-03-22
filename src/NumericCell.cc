/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2013  Dr. Jürgen Sauermann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <errno.h>
#include <fenv.h>
#include <math.h>

#include "Workspace.hh"
#include "Value.hh"
#include "Error.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IntCell.hh"

//-----------------------------------------------------------------------------
void
NumericCell::bif_not(Cell * Z) const
{
   if (get_near_bool(Workspace::get_CT()))   new (Z) IntCell(0);
   else                                      new (Z) IntCell(1);
}
//-----------------------------------------------------------------------------
void
NumericCell::do_binomial(Cell * Z, APL_Integer a, APL_Integer b, bool negate)
{
double nom_f = 1.0;
APL_Integer nom_i = 1;
APL_Integer den_max = a;
APL_Integer den_min = b - a;

   Assert(b >= a);
   Assert(a >= 0);

   if (den_max < den_min)
      {
        den_max = b - a;
        den_min = a;
      }

   // compute b! / den_max! == den_max+1 * den_max+2 * ... * b
   //
   for (APL_Integer n = b; n > den_max; --n)
       {
         nom_i *= n;
         nom_f *= n;
       }

   if (nom_f > BIG_INT64_F)   // integer overflow
      {
        for (APL_Integer n = den_min; n > 1; --n)
            nom_f /= n;

        if (negate)   new(Z)   FloatCell(-nom_f);
        else          new(Z)   FloatCell( nom_f);
      }
   else
      {
        for (APL_Integer n = den_min; n > 1; --n)
            nom_i /= n;

        if (negate)   new(Z)   IntCell(-nom_i);
        else          new(Z)   IntCell( nom_i);
      }
}
//-----------------------------------------------------------------------------
void
NumericCell::bif_binomial(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();
   if (!is_near_real(qct) || !A->is_near_real(qct))   // complex result
      {
        const APL_Float r_1_a    = A->get_real_value();
        const APL_Float r_1_b    = get_real_value();
        const APL_Float r_1_b__a = r_1_b - A->get_real_value();

        const APL_Float i_a   = A->get_imag_value();
        const APL_Float i_b   = get_imag_value();
        const APL_Float i_b_a = i_b - i_a;

        const APL_Complex gam_1_a    = ComplexCell::gamma(r_1_a,    i_a);
        const APL_Complex gam_1_b    = ComplexCell::gamma(r_1_b,    i_b);
        const APL_Complex gam_1_b__a = ComplexCell::gamma(r_1_b__a, i_b_a);

        new (Z) ComplexCell(gam_1_b / (gam_1_a * gam_1_b__a));
        return;
      }

   if (!is_near_int(qct) || !A->is_near_int(qct))   // non-integer result
      {
        const APL_Float r_1_a    = 1.0 + A->get_real_value();
        const APL_Float r_1_b    = 1.0 + get_real_value();
        const APL_Float r_1_b__a = r_1_b - A->get_real_value();

        if (r_1_a < 0    && is_near_int(r_1_a, qct))      DOMAIN_ERROR;
        if (r_1_b < 0    && is_near_int(r_1_b, qct))      DOMAIN_ERROR;
        if (r_1_b__a < 0 && is_near_int(r_1_b__a, qct))   DOMAIN_ERROR;

        new (Z) FloatCell(  tgamma(r_1_b) / (tgamma(r_1_a) * tgamma(r_1_b__a)));
        return;
      }

const APL_Integer a = A->get_near_int(qct);
const APL_Integer b = get_near_int(qct);

int how = 0;
   if (a < 0)    how |= 4;
   if (b < 0)    how |= 2;
   if (b < a)    how |= 1;

   switch(how)
      {
        case 0: do_binomial(Z, a, b, false);                       return;
        case 1: new (Z) IntCell(0);                                return;
        case 2: Assert(0 && "Impossible case 2");                  return;
        case 3: do_binomial(Z, a, a - (b + 1), a & 1);               return;
        case 4: new (Z) IntCell(0);                                return;
        case 5: Assert(0 && "Impossible case 5");                  return;
        case 6: do_binomial(Z, -(b + 1), -(a + 1), (b - a) & 1);   return;
        case 7: new (Z) IntCell(0);                                return;
      }
}
//-----------------------------------------------------------------------------
void NumericCell::bif_and(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   // if either value is 0 then return 0
   //
   if (A->is_near_zero(qct))
      {
        new (Z) IntCell(0);
        return;
      }

   if (is_near_zero(qct))
      {
        new (Z) IntCell(0);
        return;
      }

   if (!A->is_near_real(qct) || !is_near_real(qct))   // complex
      {
        // a or b is complex; we assume (require) Gaussian integers.
        //
        const APL_Complex gcd = cpx_gcd(A->get_complex_value(),
                                           get_complex_value(), qct);

        new (Z) ComplexCell(A->get_complex_value() * (get_complex_value()/gcd));
        return;
      }

   // if both args are boolean then return the classical A ∧ B
   //
   if (A->is_near_bool(qct) && is_near_bool(qct))
      {
        new (Z) IntCell(1);
        return;
      }

   // if both args are int then return the least common multiple of them
   //
   if (A->is_near_int(qct) && is_near_int(qct))
      {
        const APL_Integer a = A->get_near_int(qct);
        const APL_Integer b =    get_near_int(qct);
        const APL_Integer gcd = int_gcd(a, b);
        new (Z) IntCell(a * (b / gcd));
        return;
      }

   // if both args are real then return the (real) least common multiple of them
   //
   if (A->is_near_real(qct) && is_near_real(qct))
      {
        const APL_Float a = A->get_real_value();
        const APL_Float b =    get_real_value();
        const APL_Float gcd = flt_gcd(a, b, qct);
        new (Z) FloatCell(a * (b / gcd));
        return;
      }

   DOMAIN_ERROR;   // char ?
}
//-----------------------------------------------------------------------------
void NumericCell::bif_nand(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();
   if ( A->get_near_bool(qct) && get_near_bool(qct))   new (Z) IntCell(0);
   else                                                new (Z) IntCell(1);
}
//-----------------------------------------------------------------------------
void
NumericCell::bif_nor(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();
   if ( A->get_near_bool(qct) || get_near_bool(qct))   new (Z) IntCell(0);
   else                                                new (Z) IntCell(1);
}
//-----------------------------------------------------------------------------
void
NumericCell::bif_or(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   // if both args are boolean then return the classical A ∨ B
   //
   if (A->is_near_bool(qct) && is_near_bool(qct))
      {
        if ( A->get_near_bool(qct) || get_near_bool(qct))   new (Z) IntCell(1);
        else                                                new (Z) IntCell(0);
      }

   if (!A->is_near_real(qct) || !is_near_real(qct))   // complex
      {
        if (A->is_near_zero(qct))
           {
             new (Z) ComplexCell(get_complex_value());
             return;
           }

        if (is_near_zero(qct))
           {
             new (Z) ComplexCell(A->get_complex_value());
             return;
           }

        // a or b is complex; we assume (require) Gaussian integers.
        //
        const APL_Complex gcd = cpx_gcd(A->get_complex_value(),
                                           get_complex_value(), qct);

        new (Z) ComplexCell(gcd);
        return;
      }

   // if both args are int then return the greatest common divisor of them
   //
   if (A->is_near_int(qct) && is_near_int(qct))
      {
        const APL_Integer a = A->get_near_int(qct);
        const APL_Integer b =    get_near_int(qct);
        const APL_Integer gcd = int_gcd(a, b);
        new (Z) IntCell(gcd);
        return;
      }

   // if both args are real then return the (real) greatest common divisor
   //
   if (A->is_near_real(qct) && is_near_real(qct))
      {
        const APL_Float a = A->get_real_value();
        const APL_Float b =    get_real_value();
        const APL_Float gcd = flt_gcd(a, b, qct);
        new (Z) FloatCell(gcd);
        return;
      }

   DOMAIN_ERROR;   // char ?
}
//-----------------------------------------------------------------------------
APL_Integer
NumericCell::int_gcd(APL_Integer a, APL_Integer b)
{
   if (a == 0x8000000000000000ULL)   DOMAIN_ERROR;
   if (b == 0x8000000000000000ULL)   DOMAIN_ERROR;

   if (a < 0)   a = - a;
   if (b < 0)   b = - b;
   if (b < a)
      {
         const APL_Integer _b = b;
         b = a;
         a = _b;
      }

   // at this point 0 ≤ a ≤ b
   //
   for (;;)
       {
         if (a == 0)   return b;
         const APL_Integer r = b%a;
         b = a;
         a = r;
       }
}
//-----------------------------------------------------------------------------
APL_Float
NumericCell::flt_gcd(APL_Float a, APL_Float b, APL_Float qct)
{
   if (a < 0)   a = - a;
   if (b < 0)   b = - b;
   if (b < a)
      {
         const APL_Float _b = b;
         b = a;
         a = _b;
      }

   // at this point 0 ≤ a ≤ b
   //
   for (;;)
       {
         if (is_near_zero(a, qct))   return b;
         const APL_Float r = fmod(b, a);
         b = a;
         a = r;
       }
}
//-----------------------------------------------------------------------------
APL_Complex
NumericCell::cpx_gcd(APL_Complex a, APL_Complex b, APL_Float qct)
{
   if (!is_near_int(a.real(), qct))   DOMAIN_ERROR;
   if (!is_near_int(a.imag(), qct))   DOMAIN_ERROR;
   if (!is_near_int(b.real(), qct))   DOMAIN_ERROR;
   if (!is_near_int(b.imag(), qct))   DOMAIN_ERROR;

   // make a and b true integers
   //
   a = APL_Complex(round(a.real()), a.imag());
   b = APL_Complex(round(b.real()), b.imag());

   for (;;)
       {
         if (abs(a) > abs(b))   // make ∣b∣ > ∣a∣
            {
               const APL_Complex _b = b;
               b = a;
               a = _b;
            }

         if (abs(a) < 0.2)   return b;

         const APL_Complex xy = b/a;
         const APL_Complex q(round(xy.real()), round(xy.imag()));
         const APL_Complex r(b - q*a);

         b = a;
         a = r;
       }
}
//-----------------------------------------------------------------------------
APL_Complex
NumericCell::cpx_max_real(APL_Complex a)
{
const double r_abs = a.real() < 0 ? -a.real() : a.real();
const double i_abs = a.imag() < 0 ? -a.imag() : a.imag();
APL_Complex z;

   if (r_abs < i_abs)   // multiply by i in order to exchange real and imag
      {
        z = APL_Complex(- a.imag(), a.real());
     }
   else
      {
        z = a;
     }

   if (z.real() < 0)   // multiply by -1 in order to make z.real positive
      {
        z = APL_Complex(- z.real(), - z.imag());
      }

   return z;
}
//-----------------------------------------------------------------------------
