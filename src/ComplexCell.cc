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
#include <stdio.h>
#include <string.h>

#include "ComplexCell.hh"
#include "Error.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "Output.hh"
#include "Value.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
ComplexCell::ComplexCell(APL_Complex c)
{
   value.cpxp = new APL_Complex(c);
   Log(LOG_delete)
      CERR << "new    " << (const void *)value.cpxp << " at " LOC << endl;
}
//-----------------------------------------------------------------------------
ComplexCell::ComplexCell(APL_Float r, APL_Float i)
{
   value.cpxp = new APL_Complex(r, i);
   Log(LOG_delete)
      CERR << "new    " << (const void *)value.cpxp << " at " LOC << endl;
}
//-----------------------------------------------------------------------------
ComplexCell::~ComplexCell()
{
   Log(LOG_delete)   CERR << "delete " HEX(value.cpxp) << " at " LOC << endl;
   delete value.cpxp;
}
//-----------------------------------------------------------------------------
APL_Float
ComplexCell::get_real_value() const
{
   return value.cpxp->real();
}
//-----------------------------------------------------------------------------
APL_Float
ComplexCell::get_imag_value() const
{
   return value.cpxp->imag();
}
//-----------------------------------------------------------------------------
bool
ComplexCell::get_near_bool(APL_Float qct)  const   
{
   if (value.cpxp->imag() >  qct)   DOMAIN_ERROR;
   if (value.cpxp->imag() < -qct)   DOMAIN_ERROR;

   if (value.cpxp->real() > qct)   // 1 or invalid
      {
        if (value.cpxp->real() < (1.0 - qct))   DOMAIN_ERROR;
        if (value.cpxp->real() > (1.0 + qct))   DOMAIN_ERROR;
        return true;
      }
   else
      {
        if (value.cpxp->real() < -qct)   DOMAIN_ERROR;
        return false;
      }
}
//-----------------------------------------------------------------------------
APL_Integer
ComplexCell::get_near_int(APL_Float qct) const
{
// if (value.cpxp->imag() >  qct)   DOMAIN_ERROR;
// if (value.cpxp->imag() < -qct)   DOMAIN_ERROR;

const double val = value.cpxp->real();
const double result = round(val);
const double diff = val - result;
   if (diff > qct)    DOMAIN_ERROR;
   if (diff < -qct)   DOMAIN_ERROR;

   return APL_Integer(result + 0.3);
}
//-----------------------------------------------------------------------------
bool
ComplexCell::is_near_int(APL_Float qct) const
{
   return Cell::is_near_int(value.cpxp->real(), qct) &&
          Cell::is_near_int(value.cpxp->imag(), qct);
}
//-----------------------------------------------------------------------------
void
ComplexCell::demote_complex_to_real(APL_Float qct)
{
const bool small_real = value.cpxp->real() <  qct &&
                        value.cpxp->real() >  -qct;

const bool small_imag = value.cpxp->imag() <  qct &&
                        value.cpxp->imag() >  -qct;

const APL_Float qct1 = 100000*qct;
const bool large_real = value.cpxp->real() >  qct1 ||
                        value.cpxp->real() <  -qct1;

const bool large_imag = value.cpxp->imag() >  qct1 ||
                        value.cpxp->imag() <  -qct1;

   if (small_imag && large_real)
      {
        // imag is close to zero, so we demote this cell either
        // to a FloatCell, or to an IntCell
        //
        const double val = value.cpxp->real();
        delete value.cpxp;

        if (Cell::is_near_int(val, qct))
           {
             if (val < 0)   new (this)   IntCell(APL_Integer(val - 0.3));
             else           new (this)   IntCell(APL_Integer(val + 0.3));
           }
        else
           {
             new (this) FloatCell(val);
           }
      }
   else if (small_real && large_imag)
      {
        // real is close to zero, so we round it down.
        //
        *value.cpxp = APL_Complex(0.0, value.cpxp->imag());
      }
}
//-----------------------------------------------------------------------------
bool
ComplexCell::is_near_zero(APL_Float qct) const
{
   if (value.cpxp->real() >  qct)   return false;
   if (value.cpxp->real() < -qct)   return false;
   if (value.cpxp->imag() >  qct)   return false;
   if (value.cpxp->imag() < -qct)   return false;
   return true;
}
//-----------------------------------------------------------------------------
bool
ComplexCell::is_near_one(APL_Float qct) const
{
   if (value.cpxp->real() >  (1.0 + qct))   return false;
   if (value.cpxp->real() <  (1.0 - qct))   return false;
   if (value.cpxp->imag() >  qct)           return false;
   if (value.cpxp->imag() < -qct)           return false;
   return true;
}
//-----------------------------------------------------------------------------
bool
ComplexCell::equal(const Cell & A, APL_Float qct) const
{
   if (!A.is_numeric())   return false;

   {
     const APL_Float valA = A.get_real_value();
     const APL_Float valB =   get_real_value();
     const APL_Float posA = valA < 0 ? -valA : valA;
     const APL_Float posB = valB < 0 ? -valB : valB;
     const APL_Float maxAB = posA > posB ? posA : posB;
     const APL_Float maxCT = qct * maxAB;

     if (valB < (valA - maxCT))   return false;
     if (valB > (valA + maxCT))   return false;
   }

   {
     const APL_Float valA = A.get_imag_value();
     const APL_Float valB =   get_imag_value();
     const APL_Float posA = valA < 0 ? -valA : valA;
     const APL_Float posB = valB < 0 ? -valB : valB;
     const APL_Float maxAB = posA > posB ? posA : posB;
     const APL_Float maxCT = qct * maxAB;

     if (valB < (valA - maxCT))   return false;
     if (valB > (valA + maxCT))   return false;
   }

   return true;
}
//-----------------------------------------------------------------------------
bool
ComplexCell::greater(const Cell * other, bool ascending) const
{
   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
void
ComplexCell::release(const char * loc)
{
   delete value.cpxp;
   new (this) Cell;
}
//-----------------------------------------------------------------------------
// monadic build-in functions...
//-----------------------------------------------------------------------------
void
ComplexCell::bif_factorial(Cell * Z) const
{
   if (is_near_real(Workspace::get_CT()))
      {
        const FloatCell fc(get_real_value());
        fc.bif_factorial(Z);
        return;
      }

const APL_Float zr = get_real_value() + 1.0;
const APL_Float zi = get_imag_value();
const APL_Complex z(zr, zi);

   new (Z) ComplexCell(gamma(get_real_value(), get_imag_value()));
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_conjugate(Cell * Z) const
{
   new (Z) ComplexCell(conj(*value.cpxp));
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_negative(Cell * Z) const
{
   new (Z) ComplexCell(- *value.cpxp);
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_direction(Cell * Z) const
{
const APL_Complex val = *value.cpxp; 
const APL_Float mag = abs(val);
   if (mag == 0.0)   new (Z) IntCell(0);
   else              new (Z) ComplexCell(val/mag);
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_magnitude(Cell * Z) const
{
   new (Z) FloatCell(abs(*value.cpxp));
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_reciprocal(Cell * Z) const
{
const APL_Float qct = Workspace::get_CT();
   if (is_near_zero(qct))  DOMAIN_ERROR;

   if (is_near_real(qct))   new (Z) FloatCell(1.0/value.cpxp->real());
   else                     new (Z) ComplexCell(APL_Complex(1.0)/ *value.cpxp);
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_roll(Cell * Z) const
{
const APL_Integer qio = Workspace::get_IO();
   if (!is_near_int(Workspace::get_CT()))   DOMAIN_ERROR;

const APL_Integer set_size = get_checked_near_int();
   if (set_size <= 0)   DOMAIN_ERROR;

const APL_Integer rnd = Workspace::get_RL();
   new (Z) IntCell(qio + (rnd % set_size));
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_pi_times(Cell * Z) const
{
   new (Z) ComplexCell(M_PI * *value.cpxp);
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_ceiling(Cell * Z) const
{
const double r = value.cpxp->real();
const double i = value.cpxp->imag();
const double dr = r - floor(r);
const double di = i - floor(i);

   if (dr + di > 1.0)   // round real and imag up.
      new (Z) ComplexCell(ceil(r), ceil(i));
   else if (di >= dr)   // round real down and imag up
      new (Z) ComplexCell(floor(r), ceil(i));
   else                 // round real up and imag down
      new (Z) ComplexCell(ceil(r), floor(i));
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_floor(Cell * Z) const
{
const double r = value.cpxp->real();
const double i = value.cpxp->imag();
const double dr = r - floor(r);
const double di = i - floor(i);

   if (dr + di < 1.0)   // round real and imag down.
      new (Z) ComplexCell(floor(r), floor(i));
   else if (di > dr)   // round real down and imag up
      new (Z) ComplexCell(floor(r), ceil (i));
   else                // round real up and imag down
      new (Z) ComplexCell(ceil (r), floor(i));
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_exponential(Cell * Z) const
{
   new (Z) ComplexCell(exp(*value.cpxp));
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_nat_log(Cell * Z) const
{
   new (Z) ComplexCell(log(*value.cpxp));
   Z->demote_complex_to_real(Workspace::get_CT());
}
//-----------------------------------------------------------------------------
// dyadic build-in functions...
//-----------------------------------------------------------------------------
void ComplexCell::bif_add(Cell * Z, const Cell * A) const
{
   new (Z) ComplexCell(A->get_complex_value() + get_complex_value());
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_subtract(Cell * Z, const Cell * A) const
{
   new (Z) ComplexCell(A->get_complex_value() - get_complex_value());
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_multiply(Cell * Z, const Cell * A) const
{
   new (Z) ComplexCell(A->get_complex_value() * get_complex_value());
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_divide(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   if (is_near_zero(qct))
      {
         if (A->is_near_zero(qct))   new (Z) IntCell(1);
         else                        DOMAIN_ERROR;
      }

   new (Z) ComplexCell(A->get_complex_value() / get_complex_value());
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_residue(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   if (A->is_near_zero(qct))
      {
        Z->init(*this);
      }
   else if (is_near_zero(qct))
      {
        new (Z) IntCell(0);
      }
   else if (A->is_numeric())
      {
        const APL_Complex a = A->get_complex_value();
        const APL_Complex b = *value.cpxp;

        // We divide A by B, round down the result, and subtract.
        //
        A->bif_divide(Z, this);     // Z = B/A
        Z->bif_floor(Z);            // Z = A/B rounded down.
        new (Z) ComplexCell(b - a*Z->get_complex_value());
      }
   else
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_equal(Cell * Z, const Cell * A) const
{
   if (A->is_complex_cell())
      {
        const APL_Float qct = Workspace::get_CT();
        const APL_Complex diff = A->get_complex_value() - *value.cpxp;
        if      (diff.real() >  qct)   new (Z) IntCell(0);
        else if (diff.real() < -qct)   new (Z) IntCell(0);
        else if (diff.imag() >  qct)   new (Z) IntCell(0);
        else if (diff.imag() > -qct)   new (Z) IntCell(0);
        else                           new (Z) IntCell(1);
      }
   else
      {
        new (Z) IntCell(0);
      }
}
//-----------------------------------------------------------------------------
void ComplexCell::bif_power(Cell * Z, const Cell * A) const
{
   if (A->is_complex_cell())
      {
        APL_Complex z = pow(A->get_complex_value(), get_complex_value());
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
ComplexCell::bif_logarithm(Cell * Z, const Cell * A) const
{
   if (A->is_real_cell())
      {
        new (Z) ComplexCell(log(*value.cpxp) / log(A->get_real_value()));
      }
   else if (A->is_complex_cell())
      {
        new (Z) ComplexCell(log(*value.cpxp) / log(A->get_complex_value()));
      }
   else
      {
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
ComplexCell::bif_circle_fun(Cell * Z, const Cell * A) const
{
const APL_Integer fun = A->get_near_int(Workspace::get_CT());
const APL_Complex b = *value.cpxp;

   switch(fun)
      {
        case -12: ComplexCell(-b.imag(), b.real()).bif_exponential(Z);   return;
        case -11: new (Z) ComplexCell(-b.imag(), b.real());              return;
        case -10: new (Z) ComplexCell(b.real(), -b.imag());              return;
        case  -9: new (Z) ComplexCell(            b   );                 return;
        case  -7: // arctanh(z) = 0.5 (ln(1 + z) - ln(1 - z))
                  {
                    const APL_Complex b1      = ONE() + b;
                    const APL_Complex b_1     = ONE() - b;
                    const APL_Complex log_b1  = log(b1);
                    const APL_Complex log_b_1 = log(b_1);
                    const APL_Complex diff    = log_b1 - log_b_1;
                    const APL_Complex half    = 0.5 * diff;
                    new (Z) ComplexCell(half);                           return;
                  }
        case  -6: // arccosh(z) = ln(z + sqrt(z + 1) sqrt(z - 1))
                  {
                    const APL_Complex b1     = b + ONE();
                    const APL_Complex b_1    = b - ONE();
                    const APL_Complex root1  = sqrt(b1);
                    const APL_Complex root_1 = sqrt(b_1);
                    const APL_Complex prod   = root1 * root_1;
                    const APL_Complex sum    = b + prod;
                    const APL_Complex loga   = log(sum);
                    new (Z) ComplexCell(loga);                           return;
                  }
        case  -5: // arcsinh(z) = ln(z + sqrt(z^2 + 1))
                  {
                    const APL_Complex b2 = b*b;
                    const APL_Complex b2_1 = b2 + ONE();
                    const APL_Complex root = sqrt(b2_1);
                    const APL_Complex sum =  b + root;
                    const APL_Complex loga = log(sum);
                    new (Z) ComplexCell(loga);                           return;
                  }
        case  -4: new (Z) ComplexCell(sqrt(b*b - 1.0));                  return;
        case  -3: // arctan(z) = i/2 (ln(1 - iz) - ln(1 + iz))
                  {
                    const APL_Complex iz = APL_Complex(- b.imag(), b.real());
                    const APL_Complex piz = ONE() + iz;
                    const APL_Complex niz = ONE() - iz;
                    const APL_Complex log_piz = log(piz);
                    const APL_Complex log_niz = log(niz);
                    const APL_Complex diff = log_niz - log_piz;
                    const APL_Complex prod = APL_Complex(0, 0.5) * diff;
                    new (Z) ComplexCell(prod);                           return;
                  }
        case  -2: // arccos(z) = -i (ln( z + sqrt(z^2 - 1)))
                  {
                    const APL_Complex b2 = b*b;
                    const APL_Complex diff = b2 - ONE();
                    const APL_Complex root = sqrt(diff);
                    const APL_Complex sum = b + root;
                    const APL_Complex loga = log(sum);
                    const APL_Complex prod = MINUS_i() * loga;
                    new (Z) ComplexCell(prod);                           return;
                  }
        case  -1: // arcsin(z) = -i (ln(iz + sqrt(1 - z^2)))
                  {
                    const APL_Complex b2 = b*b;
                    const APL_Complex diff = ONE() - b2;
                    const APL_Complex root = sqrt(diff);
                    const APL_Complex sum  = APL_Complex(-b.imag(), b.real())
                                           + root;
                    const APL_Complex loga = log(sum);
                    const APL_Complex prod = MINUS_i() * loga;
                    new (Z) ComplexCell(prod);                           return;
                  }
        case   0: new (Z) ComplexCell(sqrt (1.0 - b*b));                 return;
        case   1: new (Z) ComplexCell( sin       (b));                   return;
        case   2: new (Z) ComplexCell( cos       (b));                   return;
        case   3: new (Z) ComplexCell( tan       (b));                   return;
        case   4: new (Z) ComplexCell(sqrt (1.0 + b*b));                 return;
        case   5: new (Z) ComplexCell( sinh      (b));                   return;
        case   6: new (Z) ComplexCell( cosh      (b));                   return;
        case   7: new (Z) ComplexCell( tanh      (b));                   return;
        case  -8:
        case   8: {
                    bool pos_8 = false;
                    if (b.real() >  0)      { if (b.imag() > 0)  pos_8 = true; }
                    else if (b.real() == 0) { if (b.imag() > 1)  pos_8 = true; }
                    else                    { if (b.imag() >= 0) pos_8 = true; }

                    if (fun == -8)   pos_8 = ! pos_8;

                    const APL_Complex sq = sqrt(-(1.0 + get_complex_value()
                                                      * get_complex_value()));

                    if (pos_8)   new (Z) ComplexCell(sq);
                    else         new (Z) ComplexCell(-sq);
                  }                                                      return;

        case   9: new (Z) FloatCell(     b.real());                      return;
        case  10: new (Z) FloatCell(sqrt(b.real()*b.real()
                                       + b.imag()* b.imag()));           return;
        case  11: new (Z) FloatCell(b.imag());                           return;
        case  12: new (Z) FloatCell(phase());                            return;
      }

   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
APL_Float
ComplexCell::phase() const
{
   if (is_near_zero(Workspace::get_CT()))   return 0.0;

APL_Float real = value.cpxp->real();
APL_Float imag = value.cpxp->imag();

   return atan2(imag, real);
}
//-----------------------------------------------------------------------------
APL_Complex
ComplexCell::gamma(APL_Float x, const APL_Float y)
{
   if (x < 0.5)
      return M_PI / sin(M_PI * APL_Complex(x, y)) * gamma(1 - x, -y);

   // coefficients for lanczos approximation of the gamma function.
   //
#define p0 APL_Complex(  1.000000000190015   )
#define p1 APL_Complex( 76.18009172947146    )
#define p2 APL_Complex(-86.50532032941677    )
#define p3 APL_Complex( 24.01409824083091    )
#define p4 APL_Complex( -1.231739572450155   )
#define p5 APL_Complex(  1.208650973866179E-3)
#define p6 APL_Complex( -5.395239384953E-6   )

   errno = 0;
   feclearexcept(FE_ALL_EXCEPT);

   x += 1.0;

const APL_Complex z(x, y);

const APL_Complex ret( (sqrt(2*M_PI) / z)
                      * (p0                            +
                          p1 / APL_Complex(x + 1.0, y) +
                          p2 / APL_Complex(x + 2.0, y) +
                          p3 / APL_Complex(x + 3.0, y) +
                          p4 / APL_Complex(x + 4.0, y) +
                          p5 / APL_Complex(x + 5.0, y) +
                          p6 / APL_Complex(x + 6.0, y))
                       * pow(z + 5.5, z + 0.5)
                       * exp(-(z + 5.5))
                     );

   if (errno)   DOMAIN_ERROR;

   return ret;

#undef p0
#undef p1
#undef p2
#undef p3
#undef p4
#undef p5
#undef p6
}
//-----------------------------------------------------------------------------
PrintBuffer
ComplexCell::character_representation(const PrintContext & pctx) const
{
bool scaled_real = pctx.get_scaled();   // may be changed by print function
UCS_string ucs(value.cpxp->real(), scaled_real, pctx);

ColInfo info;
   info.flags |= CT_COMPLEX;
   if (scaled_real)   info.flags |= real_has_E;
int int_fract = ucs.size();;
   info.real_len = ucs.size();
   info.int_len = ucs.size();
   loop(u, ucs.size())
      {
       if (ucs[u] == UNI_ASCII_FULLSTOP)
           {
             info.int_len = u;
             if (!scaled_real)   break;
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

   if (!is_near_real(Workspace::get_CT()))
      {
        ucs.append(UNI_ASCII_J);
        bool scaled_imag = pctx.get_scaled();  // may be changed by UCS_string()
        const UCS_string ucs_i(value.cpxp->imag(), scaled_imag, pctx);

        ucs.append(ucs_i);

        info.imag_len = ucs.size() - info.real_len;
        if (scaled_imag)   info.flags |= imag_has_E;
      }

   return PrintBuffer(ucs, info);
}
//-----------------------------------------------------------------------------
bool
ComplexCell::need_scaling(const PrintContext &pctx) const
{
   // a complex number needs scaling if the real part needs it, ot
   // the complex part is significant and needs it.
   return FloatCell::need_scaling(value.cpxp->real(), pctx.get_PP()) ||
          (!is_near_real(Workspace::get_CT()) && 
          FloatCell::need_scaling(value.cpxp->imag(), pctx.get_PP()));
}
//-----------------------------------------------------------------------------
