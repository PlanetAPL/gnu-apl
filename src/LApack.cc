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

#include <iostream>

#include "Common.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "Output.hh"
#include "Value.hh"

using namespace std;

#if HAVE_LIBLAPACK   // liblapack is installed.found

/**

   For this code to compile you need liblapack which has the following
   copyrights:

Copyright (c) 1992-2011 The University of Tennessee and The University
                        of Tennessee Research Foundation.  All rights
                        reserved.
Copyright (c) 2000-2011 The University of California Berkeley. All
                        rights reserved.
Copyright (c) 2006-2011 The University of Colorado Denver.  All rights
                        reserved.

   liblapack is NOT distributed with GNU APL; you can download it from
   http://www.netlib.org/lapack/ and install it before compiling GNU APL.

   On Debian systems it might be eassier to just do this:

   apt-get install liblapack-dev

**/

/// An integer.
#ifdef __LP64__
typedef int integer;
#else
typedef long integer;
#endif

/// A Floating point number.
typedef double doublereal;

/// A complex number.
struct doublecomplex
{
  double r;   ///< The real part.
  double i;   ///< The imaginary part.
};

extern "C"
{
int dgelsy_(integer *m, integer *n, integer *nrhs,
        doublereal *a, integer *lda, doublereal *b, integer *ldb, integer *
        jpvt, doublereal *rcond, integer *rank, doublereal *work, integer *
        lwork, integer *info);

int zgelsy_(integer *m, integer *n, integer *nrhs,
        doublecomplex *a, integer *lda, doublecomplex *b, integer *ldb,
        integer *jpvt, doublereal *rcond, integer *rank, doublecomplex *work,
        integer *lwork, doublereal *rwork, integer *info);
}
//-----------------------------------------------------------------------------
static int
complex_matrix_divide(integer /* rows */ m, integer /* cols */ n,
                      doublecomplex *a, doublecomplex * b)
{
integer       nrhs = 1;   // number of result columns.
integer       lda = m; 
integer       ldb = m;
integer       jpvt[n];
doublereal    rcond = 0.0;
integer       rank;
integer       lwork;
doublereal    rwork[2*n];
integer       info;

   // first, compute the optimal work size...
   {
     doublecomplex work[100];
     lwork = -1;
     zgelsy_(&m, &n, &nrhs, a, &lda, b, &ldb, jpvt, &rcond, &rank, work,
             &lwork, rwork, &info);

     if (info)
        CERR << "info = " << info << " in pass 1 of dgelsy_()" << endl;

     lwork = 10 + integer(work[0].r);
   }

   // Then compute the result.
   {
     doublecomplex work[lwork];
     zgelsy_(&m, &n, &nrhs, a, &lda, b, &ldb, jpvt, &rcond, &rank, work,
             &lwork, rwork, &info);

     if (info)
        CERR << "info = " << info << " in pass 2 of dgelsy_()" << endl;
   }

   return info;
}
//-----------------------------------------------------------------------------
static int
real_matrix_divide(integer /* rows */ m, integer /* cols */ n,
                   doublereal *a, doublereal * b)
{
integer       nrhs = 1;   // number of result columns.
integer       lda = m; 
integer       ldb = m;
integer       jpvt[n];
doublereal    rcond = 0.0;
integer       rank;
integer       lwork;
integer       info;

   // First, compute the optimal work size...
   {
     doublereal work[100];
     lwork = -1;
     dgelsy_(&m, &n, &nrhs, a, &lda, b, &ldb, jpvt, &rcond, &rank, work,
             &lwork, &info);

     if (info)
        CERR << "info = " << info << " in pass 1 of dgelsy_()" << endl;
     lwork = 10 + integer(work[0]);
   }

   // Then compute the result.
   {
     doublereal work[lwork];
     dgelsy_(&m, &n, &nrhs, a, &lda, b, &ldb, jpvt, &rcond, &rank, work,
             &lwork, &info);

     if (info)
        CERR << "info = " << info << " in pass 2 of dgelsy_()" << endl;
   }

   return info;
}
//-----------------------------------------------------------------------------
Value_P
divide_matrix(ShapeItem rows, ShapeItem cols_A, Value_P A, ShapeItem cols_B,
              Value_P B, const Shape & shape_Z, APL_Float qct)
{
const bool need_complex = A->is_complex(qct) || B->is_complex(qct);

Value_P Z(new Value(shape_Z, LOC));

   loop(c, cols_A)
       {
         if (need_complex)
            {
              doublecomplex a[rows];
              loop(r, rows)
                  {
                    a[r].r = A->get_ravel(r*cols_A + c).get_real_value();
                    a[r].i = A->get_ravel(r*cols_A + c).get_imag_value();
                  }

              doublecomplex b[rows*cols_B];
              int bb = 0;
              loop(rr, cols_B)
              loop(cc, rows)
                 {
                   b[bb]  .r = B->get_ravel(rr + cc*cols_B).get_real_value();
                   b[bb++].i = B->get_ravel(rr + cc*cols_B).get_imag_value();
                 }

              const int result = complex_matrix_divide(rows, cols_B, b, a);
              if (result)
                 {
                   DOMAIN_ERROR;
                 }

              loop(r, shape_Z.get_cols())
                  new (&Z->get_ravel(r*cols_A + c)) ComplexCell(a[r].r, a[r].i);
            }
         else   // real
            {
              double a[rows];
              loop(r, rows)
                 {
                   a[r] = A->get_ravel(r*cols_A + c).get_real_value();
                 }

              double b[rows*cols_B];
              int bb = 0;
              loop(rr, cols_B)
              loop(cc, rows)
                 {
                   b[bb++] = B->get_ravel(rr + cc*cols_B).get_real_value();
                 }

              const int result = real_matrix_divide(rows, cols_B, b, a);
              if (result)
                 {
                   DOMAIN_ERROR;
                 }

              loop(r, shape_Z.get_cols())
                  new (&Z->get_ravel(r*cols_A + c)) FloatCell(a[r]);
            }
       }

   return Z;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#if 0   // divide_matrix() without liblapack

/*
   this is an attempt to remove the dependency on liblapack in order to
   simplify the installation of APL.

   Not finished yet, volunteers are welcome.
*/
//-----------------------------------------------------------------------------

/// helper function to init APL_Float values
inline void init_number(APL_Float & dest, const Cell & cell)
   { dest = cell.get_real_value(); }

/// helper function to init APL_Complex values
inline void init_number(APL_Complex & dest, const Cell & cell)
   { dest.real(cell.get_real_value());
     dest.imag(cell.get_imag_value()); }


/// a real or complex vector
template<class T>
class Tvector
{
public:
   /// constructor from a pointer into Tmatrix data
   Tvector(ShapeItem _len, ShapeItem _spacing, T * _data)
   : len(_len),
     spacing(_spacing ? _spacing : 1),
     colvec(_spacing != 0),
     data(_data)
   {}

   /// return idx'th element of this vector
   T & operator[](int idx)
   { Assert1(idx >= 0);   Assert1(idx < len);   return data[idx * spacing]; }

   /// return idx'th element of this vector
   const T & operator[](int idx) const
   { Assert1(idx >= 0);   Assert1(idx < len);   return data[idx * spacing]; }

   /// return the number of elements
   ShapeItem get_len() const
      { return len; }

protected:
   /// number of elements
   const ShapeItem len;

   /// distance between elements
   const ShapeItem spacing;

   /// true for a column vector
   bool colvec;

   /// the vector elements
   T * data;
};

/// a real or complex matrix
template<class T>
class Tmatrix
{
public:
   /// constructor from an APL value
   Tmatrix(ShapeItem _rows, ShapeItem _cols, Value * val)
   : rows(_rows),
     cols(_cols),
     data(new T[_rows * _cols])
      {
         const ShapeItem len = rows * cols;
         loop(l, len)   init_number(data[l], val->get_ravel(l));
      }

   /// constructor for a result
   Tmatrix(ShapeItem _rows, ShapeItem _cols)
   : rows(_rows),
     cols(_cols),
     data(new T[_rows * _cols])
      {
         const ShapeItem len = rows * cols;
         FloatCell n(0);
         loop(l, len)   init_number(data[l], n);
      }

   /// destructor
   ~Tmatrix()   { delete data; }

   /// return the idx'th row
   Tvector<T> row(int r)
      { Assert1(r >= 0);   Assert1(r < rows);
        return Tvector<T>(cols, 1, data + r); }

   /// return the idx'th row
   const Tvector<T> row(int r) const
      { Assert1(r >= 0);   Assert1(r < rows);
        return Tvector<T>(cols, 0, data + r); }

   /// return the idx'th column
   Tvector<T> col(int c)
      { Assert1(c >= 0);   Assert1(c < cols);
        return Tvector<T>(rows, rows, data + c * rows); }

   /// return the idx'th column
   const Tvector<T> col(int c) const
      { Assert1(c >= 0);   Assert1(c < cols);
        return Tvector<T>(rows, rows, data + c * rows); }

   /// divide
   void divide(Tvector<T> & ZZ, const Tvector<T> & AA) const;
protected:
   /// number of matrix rows
   const ShapeItem rows;

   /// number of matrix columns
   const ShapeItem cols;

   /// matrix elements in row-order
   T * data;
};
//-----------------------------------------------------------------------------
Value_P
divide_matrix(ShapeItem rows, ShapeItem cols_A, Value_P A, ShapeItem cols_B,
              Value_P B, const Shape & shape_Z, APL_Float qct)
{
const bool need_complex = A->is_complex(qct) || B->is_complex(qct);

Value_P Z(new Value(shape_Z, LOC));

   if (need_complex)
      {
        const Tmatrix<APL_Complex> TA(rows, cols_A, A);
        const Tmatrix<APL_Complex> TB(rows, cols_B, B);
        Tmatrix<APL_Complex> TZ(cols_B, cols_A);
        loop(cA, cols_A)
           {
             const Tvector<APL_Complex> VA(TA.col(cA));
             Tvector<APL_Complex> VZ(TZ.col(cA));
             TB.divide(VZ, VA);
           }
      }
   else
      {
        const Tmatrix<APL_Float> TA(rows, cols_A, A);
        const Tmatrix<APL_Float> TB(rows, cols_B, B);
        Tmatrix<APL_Float> TZ(cols_B, cols_A);
        loop(cA, cols_A)
           {
             const Tvector<APL_Float> VA(TA.col(cA));
             Tvector<APL_Float> VZ(TZ.col(cA));
             TB.divide(VZ, VA);
           }
      }


   DOMAIN_ERROR;
   return Z;
}
//-----------------------------------------------------------------------------
template<class T>
void
Tmatrix<T>::divide(Tvector<T> & ZZ, const Tvector<T> & AA) const
{
   CERR << "dividing " << rows << "x" << cols << " matrix by "
        << AA.get_len() << "-vector. Result is " << ZZ.get_len()
        << "-vector" << endl;
}
//-----------------------------------------------------------------------------

#endif // divide_matrix() without liblapack

#else    // not HAVE_LIBLAPACK

#warning liblapack not found or not installed. ⌹ will not work.

Value_P
divide_matrix(ShapeItem rows, ShapeItem cols_A, Value_P A, ShapeItem cols_B,
              Value_P B, const Shape & shape_Z, APL_Float qct)
{
   CERR <<
"function divide_matrix() aka. ⌹  returns DOMAIN ERROR because\n"
"library liblapack was not found when GNU APL was compiled. To fix this\n" 
"install liblapack from http://www.netlib.org/lapack/ and re-run\n"
"./configure, make, and make install." << endl;

   DOMAIN_ERROR;
}

#endif   // HAVE_LIBLAPACK

