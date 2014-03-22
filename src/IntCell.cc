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
#include <math.h>

#include "Value.hh"
#include "Error.hh"
#include "PointerCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "Workspace.hh"

/*
   general comment: for operations between an IntCell and a "higher" numeric
   Cell type (i.e. FloatCell or ComplexCell), we try to call the corresponding
   function (or its opposite) of the higher type.

   For example:
   INT ≠ FLOAT  --> FLOAT ≠ INT
   INT ≤ FLOAT  --> FLOAT ≥ INT   (≤ is the opposite of ≥)

 */
//-----------------------------------------------------------------------------
bool
IntCell::greater(const Cell * other, bool ascending) const
{
const uint64_t this_val  = get_int_value();

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
               if (this_val == other_val)   return this > other;
               return this_val > other_val ? ascending : !ascending;
             }

        case CT_FLOAT:
             {
               const APL_Float other_val = other->get_real_value();
               if (this_val == other_val)   return true;
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
IntCell::equal(const Cell & other, APL_Float qct) const
{
   if (other.is_integer_cell())    return value.ival == other.get_int_value();
   if (!other.is_numeric())        return false;
   return other.equal(*this, qct);
}
//-----------------------------------------------------------------------------
// monadic built-in functions...
//-----------------------------------------------------------------------------
void
IntCell::bif_factorial(Cell * Z) const
{
const APL_Integer iv = value.ival;

   // N! fits in:
   // 32-bit signed integer for N <= 12
   // 64-bit signed integer for N <= 20
   // 64-bit float for N <= 170
   //
   
   if (iv < 0)     DOMAIN_ERROR;
   if (iv > 170)   DOMAIN_ERROR;

   if (iv < 21)   // integer result that fits into a signed 64 bit integer
      {
        const APL_Integer table[21] =
           {
             /*  0! */  0x0000000000000001LL,
             /*  1! */  0x0000000000000001LL,
             /*  2! */  0x0000000000000002LL,
             /*  3! */  0x0000000000000006LL,
             /*  4! */  0x0000000000000018LL,
             /*  5! */  0x0000000000000078LL,
             /*  6! */  0x00000000000002d0LL,
             /*  7! */  0x00000000000013b0LL,
             /*  8! */  0x0000000000009d80LL,
             /*  9! */  0x0000000000058980LL,
             /* 10! */  0x0000000000375f00LL,
             /* 11! */  0x0000000002611500LL,
             /* 12! */  0x000000001c8cfc00LL,
             /* 13! */  0x000000017328cc00LL,
             /* 14! */  0x000000144c3b2800LL,
             /* 15! */  0x0000013077775800LL,
             /* 16! */  0x0000130777758000LL,
             /* 17! */  0x0001437eeecd8000LL,
             /* 18! */  0x0016beecca730000LL,
             /* 19! */  0x01b02b9306890000LL,
             /* 20! */  0x21c3677c82b40000LL,
           };

        new (Z) IntCell(table[iv]);
        return;
      }

const double arg = double(iv + 1);
const double res = tgamma(arg);
   new (Z) FloatCell(res);
}
//-----------------------------------------------------------------------------
void IntCell::bif_conjugate(Cell * Z) const
{
   new (Z) IntCell(value.ival);
}
//-----------------------------------------------------------------------------
void IntCell::bif_negative(Cell * Z) const
{
   if (value.ival == 0x8000000000000000LL)   // integer overflow
      new (Z) FloatCell(- value.ival);
   else
      new (Z) IntCell(- value.ival);
}
//-----------------------------------------------------------------------------
void IntCell::bif_direction(Cell * Z) const
{
   if      (value.ival > 0)   new (Z) IntCell( 1);
   else if (value.ival < 0)   new (Z) IntCell(-1);
   else                       new (Z) IntCell( 0);
}
//-----------------------------------------------------------------------------
void IntCell::bif_magnitude(Cell * Z) const
{
   if (value.ival >= 0)   new (Z) IntCell( value.ival);
   else                   new (Z) IntCell(-value.ival);
}
//-----------------------------------------------------------------------------
void IntCell::bif_reciprocal(Cell * Z) const
{
   switch(value.ival)
      {
        case  0: DOMAIN_ERROR;
        case  1: new (Z) IntCell(1);    return;
        case -1: new (Z) IntCell(-1);   return;
        default: new (Z) FloatCell(1.0/value.ival);
      }
}
//-----------------------------------------------------------------------------
void IntCell::bif_roll(Cell * Z) const
{
const APL_Integer qio = Workspace::get_IO();
const APL_Integer set_size = value.ival;
   if (set_size <= 0)   DOMAIN_ERROR;

const APL_Integer rnd = Workspace::get_RL();
   new (Z) IntCell(qio + (rnd % set_size));
}
//-----------------------------------------------------------------------------
void IntCell::bif_pi_times(Cell * Z) const
{
   new (Z) FloatCell(M_PI * value.ival);
}
//-----------------------------------------------------------------------------
void IntCell::bif_ceiling(Cell * Z) const
{
   new (Z) IntCell(value.ival);
}
//-----------------------------------------------------------------------------
void IntCell::bif_floor(Cell * Z) const
{
   new (Z) IntCell(value.ival);
}
//-----------------------------------------------------------------------------
void IntCell::bif_exponential(Cell * Z) const
{
   new (Z) FloatCell(exp(value.ival));
}
//-----------------------------------------------------------------------------
void IntCell::bif_nat_log(Cell * Z) const
{
   if (value.ival > 0)
      {
        new (Z) FloatCell(log(value.ival));
      }
   else if (value.ival < 0)
      {
        const double real = log(-value.ival);
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
void IntCell::bif_residue(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   if (A->is_near_zero(qct))   // 0∣B is B
      {
        Z->init(*this);
      }
   else if (value.ival == 0)   // B∣0 is 0
      {
        new (Z) IntCell(0);
      }
   else if (A->is_integer_cell())
      {
         APL_Integer rest = value.ival % A->get_int_value();
         if (A->get_int_value() < 0)   // A negative -> Z negative
            {
              if (rest > 0)   rest += A->get_int_value();   // += since A < 0
            }
         else                          // A positive -> Z positive
            {
              if (rest < 0)   rest += A->get_int_value();
            }
         new (Z) IntCell(rest);
      }
   else if (A->is_real_cell())
      {
         const APL_Float valA = A->get_real_value();
         const APL_Float f_quot = value.ival / valA;
         APL_Integer i_quot = floor(f_quot);

         // compute i_quot with respect to ⎕CT
         //
         if (FloatCell::equal_within_quad_CT(i_quot + 1, f_quot, qct))
            ++i_quot;

         APL_Float rest = value.ival - valA * i_quot;
         if (Cell::is_near_zero(rest, qct))
            {
              new (Z) IntCell(0);
              return;
            }

         if (valA < 0)   // A negative -> Z negative
            {
              if (rest > 0)   rest += valA;   // += since A < 0
            }
          else
            {
              if (rest < 0)   rest += valA;
            }
          new (Z) FloatCell(rest);
      }
   else if (A->is_numeric())   // i.e. complex
      {
        const APL_Complex a = A->get_complex_value();
        const APL_Complex b(value.ival);

        // We divide A by B, round down the result, and subtract.
        //
        A->bif_divide(Z, this);      // Z = B/A
        Z->bif_floor(Z);             // Z = A/B rounded down.
        new (Z) ComplexCell(b - a*Z->get_complex_value());
      }
   else
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void IntCell::bif_maximum(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
         const APL_Integer a = A->get_int_value();
         if (a >= value.ival)   new (Z) IntCell(a);
         else                   new (Z) IntCell(value.ival);
      }
   else if (A->is_float_cell())
      {
         const APL_Float a = A->get_real_value();
         if (a >= APL_Float(value.ival))   new (Z) FloatCell(a);
         else                              new (Z) IntCell(value.ival);
      }
   else   // maximum not defined for complex.
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void IntCell::bif_minimum(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
         const APL_Integer a = A->get_int_value();
         if (a <= value.ival)   new (Z) IntCell(a);
         else                   new (Z) IntCell(value.ival);
      }
   else if (A->is_float_cell())
      {
         const APL_Float a = A->get_real_value();
         if (a <= APL_Float(value.ival))   new (Z) FloatCell(a);
         else                              new (Z) IntCell(value.ival);
      }
   else   // minimum not defined for complex.
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void IntCell::bif_less_than(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
        new (Z) IntCell(A->get_int_value() < value.ival ? 1 : 0);
      }
   else if (A->is_numeric())   // promote the request to float or complex
      {
        A->bif_greater_than(Z, this);
      }
   else   // char cell
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void IntCell::bif_greater_than(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
        new (Z) IntCell(A->get_int_value() > value.ival ? 1 : 0);
      }
   else if (A->is_numeric())   // promote the request to float or complex
      {
        A->bif_less_than(Z, this);
      }
   else   // char cell
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
IntCell::bif_equal(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
        new (Z) IntCell(A->get_int_value() == value.ival ? 1 : 0);
      }
   else if (A->is_numeric())   // promote the request to float or complex
      {
        A->bif_equal(Z, this);
      }
   else   // char cell
      {
        new (Z) IntCell(0);
      }
}
//-----------------------------------------------------------------------------
PrintBuffer
IntCell::character_representation(const PrintContext & pctx) const
{
   if (pctx.get_scaled())
      {
        FloatCell fc(get_int_value());
        return fc.character_representation(pctx);
      }

char cc[40];
int len = snprintf(cc, sizeof(cc), "%lld", value.ival);

UCS_string ucs;

   loop(l, len)
       {
         if (cc[l] == '-')   ucs.append(UNI_OVERBAR);
         else                ucs.append(Unicode(cc[l]));
       }

ColInfo info;
   info.flags |= CT_INT;
   info.real_len = ucs.size();
   info.int_len = ucs.size();
   
   return PrintBuffer(ucs, info);
}
//-----------------------------------------------------------------------------
int
IntCell::CDR_size() const
{
   // use 4 byte for small integers and 8 bytes for others (converted to float).
   //
const APL_Integer val = get_int_value();
const int32_t high = val >> 32;

   if (val == 0)              return 4;
   if (val == 0xFFFFFFFF)     return 4;
   return 8;
}
//-----------------------------------------------------------------------------
