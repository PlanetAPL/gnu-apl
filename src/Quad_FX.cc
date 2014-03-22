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

#include <stdlib.h>
#include <dlfcn.h>

#include "IntCell.hh"
#include "NativeFunction.hh"
#include "Quad_FX.hh"
#include "Symbol.hh"
#include "UserFunction.hh"
#include "Workspace.hh"

Quad_FX  Quad_FX::fun;

//=============================================================================
Token
Quad_FX::eval_B(Value_P B)
{
static const int eprops[] = { 0, 0, 0, 0 };
   return do_quad_FX(eprops, B);
}
//-----------------------------------------------------------------------------
Token
Quad_FX::eval_AB(Value_P A, Value_P B)
{
   if (A->get_rank() > 1)         RANK_ERROR;

   if (A->is_char_string())   return do_native_FX(A, -1, B);

int eprops[4];
   if (A->element_count() == 1)
      {
        eprops[0] = A->get_ravel(0).get_int_value();
        if (eprops[0] < 0)   DOMAIN_ERROR;
        if (eprops[0] > 1)   DOMAIN_ERROR;
        eprops[3] = eprops[2] = eprops[1] = eprops[0];
      }
   if (A->element_count() == 4)
      {
         loop(e, 4)
            {
              eprops[e] = A->get_ravel(e).get_int_value();
              if (eprops[e] < 0)   DOMAIN_ERROR;
              if (eprops[e] > 1)   DOMAIN_ERROR;
            }
      }
   else    LENGTH_ERROR;

   return do_quad_FX(eprops, B);
}
//-----------------------------------------------------------------------------
Token
Quad_FX::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   if (!X->is_skalar_or_len1_vector())   AXIS_ERROR;
   if (A->get_rank() > 1)                RANK_ERROR;
   if (!A->is_char_string())             DOMAIN_ERROR;

const Axis axis = X->get_single_axis(10);

   return do_native_FX(A, axis, B);
}
//-----------------------------------------------------------------------------
Token
Quad_FX::do_quad_FX(const int * exec_props, Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;
   if (B->get_rank() < 1)   RANK_ERROR;

UCS_string text;

   // ⎕FX accepts two kinds of B argments:
   //
   // 1. A vector whose elements are the lines of the function, or
   // 2. A character matrix whose rows are the lines of the function.
   //
   // we convert each format into text, which is a UCS string with
   // lines separated by ASCII_LF.
   //
   if (B->get_rank() == 1)   // case 1: vector of simple character vectors
      {
        const ShapeItem ec = B->element_count();
        loop(e, ec)
           {
             const Cell & cell = B->get_ravel(e);
             if (cell.is_character_cell())   /// a line with a single char.
                {
                  text.append(cell.get_char_value());
                  text.append(UNI_ASCII_LF);
                  continue;
                }

             if (!cell.is_pointer_cell())   DOMAIN_ERROR;
             Value_P line = cell.get_pointer_value();
             Assert(line);

             Log(LOG_quad_FX)
                {
                  CERR << "[" << setw(2) << e << "] " << *line << endl;
                }

             if (line->is_char_vector())
                {
                  const uint64_t line_len = line->element_count();
                  loop(l, line_len)
                     {
                       const Cell & c = line->get_ravel(l);
                       if (!c.is_character_cell())   DOMAIN_ERROR;
                       text.append(c.get_char_value());
                     }
                }
             else if (line->is_skalar())
                {
                  const Cell & c1 = line->get_ravel(0);
                  if (!c1.is_character_cell())   DOMAIN_ERROR;
                  text.append(c1.get_char_value());
                }
             else DOMAIN_ERROR;

             text.append(UNI_ASCII_LF);
           }
      }
   else                      // simple character matrix
      {
        const ShapeItem rows = B->get_rows();
        const ShapeItem cols = B->get_cols();
        const Cell * cB = &B->get_ravel(0);

        loop(row, rows)
           {
             loop(col, cols)   text.append(cB++->get_char_value());
             text.append(UNI_ASCII_LF);
           }
      }

   return do_quad_FX(exec_props, text);
}
//-----------------------------------------------------------------------------
Token
Quad_FX::do_quad_FX(const int * exec_props, const UCS_string & text)
{
int error_line = 0;
UserFunction * fun = UserFunction::fix(text, error_line, false, LOC);
   if (fun == 0)
      {
        Value_P Z(new Value(LOC));
        new (&Z->get_ravel(0))   IntCell(error_line + Workspace::get_IO());
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   fun->set_exec_properties(exec_props);

Symbol * sym_fun = fun->get_sym_FUN();
   Assert(sym_fun);

const UCS_string fun_name = sym_fun->get_name();
Value_P Z(new Value(fun_name, LOC));

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Quad_FX::do_native_FX(Value_P A, Axis axis, Value_P B)
{
UCS_string so_name = A->get_UCS_ravel();
UCS_string function_name = B->get_UCS_ravel();

NativeFunction * fun = NativeFunction::fix(so_name, function_name);
   if (fun == 0)
      {
        Value_P Z = Value::Zero_P;
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

Value_P Z = B;
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
