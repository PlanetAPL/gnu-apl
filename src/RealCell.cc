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

#include <math.h>

#include "Value.hh"
#include "Error.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "RealCell.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
void RealCell::bif_add(Cell * Z, const Cell * A) const
{
   if (A->is_complex_cell())
      {
        A->bif_add(Z, this);
      }
   else if (A->is_real_cell())
      {
        new (Z) FloatCell(A->get_real_value() + get_real_value());
        Z->demote_float_to_int(Workspace::get_CT());
      }
   else
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
RealCell::bif_subtract(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   if (A->is_complex_cell())
      {
        new (Z) ComplexCell(A->get_real_value() - get_real_value(),
                            A->get_imag_value());
        Z->demote_complex_to_real(qct);
        Z->demote_float_to_int(qct);
      }
   else if (A->is_real_cell())
      {
        new (Z) FloatCell(A->get_real_value() - get_real_value());
        Z->demote_float_to_int(qct);
      }
   else
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void RealCell::bif_multiply(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   if (A->is_complex_cell())
      {
        A->bif_multiply(Z, this);
      }
   else if (A->is_real_cell())
      {
        new (Z) FloatCell(A->get_real_value() * get_real_value());
        Z->demote_float_to_int(qct);
      }
   else
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
RealCell::bif_divide(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   if (this->is_near_zero(qct))   // divide by 0
      {
         if (!A->is_near_zero(qct))   DOMAIN_ERROR;
         new (Z) IntCell(1);   // 0/0 is 1 in APL
         return;
      }

const APL_Float real = A->get_real_value() / get_real_value();

   if (A->is_complex_cell())
      {
        const APL_Float imag = A->get_imag_value() / get_real_value();
        if (!is_near_zero(imag, qct))      new (Z) ComplexCell(real, imag);
        else if (is_near_int(real, qct))   new (Z) IntCell(real, qct);
        else                               new (Z) FloatCell(real);
      }
   else if (A->is_real_cell())
      {
        if (is_near_int(real, qct))   new (Z) IntCell(real, qct);
        else                          new (Z) FloatCell(real);
      }
   else
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
RealCell::bif_power(Cell * Z, const Cell * A) const
{
   if (A->is_real_cell())
      {
        if (A->get_real_value() < 0)
           {
             const APL_Complex z = pow(A->get_complex_value(),get_real_value());
             new (Z) ComplexCell(z);
             Z->demote_complex_to_real(Workspace::get_CT());
           }
        else
           {
             new (Z) FloatCell(pow(A->get_real_value(), get_real_value()));
           }
      }
   else if (A->is_complex_cell())
      {
        APL_Complex z = pow(A->get_complex_value(), get_real_value());

        new (Z) ComplexCell(z);
        Z->demote_complex_to_real(Workspace::get_CT());
      }
   else
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
RealCell::bif_logarithm(Cell * Z, const Cell * A) const
{
   // A⍟B is defined as: (⍟B)÷(⍟A)
   // if A=1 then ⍟A is 0 which causes division by 0 unless ⍟B is 0 as well.
   //
const APL_Float qct = Workspace::get_CT();
   if (A->is_near_one(qct))   // ⍟A is 0
      {
         if (this->is_near_one(qct))   // ⍟B is 0
            new (Z) IntCell(1);
         else
             DOMAIN_ERROR;
         return;
      }

   if (A->is_real_cell())
      {
        new (Z) FloatCell(log(get_real_value()) / log(A->get_real_value()));
      }
   else if (A->is_complex_cell())
      {
        new (Z) ComplexCell(log(get_real_value())/log(A->get_complex_value()));
      }
   else
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
RealCell::bif_circle_fun(Cell * Z, const Cell * A) const
{
const APL_Integer fun = A->get_near_int(Workspace::get_CT());
const APL_Float b = get_real_value();

   switch(fun)
      {
        case -12: ComplexCell(0, b).bif_exponential(Z);                  return;
        case -11: new (Z) ComplexCell(0.0, b );                          return;
        case -10: new (Z) FloatCell(      b );                           return;
        case  -9: new (Z) FloatCell(      b );                           return;
        case  -8: new (Z) ComplexCell(0, sqrt(1.0 + b*b));               return;
        case  -7: new (Z) FloatCell(atanh(b));                           return;
        case  -6: new (Z) FloatCell(acosh(b));                           return;
        case  -5: new (Z) FloatCell(asinh(b));                           return;
        case  -4: if (b > 1)   new (Z) FloatCell(sqrt(b*b - 1.0));
                  else         new (Z) ComplexCell(0, sqrt(1.0 - b*b));  return;
        case  -3: new (Z) FloatCell(atan (b));                           return;
        case  -2: new (Z) FloatCell(acos (b));                           return;
        case  -1: new (Z) FloatCell(asin (b));                           return;
        case   0: new (Z) FloatCell(sqrt (1 - b*b));                     return;
        case   1: new (Z) FloatCell( sin (b));                           return;
        case   2: new (Z) FloatCell( cos (b));                           return;
        case   3: new (Z) FloatCell( tan (b));                           return;
        case   4: new (Z) FloatCell(sqrt (1 + b*b));                     return;
        case   5: new (Z) FloatCell( sinh(b));                           return;
        case   6: new (Z) FloatCell( cosh(b));                           return;
        case   7: new (Z) FloatCell( tanh(b));                           return;
        case   8: new (Z) ComplexCell(0, -sqrt(1.0 + b*b));              return;
        case   9: new (Z) FloatCell(      b );                           return;
        case  10: new (Z) FloatCell((b < 0) ? -b : b);                   return;
        case  11: new (Z) FloatCell(    0.0 );                           return;
        case  12: new (Z) FloatCell(    0.0 );                           return;
      }

   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
