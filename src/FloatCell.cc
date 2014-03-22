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

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ComplexCell.hh"
#include "Error.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "UTF8_string.hh"
#include "Value.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
bool
FloatCell::equal(const Cell & A, APL_Float qct) const
{
   if (!A.is_numeric())       return false;
   if (A.is_complex_cell())   return A.equal(*this, qct);
   return equal_within_quad_CT(A.get_real_value(), get_real_value(), qct);
}
//-----------------------------------------------------------------------------
bool
FloatCell::equal_within_quad_CT(APL_Float A, APL_Float B, APL_Float qct)
{
const APL_Float posA = A < 0 ? -A : A;
const APL_Float posB = B < 0 ? -B : B;
const APL_Float maxAB = posA > posB ? posA : posB;

   // if the signs of A and B differ then they are unequal (standard page 100)
   // we treat exact 0.0 as having both signs
   //
   if (A < 0.0 && B > 0.0)   return false;
   if (A > 0.0 && B < 0.0)   return false;

   // for large numbers we allow a larger ⎕CT
   //
   if (maxAB > 1.0)   qct *= maxAB;

   if (posB < (posA - qct))   return false;
   if (posB > (posA + qct))   return false;
   return true;
}
//-----------------------------------------------------------------------------
bool
FloatCell::greater(const Cell * other, bool ascending) const
{
const APL_Float this_val  = get_real_value();

   switch(other->get_cell_type())
      {
        case CT_NONE:
        case CT_BASE:
             Assert(0);

        case CT_CHAR:
             {
               const Unicode other_val = other->get_char_value();
               if (this_val == other_val)   return !ascending;
               return this_val > other_val ? ascending : !ascending;
             }

        case CT_INT:
             {
               const APL_Integer other_val = other->get_int_value();
               if (this_val == other_val)   return !ascending;
               return this_val > other_val ? ascending : !ascending;
             }

        case CT_FLOAT:
             {
               const APL_Float other_val = other->get_real_value();
               if (this_val == other_val)   return this > other;
               return this_val > other_val ? ascending : !ascending;
             }

        case CT_COMPLEX:
        case CT_POINTER:
             DOMAIN_ERROR;

        default:
           Assert(0);
      }
}
//-----------------------------------------------------------------------------
bool
FloatCell::get_near_bool(APL_Float qct)  const
{
   if (value.fval > qct)   // maybe 1
      {
        if (value.fval > (1.0 + qct))   DOMAIN_ERROR;
        if (value.fval < (1.0 - qct))   DOMAIN_ERROR;
        return true;
      }

   // maybe 0. We already know that value.fval ≤ qct
   //
   if (value.fval < -qct)   DOMAIN_ERROR;
   return false;
}
//-----------------------------------------------------------------------------
void
FloatCell::demote_float_to_int(APL_Float qct)
{
   if (Cell::is_near_int(value.fval, qct))
      {
        if (value.fval < 0)
           {
             APL_Integer ival(0.3 - value.fval);
             new (this) IntCell(-ival);
           }
        else
           {
             APL_Integer ival(0.3 + value.fval);
             new (this) IntCell(ival);
           }
      }
}
//-----------------------------------------------------------------------------
// monadic built-in functions...
//-----------------------------------------------------------------------------
void FloatCell::bif_factorial(Cell * Z) const
{
const APL_Float qct = Workspace::get_CT();

   if (is_near_int(qct))
      {
        const APL_Integer iv = get_near_int(qct);
        IntCell icell(iv);
        icell.bif_factorial(Z);
        return;
      }

   // max N! that fits into double is about 170
   //
   if (value.fval > 170)   DOMAIN_ERROR;

const double arg = value.fval + 1.0;
   new (Z) FloatCell(tgamma(arg));
}
//-----------------------------------------------------------------------------
void FloatCell::bif_conjugate(Cell * Z) const
{
   if (is_near_int(Workspace::get_CT()))
      new (Z) IntCell(get_checked_near_int());
   else
      new (Z) FloatCell(value.fval);
}
//-----------------------------------------------------------------------------
void FloatCell::bif_negative(Cell * Z) const
{
   new (Z) FloatCell(- value.fval);
}
//-----------------------------------------------------------------------------
void
FloatCell::bif_direction(Cell * Z) const
{
   // bif_direction does NOT use ⎕CT
   //
const APL_Float qct = Workspace::get_CT();
   if      (value.fval == 0)   new (Z) IntCell( 0);
   else if (value.fval < 0)    new (Z) IntCell(-1);
   else                        new (Z) IntCell( 1);
}
//-----------------------------------------------------------------------------
void FloatCell::bif_magnitude(Cell * Z) const
{
   if (value.fval >= 0.0)   new (Z) FloatCell( value.fval);
   else                     new (Z) FloatCell(-value.fval);
}
//-----------------------------------------------------------------------------
void FloatCell::bif_reciprocal(Cell * Z) const
{
   if (is_near_zero(Workspace::get_CT()))  DOMAIN_ERROR;

   new (Z) FloatCell(1.0/value.fval);
}
//-----------------------------------------------------------------------------
void FloatCell::bif_roll(Cell * Z) const
{
const APL_Integer qio = Workspace::get_IO();
   if (!is_near_int(Workspace::get_CT()))   DOMAIN_ERROR;

const APL_Integer set_size = get_checked_near_int();
   if (set_size <= 0)   DOMAIN_ERROR;

const APL_Integer rnd = Workspace::get_RL();
   new (Z) IntCell(qio + (rnd % set_size));
}
//-----------------------------------------------------------------------------
void FloatCell::bif_pi_times(Cell * Z) const
{
   new (Z) FloatCell(M_PI * value.fval);
}
//-----------------------------------------------------------------------------
void
FloatCell::bif_ceiling(Cell * Z) const
{
   // we want values slightly above an int to be rounded down rather than up
   //
const double d_ret = ceil(value.fval - Workspace::get_CT());

   if (d_ret > BIG_INT64_F || d_ret < BIG_INT64_F)   new (Z) FloatCell(d_ret);
   else                                          new (Z) IntCell(llrint(d_ret));
}
//-----------------------------------------------------------------------------
void FloatCell::bif_floor(Cell * Z) const
{
   // we want values slightly below an int to be rounded up rather than down
   //
const double d_ret = floor(value.fval + Workspace::get_CT());

   if (d_ret > 9.22E18 || d_ret < -9.22E18)   new (Z) FloatCell(d_ret);
   else                                       new (Z) IntCell(llrint(d_ret));
}
//-----------------------------------------------------------------------------
void FloatCell::bif_exponential(Cell * Z) const
{
   new (Z) FloatCell(exp(value.fval));
}
//-----------------------------------------------------------------------------
void FloatCell::bif_nat_log(Cell * Z) const
{
   if (value.fval > 0)
      {
        new (Z) FloatCell(log(value.fval));
      }
   else if (value.fval < 0)
      {
        const double real = log(-value.fval);
        const double imag = M_PI;   // argz(-1)
        new (Z) ComplexCell(real, imag);
      }
   else
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
// dyadic built-in functions...
//-----------------------------------------------------------------------------
void
FloatCell::bif_residue(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   // if A is zero , return B
   //
   if (A->is_near_zero(0))
      {
        Z->init(*this);
        return;
      }

   // if ⎕CT != 0 and B÷A is close to an integer within ⎕CT then return zero.
   // Otherwise return B mod A
   //
const APL_Float remainder = fmod(get_real_value(), A->get_real_value()); 
   if (qct == 0.0)   // ⎕CT is 0
      {
        new (Z) FloatCell(remainder);
      }
   else if (Cell::is_near_zero(remainder, qct))
      {
        new (Z) IntCell(0);
      }
   else if (Cell::is_near_zero(A->get_real_value() - remainder, qct))
      {
        new (Z) IntCell(0);
      }
   else
      {
        new (Z) FloatCell(remainder);
      }
}
//-----------------------------------------------------------------------------
void FloatCell::bif_maximum(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
         const APL_Integer a = A->get_int_value();
         if (APL_Float(a) >= value.fval)   new (Z) IntCell(a);
         else                              new (Z) FloatCell(value.fval);
      }
   else if (A->is_float_cell())
      {
         const APL_Float a = A->get_real_value();
         if (a >= value.fval)   new (Z) FloatCell(a);
         else                   new (Z) FloatCell(value.fval);
      }
   else   // maximum not defined for complex.
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
FloatCell::bif_minimum(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
         const APL_Integer a = A->get_int_value();
         if (APL_Float(a) <= value.fval)   new (Z) IntCell(a);
         else                              new (Z) FloatCell(value.fval);
      }
   else if (A->is_float_cell())
      {
         const APL_Float a = A->get_real_value();
         if (a <= value.fval)   new (Z) FloatCell(a);
         else                   new (Z) FloatCell(value.fval);
      }
   else   // minimum not defined for complex.
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
FloatCell::bif_less_than(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   if (A->is_complex_cell())
      {
        // complex has no < so we allow this only for near-real numbers
        //
        if (!A->is_near_real(qct))   DOMAIN_ERROR;
           
        FloatCell  AA(A->get_real_value());
        if (equal(AA, qct))                          new (Z) IntCell(0);
        else if (AA.get_real_value() > value.fval)   new (Z) IntCell(1);
        else                                         new (Z) IntCell(0);
      }
   else if (A->is_numeric())
      {
        if (equal(*A, qct ))                         new (Z) IntCell(0);
        else if (A->get_real_value() < value.fval)   new (Z) IntCell(1);
        else                                         new (Z) IntCell(0);
      }
   else   // char cell
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
FloatCell::bif_greater_than(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   if (A->is_complex_cell())
      {
        // complex has no > so we allow this only for near-real numbers
        //
        if (!A->is_near_real(qct))   DOMAIN_ERROR;
           
        FloatCell AA(A->get_real_value());
        if (equal(AA, qct))                          new (Z) IntCell(0);
        else if (AA.get_real_value() < value.fval)   new (Z) IntCell(1);
        else                                         new (Z) IntCell(0);
      }
   else if (A->is_numeric())
      {
        if (equal(*A, Workspace::get_CT()))          new (Z) IntCell(0);
        else if (A->get_real_value() > value.fval)   new (Z) IntCell(1);
        else                                         new (Z) IntCell(0);
      }
   else   // char cell
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
FloatCell::bif_equal(Cell * Z, const Cell * A) const
{
   if (A->is_complex_cell())   // promote the request to complex
      {
        A->bif_equal(Z, this);
      }
   else if (A->is_numeric())
      {
        if (equal(*A, Workspace::get_CT()))    new (Z) IntCell(1);
        else                                   new (Z) IntCell(0);
      }
   else
      {
        new (Z) IntCell(0);
      }
}
//-----------------------------------------------------------------------------
PrintBuffer
FloatCell::character_representation(const PrintContext & pctx) const
{
bool scaled = pctx.get_scaled();   // may be changed by print function
UCS_string ucs = UCS_string(value.fval, scaled, pctx);

ColInfo info;
   info.flags |= CT_FLOAT;
   if (scaled)   info.flags |= real_has_E;

   // assume integer.
   //
int int_fract = ucs.size();;
   info.real_len = ucs.size();
   info.int_len = ucs.size();
   loop(u, ucs.size())
      {
        if (ucs[u] == UNI_ASCII_FULLSTOP)
           {
             info.int_len = u;
             if (!pctx.get_scaled())   break;
             continue;
           }

        if (ucs[u] == UNI_ASCII_E)
           {
             if (info.int_len > u)   info.int_len = u;
             int_fract = u;
             break;
           }
      }

   info.fract_len = int_fract - info.int_len;

   return PrintBuffer(ucs, info);
}
//-----------------------------------------------------------------------------
bool
FloatCell::is_big(APL_Float val, int quad_pp)
{
static const double big[MAX_QUAD_PP + 1] =
{
                  1ULL, // not used since MIN_QUAD_PP == 1
                 10ULL,
                100ULL,
               1000ULL,
              10000ULL,
             100000ULL,
            1000000ULL,
           10000000ULL,
          100000000ULL,
         1000000000ULL,
        10000000000ULL,
       100000000000ULL,
      1000000000000ULL,
     10000000000000ULL,
    100000000000000ULL,
   1000000000000000ULL,
  10000000000000000ULL,
};

   return val >= big[quad_pp] || val <= -big[quad_pp];
}
//-----------------------------------------------------------------------------
bool
FloatCell::need_scaling(APL_Float val, int quad_pp)
{
   // A number is printed in scaled format if (see lrm pp. 11-13) either:
   //
   // (1) its integer part is longer that quad-PP, or

   // (2a) is non-zero, and
   // (2b) its integer part is 0, and
   // (2c) its fractional part starts with at least 5 zeroes.
   //
   if (val < 0)   val = - val;   // simplify comparisons

   if (is_big(val, quad_pp))        return true;    // case 1.

   if (val == 0.0)                  return false;   // not 2a.

   if (val < 0.000001)              return true;    // case 2

   return false;
}
//-----------------------------------------------------------------------------
void
FloatCell::map_FC(UCS_string & ucs)
{
   loop(u, ucs.size())
      {
        switch(ucs[u])
           {
             case UNI_ASCII_FULLSTOP: ucs[u] = Workspace::get_FC(0);   break;
             case UNI_ASCII_COMMA:    ucs[u] = Workspace::get_FC(1);   break;
             case UNI_OVERBAR:        ucs[u] = Workspace::get_FC(5);   break;
           }
      }
}
//-----------------------------------------------------------------------------
