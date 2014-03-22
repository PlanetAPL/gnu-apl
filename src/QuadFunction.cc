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

#include <unistd.h>

#include "ArrayIterator.hh"
#include "Avec.hh"
#include "CDR.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IndexIterator.hh"
#include "Input.hh"
#include "IntCell.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "PrintOperator.hh"
#include "QuadFunction.hh"
#include "Tokenizer.hh"
#include "UserFunction.hh"
#include "Value.hh"
#include "Workspace.hh"

extern char **environ;

Quad_AF  Quad_AF::fun;
Quad_AT  Quad_AT::fun;
Quad_CR  Quad_CR::fun;
Quad_DL  Quad_DL::fun;
Quad_EA  Quad_EA::fun;
Quad_EC  Quad_EC::fun;
Quad_ENV Quad_ENV::fun;
Quad_ES  Quad_ES::fun;
Quad_EX  Quad_EX::fun;
Quad_INP Quad_INP::fun;
Quad_NA  Quad_NA::fun;
Quad_NC  Quad_NC::fun;
Quad_NL  Quad_NL::fun;
Quad_SI  Quad_SI::fun;
Quad_UCS Quad_UCS::fun;

//=============================================================================
Token
Quad_AF::eval_B(Value_P B)
{
const ShapeItem ec = B->element_count();
Value_P Z(new Value(B->get_shape(), LOC));

   loop(v, ec)
       {
         const Cell & cell_B = B->get_ravel(v);

         if (cell_B.is_character_cell())   // Unicode to AV index
            {
              const Unicode uni = cell_B.get_char_value();
              int32_t pos = Avec::find_av_pos(uni);
              if (pos < 0)   new (&Z->get_ravel(v)) IntCell(MAX_AV);
              else           new (&Z->get_ravel(v)) IntCell(pos);
              continue;
            }

         if (cell_B.is_integer_cell())
            {
              const APL_Integer idx = cell_B.get_int_value();
              new (&Z->get_ravel(v))   CharCell(Quad_AV::indexed_at(idx));
              continue;
            }

         DOMAIN_ERROR;
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_AT::eval_AB(Value_P A, Value_P B)
{
   // A should be an integer skalar 1, 2, 3, or 4
   //
   if (A->get_rank() > 0)   RANK_ERROR;
const APL_Integer mode = A->get_ravel(0).get_int_value();
   if (mode < 1)   DOMAIN_ERROR;
   if (mode > 4)   DOMAIN_ERROR;

const ShapeItem cols = B->get_cols();
const ShapeItem rows = B->get_rows();
   if (rows == 0)   LENGTH_ERROR;

const int mode_vec[] = { 3, 7, 4, 2 };
const int mode_len = mode_vec[mode - 1];
Shape shape_Z(rows);
   shape_Z.add_shape_item(mode_len);

Value_P Z(new Value(shape_Z, LOC));

   loop(r, rows)
      {
        // get the symbol name by stripping trailing spaces
        //
        const ShapeItem b = r*cols;   // start of the symbol name
        UCS_string symbol_name;
        loop(c, cols)
           {
            const Unicode uni = B->get_ravel(b + c).get_char_value();
            if (uni == UNI_ASCII_SPACE)   break;
            symbol_name.append(uni);
           }

        NamedObject * obj = Workspace::lookup_existing_name(symbol_name);
        if (obj == 0)   throw_symbol_error(symbol_name, LOC);

        const Function * function = obj->get_function();
        if (function)             // user defined or system function.
           {
             function->get_attributes(mode, &Z->get_ravel(r*mode_len));
             continue;
           }

        Symbol * symbol = obj->get_symbol();
        if (symbol)               // user defined or system var.
           {
             symbol->get_attributes(mode, &Z->get_ravel(r*mode_len));
             continue;
           }

        // neither function nor variable (e.g. unused name)
        VALUE_ERROR;

        if (Avec::is_quad(symbol_name[0]))   // system function or variable
           {
             int l;
             const Token t(Workspace::get_quad(symbol_name, l));
             if (t.get_Class() == TC_SYMBOL)   // system variable
                {
                  Symbol * symbol = t.get_sym_ptr();
                }
             else
                {
                  Function * fun = t.get_function();
                }
           }
        else                                   // user defined
           {
             Symbol * symbol = Workspace::lookup_existing_symbol(symbol_name);
             if (symbol == 0)   VALUE_ERROR;

             symbol->get_attributes(mode, &Z->get_ravel(r*mode_len));
           }
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_CR::eval_B(Value_P B)
{
UCS_string symbol_name(*B.get());

   // remove trailing whitespaces in B
   //
   while (symbol_name.back() <= ' ')   symbol_name.pop_back();

   /*  return an empty character matrix,     if:
    *  1) symbol_name is not known,          or
    *  2) symbol_name is not user defined,   or
    *  3) symbol_name is a function,         or
    *  4) symbol_name is not displayable
    */
NamedObject * obj = Workspace::lookup_existing_name(symbol_name);
   if (obj == 0)   return Token(TOK_APL_VALUE1, Value::Str0_0_P);
   if (!obj->is_user_defined())   return Token(TOK_APL_VALUE1, Value::Str0_0_P);

const Function * function = obj->get_function();
   if (function == 0)   return Token(TOK_APL_VALUE1, Value::Str0_0_P);

   if (function->get_exec_properties()[0] != 0)
      return Token(TOK_APL_VALUE1, Value::Str0_0_P);

   // show the function...
   //
const UCS_string ucs = function->canonical(false);
vector<UCS_string> tlines;
   ucs.to_vector(tlines);
int max_len = 0;
   loop(row, tlines.size())
      {
        if (max_len < tlines[row].size())   max_len = tlines[row].size();
      }

Shape shape_Z;
   shape_Z.add_shape_item(tlines.size());
   shape_Z.add_shape_item(max_len);
   
Value_P Z(new Value(shape_Z, LOC));
   loop(row, tlines.size())
      {
        const UCS_string & line = tlines[row];
        loop(col, line.size())
            new (Z->next_ravel()) CharCell(line[col]);

        loop(col, max_len - line.size())
            new (Z->next_ravel()) CharCell(UNI_ASCII_SPACE);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Quad_CR::eval_AB(Value_P A, Value_P B)
{
   if (A->get_rank() > 1)                    RANK_ERROR;
   if (!A->is_skalar_or_len1_vector())       LENGTH_ERROR;
   if (!A->get_ravel(0).is_integer_cell())   DOMAIN_ERROR;

Value_P Z = do_CR(A->get_ravel(0).get_int_value(), *B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR(APL_Integer a, const Value & B)
{
PrintStyle style;

   switch(a)
      {
        case  0:  style = PR_APL;              break;
        case  1:  style = PR_APL_FUN;          break;
        case  2:  style = PR_BOXED_CHAR;       break;
        case  3:  style = PR_BOXED_GRAPHIC;    break;
        case  4:
        case  8: { // like 3/7, but enclose B so that the entire B is boxed
                   Value_P B1(new Value(LOC));
                   new (&B1->get_ravel(0))   PointerCell(B.clone(LOC));
                   Value_P Z = do_CR(a - 1, *B1.get());
                   return Z;
                 }
                 break;
        case  5: style = PR_HEX;              break;
        case  6: style = PR_hex;              break;
        case  7: style = PR_BOXED_GRAPHIC1;   break;
        case  9: style = PR_BOXED_GRAPHIC2;   break;
        case 10: {
                   vector<UCS_string> ucs_vec;
                   do_CR10(ucs_vec, B);

                   Value_P Z(new Value(ucs_vec.size(), LOC));
                   loop(line, ucs_vec.size())
                      {
                        Value_P Z_line(new Value(ucs_vec[line], LOC));
                        new (Z->next_ravel())   PointerCell(Z_line);
                      }
                   Z->set_default(*Value::Str0_0_P);
                   Z->check_value(LOC);
                   return Z;
                 }

        default: DOMAIN_ERROR;
      }

const PrintContext pctx(style, Workspace::get_PP(),
                        Workspace::get_CT(), Workspace::get_PW());
PrintBuffer pb(B, pctx);
   Assert(pb.is_rectangular());

const ShapeItem height = pb.get_height();
const ShapeItem width = pb.get_width(0);

const Shape sh(height, width);
Value_P Z(new Value(sh, LOC));

   // prototype
   //
   Z->get_ravel(0).init(CharCell(UNI_ASCII_SPACE));

   loop(y, height)
      loop(x, width)
         Z->get_ravel(x + y*width).init(CharCell(pb.get_char(x, y)));

   return Z;
}
//-----------------------------------------------------------------------------
void
Quad_CR::do_CR10(vector<UCS_string> & result, const Value & B)
{
const UCS_string obj_name(B);
const NamedObject * named_obj = Workspace::lookup_existing_name(obj_name);
   if (named_obj == 0)   DOMAIN_ERROR;

   switch(named_obj->get_nc())
      {
        case NC_VARIABLE:
             {
               const  Symbol * symbol = named_obj->get_symbol();
               Assert(symbol);
               const Value & value = *symbol->get_apl_value().get();
               const UCS_string pick;
               do_CR10_var(result, obj_name, pick, value);
               return;
             }

        case NC_FUNCTION:
        case NC_OPERATOR:
             TODO;

        default: DOMAIN_ERROR;
      }

}
//-----------------------------------------------------------------------------
void
Quad_CR::do_CR10_var(vector<UCS_string> & result, const UCS_string & var_name,
                 const UCS_string & pick, const Value & value)
{
   // recursively encode var_name with value.
   //
   // we emit one of 2 formats:
   //
   // short format: var_name ← (⍴ value) ⍴ value             " shortlog "
   //         
   // long format:  var_name ← (⍴ value) ⍴ value             " prolog "
   //               var_name[] ← partial value
   //               var_name ← (⍴ var_name) ⍴ var_name       " epilog "
   //         
   // short format (the default) requires a short and not nested value
   //
bool short_format = true;   // if false then prolog has been mitted.
bool nested = false;

UCS_string name(var_name);
   if (pick.size())
      {
         name.clear();
         name.append_utf8("((⎕IO+");
         name.append(pick);
         name.append_utf8(")⊃");
         name.append(var_name);
         name.append_utf8(")");
      }

   // compute the prolog
   //
UCS_string prolog(name);
   {
     prolog.append_utf8("←");
     if (pick.size())   prolog.append_utf8("⊂");
     prolog.append_number(value.element_count());
     prolog.append_utf8("⍴0");
   }

UCS_string shape_rho;
   {
     if (!value.is_skalar())
        {
          loop(r, value.get_rank())
              {
                if (r)   shape_rho.append_utf8(" ");
                shape_rho.append_number(value.get_shape_item(r));
              }

           shape_rho.append_utf8("⍴");
         }
   }

UCS_string epilog(name);
   {
     epilog.append_utf8("←");
     epilog.append(shape_rho);
     epilog.append(name);
   }

   enum V_mode
      {
        Vm_NONE,
        Vm_NUM,
        Vm_QUOT,
        Vm_UCS
      };

   if (value.element_count() == 0)   // empty value
      {
        Value_P proto = value.prototype(LOC);
        do_CR10_var(result, var_name, pick, *proto);
        UCS_string reshape(name);
        reshape.append_utf8("←");
        reshape.append(shape_rho);
        reshape.append(name);

        result.push_back(reshape);
        return;
      }

   // step 1: plain numbers and characters
   //
   {
     ShapeItem pos = 0;
     V_mode mode = Vm_NONE;
     enum { max_len = 78 };
     UCS_string line;
     int count = 0;
     int lpos = 0;
     loop(p, value.element_count())
        {
          // compute next
          //
          UCS_string next;
          V_mode next_mode;
          const Cell & cell = value.get_ravel(p);
          if (cell.is_character_cell())
             {
                next_mode = Vm_UCS;
                next.append_number(cell.get_char_value());
             }
          else if (cell.is_integer_cell())
             {
               next_mode = Vm_NUM;
               next.append_number(cell.get_int_value());
             }
          else if (cell.is_float_cell())
             {
               next_mode = Vm_NUM;
               bool scaled = false;
               PrintContext pctx(PR_APL_MIN, MAX_QUAD_PP, 0.0, MAX_QUAD_PW);
               next = UCS_string(cell.get_real_value(), scaled, pctx);
             }
          else if (cell.is_complex_cell())
             {
               next_mode = Vm_NUM;
               bool scaled = false;
               PrintContext pctx(PR_APL_MIN, MAX_QUAD_PP, 0.0, MAX_QUAD_PW);
               next = UCS_string(cell.get_real_value(), scaled, pctx);
               next.append_utf8("J");
               scaled = false;
               next.append(UCS_string(cell.get_imag_value(), scaled, pctx));
             }
          else if (cell.is_pointer_cell())
             {
               // adapt to current mode
               //
               next_mode = mode;
               if      (mode == Vm_NONE)   next.append_utf8("0");
               else if (mode == Vm_NUM)    next.append_utf8("0");
               else if (mode == Vm_QUOT)   next.append_utf8("!");
               else if (mode == Vm_UCS)    next.append_utf8("33");

               nested = true;
               if (short_format)
                  {
                    result.push_back(prolog);
                    short_format = false;
                  }
             }
          else DOMAIN_ERROR;

           if (count && (line.size() + next.size()) > max_len)   // long value
              {
                 if (mode == Vm_QUOT)       line.append_utf8("'");
                 else if (mode == Vm_UCS)   line.append_utf8(")");

                  if (short_format)
                     {
                       result.push_back(prolog);
                       short_format = false;
                     }

                 UCS_string pref = name;
                 pref.append_utf8("[");
                 pref.append_number(pos);
                 pref.append_utf8("+⍳");
                 pref.append_number(count);
                 pref.append_utf8("]←");
                 pref.append(line);
                 pos += count;
                 count = 0;

                 result.push_back(pref);
                 line.clear();
                 mode = Vm_NONE;
              }

          const bool mode_changed = (mode != next_mode);
          if (mode_changed)   // close old mode
             {
               if      (mode == Vm_QUOT)   line.append_utf8("'");
               else if (mode == Vm_UCS)    line.append_utf8(")");
             }

          if (mode_changed)   // open new mode
             {
               if (mode != Vm_NONE)   line.append_utf8(",");

               if      (next_mode == Vm_UCS)    line.append_utf8("(⎕UCS ");
               else if (next_mode == Vm_QUOT)   line.append_utf8("'");
               else if (next_mode == Vm_NUM)    line.append_utf8("");
             }
          else                // sepatator in same mode
             {
               if      (next_mode == Vm_QUOT)   ;
               else if (next_mode == Vm_UCS)    line.append_utf8(",");
               else if (next_mode == Vm_NUM)    line.append_utf8(",");
             }

          mode = next_mode;
          line.append(next);
          ++count;
          next.clear();
        }

     // all items done
     //
     if (mode == Vm_QUOT)       line.append_utf8("'");
     else if (mode == Vm_UCS)   line.append_utf8(")");

     if (short_format)
        {
          UCS_string pref = name;
          pref.append_utf8("←");
          pref.append(shape_rho);
          pref.append(line);

          result.push_back(pref);
        }
     else
        {
          UCS_string pref = name;
          pref.append_utf8("[");
          pref.append_number(pos);
          pref.append_utf8("+⍳");
          pref.append_number(count);
          pref.append_utf8("]←");
          pref.append(line);

          result.push_back(pref);
        }
   }

   // step 2: nested items
   //
   if (nested)
      {
        loop(p, value.element_count())
           {
             const Cell & cell = value.get_ravel(p);
             if (!cell.is_pointer_cell())   continue;

             UCS_string sub_pick = pick;
             if (pick.size())   sub_pick.append_utf8(" ");
             sub_pick.append_number(p);

             Value_P sub_value = cell.get_pointer_value();
             do_CR10_var(result, var_name, sub_pick, *sub_value.get());
           }
      }

   // step 3: reshape to final result
   //
   if (!short_format)   // may need epilog
      {
        // if value is a skalar or a vector with != 1 elements then
        // epilog is not needed.
        //
        if (value.get_rank() > 1 ||
            (value.is_vector() && value.element_count() == 1))

            result.push_back(epilog);
      }
}
//=============================================================================
Token
Quad_DL::eval_B(Value_P B)
{
const APL_time start = now();

   // B should be an integer or real skalar
   //
   if (B->get_rank() > 0)                 RANK_ERROR;
   if (!B->get_ravel(0).is_real_cell())   DOMAIN_ERROR;

const APL_time end = start + 1000000 * B->get_ravel(0).get_real_value();
   if (end < start)                            DOMAIN_ERROR;
   if (end > start + 31*24*60*60*1000000ULL)   DOMAIN_ERROR;   // > 1 month

   for (;;)
       {
         // compute time remaining.
         //
         const APL_time wait =  end - now();
         if (wait <= 0)   break;

         const int wait_sec  = wait/1000000;
         const int wait_usec = wait%1000000;
         timeval tv = { wait_sec, wait_usec };
         if (select(0, 0, 0, 0, &tv) == 0)   break;
       }

   // return time elapsed.
   //
Value_P Z(new Value(LOC));
   new (&Z->get_ravel(0)) FloatCell(0.000001*(now() - start));

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_EA::eval_AB(Value_P A, Value_P B)
{
ExecuteList * fun = 0;
const UCS_string statement_B(*B.get());

   try
      {
        fun = ExecuteList::fix(statement_B, LOC);
      }
   catch (...)
      {
      }

   if (fun == 0)   // ExecuteList::fix() failed
      {
        // syntax error in B: try to fix A...
        //
        try
           {
             const UCS_string statement_A(*A.get());
             fun = ExecuteList::fix(statement_A, LOC);
           }
       catch (...)
           {
           }

        if (fun == 0)   // syntax error in A and B: give up.
        //
        SYNTAX_ERROR;

        // A could be fixed: execute it.
        //
        Log(LOG_UserFunction__execute)   fun->print(CERR);

        Workspace::push_SI(fun, LOC);
        Workspace::SI_top()->set_safe_execution(true);

        // install end of context handler. The handler will do nothing when
        // B succeeds, but will execute A if not.
        //
        StateIndicator * si = Workspace::SI_top();

        si->set_eoc_handler(eoc_A_done);
        si->get_eoc_arg().A = A;
        si->get_eoc_arg().B = B;

        return Token(TOK_SI_PUSHED);
      }

   Assert(fun);

   Log(LOG_UserFunction__execute)   fun->print(CERR);

   Workspace::push_SI(fun, LOC);
   Workspace::SI_top()->set_safe_execution(true);

   // install end of context handler. The handler will do nothing when
   // B succeeds, but will execute A if not.
   //
StateIndicator * si = Workspace::SI_top();
   si->set_eoc_handler(eoc_B_done);
   si->get_eoc_arg().A = A;
   si->get_eoc_arg().B = B;

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
bool
Quad_EA::eoc_B_done(Token & token, EOC_arg & arg)
{
StateIndicator * si = Workspace::SI_top();

   // in A ⎕EA B, ⍎B was executed and may or may not have failed.
   //
   si->set_safe_execution(false);

   if (token.get_tag() != TOK_ERROR)   // ⍎B succeeded
      {
        // do not clear ⎕EM and ⎕ET; they should remain visible.
        return false;   // ⍎B successful.
      }

   // in A ⎕EA B, ⍎B has failed.
   //
   si->set_safe_execution(true);

Value_P A = arg.A;
Value_P B = arg.B;
const UCS_string statement_A(*A.get());

   // ⍎A has failed. ⎕EM shows only B but should show A ⎕EA B instead.
   //
   update_EM(A, B, false);

ExecuteList * fun = 0;
   try
      {
         fun = ExecuteList::fix(statement_A, LOC);
      }
   catch (...)
      {
      }
        
   // give up if if syntax error in A (and B has failed before)
   //
   if (fun == 0)   SYNTAX_ERROR;

   // A could be fixed: execute it.
   //
   Assert(fun);

   Log(LOG_UserFunction__execute)   fun->print(CERR);

   Workspace::pop_SI(LOC);
   Workspace::push_SI(fun, LOC);
   Workspace::SI_top()->set_safe_execution(true);

   // install end of context handler for the result of ⍎A. 
   //
   {
     StateIndicator * si1 = Workspace::SI_top();
     si1->set_eoc_handler(eoc_A_done);
     si1->get_eoc_arg().A = A;
     si1->get_eoc_arg().B = B;
   }

   return true;
}
//-----------------------------------------------------------------------------
bool
Quad_EA::eoc_A_done(Token & token, EOC_arg & arg)
{
   // in A ⎕EA B, ⍎B has failed, and ⍎A was executed and may or
   // may not have failed.
   //
   Workspace::SI_top()->set_safe_execution(false);

   if (token.get_tag() != TOK_ERROR)   // ⍎B succeeded
      {
        return false;   // ⍎B successful.
      }

Value_P A = arg.A;
Value_P B = arg.B;
const UCS_string statement_A(*A.get());

   // here both ⍎B and ⍎A failed. ⎕EM shows only B, but should show
   // A ⎕EA B instead.
   //
   update_EM(A, B, true);

   // display of errors was disabled since ⎕EM was incorrect.
   // now we have fixed ⎕EM and can display it.
   //
StateIndicator * si = Workspace::SI_top();
   COUT << si->get_error().get_error_line_1() << endl
        << si->get_error().get_error_line_2() << endl
        << si->get_error().get_error_line_3() << endl;

   return false;
}
//-----------------------------------------------------------------------------
void
Quad_EA::update_EM(Value_P A, Value_P B, bool A_failed)
{
UCS_string ucs_2(6, UNI_ASCII_SPACE);
int left_caret = 6;
int right_caret;

const UCS_string statement_A(*A.get());
const UCS_string statement_B(*B.get());

   if (A_failed)
      {
        right_caret = left_caret + statement_A.size() + 3;
      }
   else   // B failed
      { //            '                        '       ⎕EA '
        left_caret += statement_A.size() + 3;
        right_caret = left_caret + statement_B.size() + 5;
      }

   ucs_2.append(UNI_SINGLE_QUOTE);
   ucs_2.append(statement_A);
   ucs_2.append(UNI_SINGLE_QUOTE);

   ucs_2.append(UNI_ASCII_SPACE);

   ucs_2.append(UNI_QUAD_QUAD);
   ucs_2.append(UNI_ASCII_E);
   ucs_2.append(UNI_ASCII_A);

   ucs_2.append(UNI_ASCII_SPACE);

   ucs_2.append(UNI_SINGLE_QUOTE);
   ucs_2.append(statement_B);
   ucs_2.append(UNI_SINGLE_QUOTE);

StateIndicator * si = Workspace::SI_top();
   si->get_error() .set_error_line_2(ucs_2, left_caret, right_caret);

   Workspace::update_EM_ET(si->get_error());  // ⎕EM and ⎕ET
}
//=============================================================================
Token
Quad_EC::eval_B(Value_P B)
{
const UCS_string statement_B(*B.get());

ExecuteList * fun = 0;

   try
      {
        fun = ExecuteList::fix(statement_B, LOC);
      }
   catch (...)
      {
      }

   if (fun == 0)
      {
        // syntax error in B
        //
        Value_P Z(new Value(3, LOC));
        Value_P Z1(new Value(2, LOC));
        Value_P Z2(new Value(Error::error_name(E_SYNTAX_ERROR), LOC));
        new (&Z1->get_ravel(0))   IntCell(Error::error_major(E_SYNTAX_ERROR));
        new (&Z1->get_ravel(1))   IntCell(Error::error_minor(E_SYNTAX_ERROR));

        new (&Z->get_ravel(0)) IntCell(0);        // return code = error
        new (&Z->get_ravel(1)) PointerCell(Z1);   // ⎕ET value
        new (&Z->get_ravel(2)) PointerCell(Z2);   // ⎕EM

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   Assert(fun);

   Log(LOG_UserFunction__execute)   fun->print(CERR);

   Workspace::push_SI(fun, LOC);
   Workspace::SI_top()->set_safe_execution(true);

   // install end of context handler.
   // The handler will create the result after executing B.
   //
   Workspace::SI_top()->set_eoc_handler(eoc);
   // no EOC handler arguments

   return Token(TOK_SI_PUSHED);
}

//-----------------------------------------------------------------------------
bool
Quad_EC::eoc(Token & result_B, EOC_arg &)
{
   Workspace::SI_top()->set_safe_execution(false);

Value_P Z(new Value(3, LOC));
Value_P Z2;

int result_type = 0;
int et0 = 0;
int et1 = 0;

   switch(result_B.get_tag())
      {
       case TOK_ERROR:
            result_type = 0;
             Z2 = Value_P(new Value(Error::error_name(
                            ErrorCode(result_B.get_int_val())), LOC));
             break;

        case TOK_APL_VALUE1:
        case TOK_APL_VALUE3:
             result_type = 1;
             Z2 = result_B.get_apl_val();
             break;

        case TOK_APL_VALUE2:
             result_type = 2;
             Z2 = result_B.get_apl_val();
             break;

        case TOK_NO_VALUE:
             result_type = 3;
             Z2 = Value::Idx0_P;  // 0⍴0
             break;

        case TOK_BRANCH:
             result_type = 4;
             Z2 = Value_P(new Value(LOC));
             new (&Z2->get_ravel(0))   IntCell(result_B.get_int_val());
             break;

        case TOK_ESCAPE:
             result_type = 5;
             Z2 = Value::Idx0_P;  // 0⍴0
             break;

        default: CERR << "unexpected result tag " << result_B.get_tag()
                      << " in " << __FUNCTION__ << endl;
                 Assert(0);
      }

Value_P Z1(new Value(2, LOC));
   new (&Z1->get_ravel(0)) IntCell(et0);
   new (&Z1->get_ravel(1)) IntCell(et1);

   new (&Z->get_ravel(0)) IntCell(result_type);
   new (&Z->get_ravel(1)) PointerCell(Z1);
   new (&Z->get_ravel(2)) PointerCell(Z2);

   Z->check_value(LOC);
   move_2(result_B, Token(TOK_APL_VALUE1, Z), LOC);

   return false;
}
//=============================================================================
Token
Quad_ENV::eval_B(Value_P B)
{
   if (!B->is_char_string())   DOMAIN_ERROR;

const ShapeItem ec_B = B->element_count();

vector<const char *> evars;

   for (char **e = environ; *e; ++e)
       {
         const char * env = *e;

         // check if env starts with B.
         //
         bool match = true;
         loop(b, ec_B)
            {
              if (B->get_ravel(b).get_char_value() != Unicode(env[b]))
                 {
                   match = false;
                   break;
                 }
            }

         if (match)   evars.push_back(env);
       }

const Shape sh_Z(evars.size(), 2);
Value_P Z(new Value(sh_Z, LOC));

   loop(e, evars.size())
      {
        const char * env = evars[e];
        UCS_string ucs;
        while (*env)
           {
             if (*env != '=')   ucs.append(Unicode(*env++));
             else               break;
           }
        ++env;   // skip '='

        Value_P varname(new Value(ucs, LOC));

        ucs.clear();
        while (*env)   ucs.append(Unicode(*env++));

        Value_P varval(new Value(ucs, LOC));

        new (Z->next_ravel()) PointerCell(varname);
        new (Z->next_ravel()) PointerCell(varval);
      }

   Z->set_default(*Value::Spc_P);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_ES::eval_AB(Value_P A, Value_P B)
{
const UCS_string ucs(*A.get());
Error error(E_NO_ERROR, LOC);
const Token ret = event_simulate(&ucs, B, error);
   if (error.error_code == E_NO_ERROR)   return ret;

   throw error;
}
//-----------------------------------------------------------------------------
Token
Quad_ES::eval_B(Value_P B)
{
Error error(E_NO_ERROR, LOC);
const Token ret = event_simulate(0, B, error);
   if (error.error_code == E_NO_ERROR)   return ret;

   throw error;
}
//-----------------------------------------------------------------------------
Token
Quad_ES::event_simulate(const UCS_string * A, Value_P B, Error & error)
{
   // B is empty: no action
   //
   if (B->element_count() == 0)   return Token(TOK_APL_VALUE1, Value::Str0_0_P);

const bool int_B = B->get_ravel(0).is_integer_cell();
   error.init(get_error_code(B), error.throw_loc);

   if (error.error_code == E_NO_ERROR)   // B = 0 0: reset ⎕ET and ⎕EM.
      {
        Workspace::clear_error(LOC);
        return Token(TOK_APL_VALUE1, Value::Str0_0_P);
      }

   // at this point we shall throw the error. Add some error details.

   // set up error message 1
   //
   if (A)                                 // A ⎕ES B
      error.error_message_1 = *A;
   else if (error.error_code == E_USER_DEFINED_ERROR)   // ⎕ES with character B
      error.error_message_1 = UCS_string(*B.get());
   else if (error.is_known())             //  ⎕ES B with known major/minor B
      /* error_message_1 already OK */ ;
   else                                   //  ⎕ES B with unknown major/minor B
      error.error_message_1.clear();

   error.show_locked = true;

   if (Workspace::SI_top())
      Workspace::SI_top()->update_error_info(error);

   return Token();
}
//-----------------------------------------------------------------------------
ErrorCode
Quad_ES::get_error_code(Value_P B)
{
   // B shall be one of:                  → Error code
   //
   // 1. empty, or                        → No error
   // 2. a 2 element integer vector, or   → B[0]/N[1]
   // 3. a simple character vector.       → user defined
   //
   // Otherwise, throw an error

   if (B->get_rank() > 1)   RANK_ERROR;

   if (B->element_count() == 0)   return E_NO_ERROR;
   if (B->is_char_string())       return E_USER_DEFINED_ERROR;
   if (B->element_count() != 2)   LENGTH_ERROR;

const APL_Integer major = B->get_ravel(0).get_int_value();
const APL_Integer minor = B->get_ravel(1).get_int_value();

   return ErrorCode(major << 16 | minor & 0xFFFF);
}
//=============================================================================
Token
Quad_EX::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

Value_P Z(var_count > 1 ? new Value(var_count, LOC) : new Value(LOC));

   loop(z, var_count)   new (Z->next_ravel()) IntCell(expunge(vars[z]));

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
int
Quad_EX::expunge(UCS_string name)
{
   if (name.size() == 0)   return 0;

Symbol * symbol = Workspace::lookup_existing_symbol(name);
   if (symbol == 0)   return 0;
   return symbol->expunge();
}
//=============================================================================
Token
Quad_INP::eval_AB(Value_P A, Value_P B)
{
EOC_arg arg(B);
quad_INP & _arg = arg.u.u_quad_INP;
   _arg.lines = 0;

   // B is the end of document marker for a 'HERE document', similar
   // to ENDCAT in cat << ENDCAT.
   //
   // make sure that B is a non-empty string and extract it.
   //
   if (B->get_rank() > 1)         RANK_ERROR;
   if (B->element_count() == 0)   LENGTH_ERROR;

   _arg.end_marker = new UCS_string(B->get_UCS_ravel());

   // A is either one string (esc1 == esc2) or two (nested) strings
   // for esc1 and esc2 respectively.
   //
   if (A->compute_depth() == 2)   // two (nested) strings
      {
        if (A->get_rank() != 1)        RANK_ERROR;
        if (A->element_count() != 2)   LENGTH_ERROR;

        UCS_string ** esc[2] = { &_arg.esc1, &_arg.esc2 };
        loop(e, 2)
           {
             const Cell & cell = A->get_ravel(e);
             if (cell.is_pointer_cell())   // char vector
                {
                  *esc[e] =
                      new UCS_string(cell.get_pointer_value()->get_UCS_ravel());
                }
             else                          // char skalar
                {
                  *esc[e] = new UCS_string(1, cell.get_char_value());
                }
           }

        if (_arg.esc1->size() == 0)   LENGTH_ERROR;
      }
   else                       // one string 
      {
        _arg.esc1 = _arg.esc2 = new UCS_string(A->get_UCS_ravel());
      }

Token tok(TOK_FIRST_TIME);
   eoc_INP(tok, arg);
   return tok;
}
//-----------------------------------------------------------------------------
Token
Quad_INP::eval_B(Value_P B)
{
EOC_arg arg(B);
quad_INP & _arg = arg.u.u_quad_INP;
   _arg.lines = 0;

   // B is the end of document marker for a 'HERE document', similar
   // to ENDCAT in cat << ENDCAT.
   //
   // make sure that B is a non-empty string and extract it.
   //
   if (B->get_rank() > 1)   RANK_ERROR;
   if (B->element_count() == 0)   LENGTH_ERROR;

   _arg.end_marker = new UCS_string(B->get_UCS_ravel());

   _arg.esc1 = _arg.esc2 = 0;

Token tok(TOK_FIRST_TIME);
   eoc_INP(tok, arg);
   return tok;
}
//-----------------------------------------------------------------------------
bool
Quad_INP::eoc_INP(Token & token, EOC_arg & _arg)
{
   if (token.get_tag() == TOK_ERROR)
      {
         CERR << "Error in ⎕INP" << endl;
         UCS_string err("*** ");
         err.append(Error::error_name((ErrorCode)(token.get_int_val())));
         err.append_utf8(" in ⎕INP ***");
         Value_P val(new Value(err, LOC));
         move_2(token, Token(TOK_APL_VALUE1, val), LOC);
      }

   // _arg may be pop_SI()ed below so we need a copy of it
   //
quad_INP arg = _arg.u.u_quad_INP;

   if (token.get_tag() != TOK_FIRST_TIME)
      {
        // token is the result of ⍎ exec and arg.lines has at least 2 lines.
        // append the value in token and then the last line to the second
        // last line and pop the last line.
        //
        Value_P result = token.get_apl_val();

        UCS_string_list * last = arg.lines;
        Assert(last);                       // remember last
        arg.lines = arg.lines->prev;        // pop last from lines
        Assert(arg.lines);                  // must be 'before' below

        // if the result is a simple char string then insert it between
        // the last two lines. Otherwise insert a 2-dimensional ⍕result
        // indented by the length of arg.lines.string (== before below)
        // 
        if (result->is_char_string())
           {
             arg.lines->string.append(result->get_UCS_ravel());   // append result
           }
        else
           {
             Value_P matrix = Bif_F12_FORMAT::monadic_format(result);
             const Cell * cm = &matrix->get_ravel(0);
             const int plen = arg.lines->string.size();   // indent

             // append the first row of the matrix to 'before'
             //
             loop(r, matrix->get_cols())
                 arg.lines->string.append(cm++->get_char_value());

             // append subsequent rows of the matrix to a new line
             //
             loop(r, matrix->get_rows() - 1)
                {
                  UCS_string row(plen, UNI_ASCII_SPACE);
                  loop(r, matrix->get_cols())   row.append(cm++->get_char_value());
                  arg.lines = new UCS_string_list(row, arg.lines);
                }
           }

        arg.lines->string.append(last->string);              // append last line

        delete last;
      }

   for (;;)   // read lines until end reached.
       {
         UCS_string line = Input::get_line();
         arg.done = (line.substr_pos(*arg.end_marker) != -1);

         if (arg.esc1)   // start marker defined (dyadic ⎕INP)
            {
              UCS_string exec;     // line esc1 ... esc2 (excluding)

              const int e1_pos = line.substr_pos(*arg.esc1);
              if (e1_pos != -1)   // and the start marker is present.
                 {
                   // push characters left of exec
                   //
                   {
                     const UCS_string before(line, 0, e1_pos);
                     arg.lines = new UCS_string_list(before, arg.lines);
                   }

                   exec = line.drop(e1_pos + arg.esc1->size());
                   exec.remove_lt_spaces();   // no leading and trailing spaces
                   line.clear();

                   if (arg.esc2 == 0)
                      {
                        // empty esc2 means exec is the rest of the line.
                        // nothing to do.
                      }
                   else
                      {
                        // non-empty esc2: search for it
                        //
                        const int e2_pos = exec.substr_pos(*arg.esc2);
                        if (e2_pos == -1)   // esc2 not found: exec rest of line
                           {
                             // esc2 not in exec: take rest of line
                             // i.e. nothing to do.
                           }
                        else
                           {
                             const int rest = e2_pos + arg.esc2->size();
                             line = UCS_string(exec, rest, exec.size() - rest);
                             exec.shrink(e2_pos);
                           }
                      }
                 }

              if (exec.size())
                 {
                   // if SI is already pushed (i.e. this is a subsequent
                   // call to eoc_INP() then pop it.
                   //
                   if (token.get_tag() != TOK_FIRST_TIME)
                      Workspace::pop_SI(LOC);

                   arg.lines = new UCS_string_list(line, arg.lines);

                   move_2(token, Bif_F1_EXECUTE::execute_statement(exec), LOC);
                   Assert(token.get_tag() == TOK_SI_PUSHED);

                   StateIndicator * si = Workspace::SI_top();
                   si->set_eoc_handler(eoc_INP);
                   si->get_eoc_arg().u.u_quad_INP = arg;
                   return true;   // continue
                 }
            }

         if (arg.done)   break;

         arg.lines = new UCS_string_list(line, arg.lines);
       }

const ShapeItem zlen = UCS_string_list::length(arg.lines);
Value_P Z(new Value(ShapeItem(zlen), LOC));
Cell * cZ = &Z->get_ravel(0) + zlen;

   loop(z, zlen)
      {
        UCS_string_list * node = arg.lines;
        arg.lines = arg.lines->prev;
        Value_P ZZ(new Value(node->string, LOC));
        ZZ->check_value(LOC);
        new (--cZ)   PointerCell(ZZ);
        delete node;
      }

   delete arg.end_marker;
   delete arg.esc1;
   if (arg.esc2 != arg.esc1)   delete arg.esc2;

   Z->set_default(*Value::zStr0_P);
   Z->check_value(LOC);
   move_2(token, Token(TOK_APL_VALUE1, Z), LOC);
   return false;   // continue
}
//=============================================================================
Token
Quad_NC::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

Value_P Z(var_count > 1 ? new Value(var_count, LOC) : new Value(LOC));

   loop(v, var_count)   new (Z->next_ravel())   IntCell(get_NC(vars[v]));

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
APL_Integer
Quad_NC::get_NC(const UCS_string ucs)
{
   if (ucs.size() == 0)   return -1;   // invalid name

   if (Avec::is_quad(ucs[0]))   // quad fun or var
      {
        int len = 0;
        const Token t = Workspace::get_quad(ucs, len);
        if (len < 2)                      return -1;   // invalid quad
        if (t.get_Class() == TC_SYMBOL)   return  2;   // quad variable
        else                              return  3;   // quad function

        // cases 0, 1, or 4 cannot happen for Quad symbols
      }

Symbol * symbol = Workspace::lookup_existing_symbol(ucs);

   if (!symbol)   return 0;   // not known

const NameClass nc = symbol->get_nc();

   // return nc, but map shared variables to regular variables
   //
   return nc == NC_SHARED_VAR ? NC_VARIABLE : nc;
}
//=============================================================================
Token
Quad_NL::do_quad_NL(Value_P A, Value_P B)
{
   if (!!A && !A->is_char_string())                        DOMAIN_ERROR;
   if (!B->is_int_vector(0.1) && !B->is_int_skalar(0.1))   DOMAIN_ERROR;

uint32_t symbol_count = Workspace::symbols_allocated();
Symbol * table[symbol_count];
   Workspace::get_all_symbols(table, symbol_count);

   // remove symbols that are erased or not in A (if provided). We remove
   // by overriding the symbol pointer to be erased by the last pointer in
   // the table and reducing the table size by 1.
   //
   loop(s, symbol_count)
      {
        Symbol * sym = table[s];
        bool do_erase = sym->is_erased();
        if (!!A)
           {
             const ShapeItem ec_A = A->element_count();
             const Unicode first_char = sym->get_name()[0];
             bool first_in_A = false;
             loop(a, ec_A)
                {
                  if (A->get_ravel(a).get_char_value() == first_char)
                     {
                       first_in_A = true;
                       break;
                     }
                }

             if (!first_in_A)   do_erase = true;
           }

        const NameClass nc = sym->get_nc();
        const ShapeItem ec_B = B->element_count();
        bool nc_in_B = false;

        loop(b, ec_B)
           {
             if (B->get_ravel(b).get_int_value() == nc)
                {
                  nc_in_B = true;
                  break;
                }
           }

        if (!nc_in_B)   do_erase = true;

        // we decrement s in order to retry it
        if (do_erase)   table[s--] = table[--symbol_count];
      }

   // compute length of longest name
   //
int longest = 0;
   loop(s, symbol_count)
      {
        if (longest < table[s]->get_name().size())   
           longest = table[s]->get_name().size();
      }

const Shape shZ(symbol_count, longest);
Value_P Z(new Value(shZ, LOC));

   // the number of symbols is small and ⎕NL is (or should) not be a perfomance
   // critical function, so we do a linear sort (find smallest and remove)
   // approach.
   //
   while (symbol_count)
      {
        // find smallest.
        //
        uint32_t smallest = symbol_count - 1;
        loop(t, symbol_count - 1)
           {
             if (table[smallest]->get_name().compare(table[t]->get_name()) > 0)
                smallest = t;
           }

        // copy name to result, padded with spaces.
        //
        loop(l, longest)
           {
             const UCS_string & ucs = table[smallest]->get_name();
             new (Z->next_ravel())   CharCell(l < ucs.size() ? ucs[l] : UNI_ASCII_SPACE);
           }

        // remove from table
        table[smallest] = table[--symbol_count];
      }

   Z->set_default(*Value::Str0_P.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_SI::eval_AB(Value_P A, Value_P B)
{
   if (A->element_count() != 1)   // not skalar-like
      {
        if (A->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }
APL_Integer a = A->get_ravel(0).get_int_value();
const ShapeItem len = Workspace::SI_entry_count();
   if (a >= len)   DOMAIN_ERROR;
   if (a < -len)   DOMAIN_ERROR;
   if (a < 0)   a += len;   // negative a counts backwards from end

const StateIndicator * si = 0;
   for (si = Workspace::SI_top(); si; si = si->get_parent())
       {
         if (si->get_level() == a)   break;   // found
       }

   Assert(si);

   if (B->element_count() != 1)   // not skalar-like
      {
        if (B->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }

const Function_PC PC = Function_PC(si->get_PC() - 1);
const Executable * exec = si->get_executable();
const ParseMode pm = exec->get_parse_mode();
const UCS_string & fun_name = exec->get_name();
const Function_Line fun_line = exec->get_line(PC);

Value_P Z;

const APL_Integer b = B->get_ravel(0).get_int_value();
   switch(b)
      {
        case 1:  Z = Value_P(new Value(fun_name, LOC));             break;
        case 2:  Z = Value_P(new Value(LOC));
                new (&Z->get_ravel(0)) IntCell(fun_line);                break;
        case 3:  {
                   UCS_string fun_and_line(fun_name);
                   fun_and_line.append(UNI_ASCII_L_BRACK);
                   fun_and_line.append_number(fun_line);
                   fun_and_line.append(UNI_ASCII_R_BRACK);
                   Z = Value_P(new Value(fun_and_line, LOC)); 
                 }
                 break;

        case 4:  if (si->get_error().error_code)
                    {
                      const UCS_string & text =
                                        si->get_error().get_error_line_2();
                      Z = Value_P(new Value(text, LOC));
                    }
                 else
                    {
                      const UCS_string text = exec->statement_text(PC);
                      Z = Value_P(new Value(text, LOC));
                    }
                 break;

        case 5: Z = Value_P(new Value(LOC));
                new (&Z->get_ravel(0)) IntCell(PC);                      break;

        case 6: Z = Value_P(new Value(LOC));
                new (&Z->get_ravel(0)) IntCell(pm);                      break;

        default: DOMAIN_ERROR;
      }

   Z->set_default(*Value::Zero_P);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Quad_SI::eval_B(Value_P B)
{
   if (B->element_count() != 1)   // not skalar-like
      {
        if (B->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }

const APL_Integer b = B->get_ravel(0).get_int_value();
const ShapeItem len = Workspace::SI_entry_count();

   if (b < 1)   DOMAIN_ERROR;
   if (b > 4)   DOMAIN_ERROR;

   // at this point we should not fail...
   //
Value_P Z(new Value(len, LOC));

ShapeItem z = 0;
   for (const StateIndicator * si = Workspace::SI_top();
        si; si = si->get_parent())
       {
         Cell * cZ = &Z->get_ravel(len - ++z);

         const Function_PC PC = Function_PC(si->get_PC() - 1);
         const Executable * exec = si->get_executable();
         const ParseMode pm = exec->get_parse_mode();
         const UCS_string & fun_name = exec->get_name();
         const Function_Line fun_line = exec->get_line(PC);

         switch(b)
           {
             case 1:  new (cZ) PointerCell(Value_P(new Value(fun_name, LOC)));   break;
             case 2:  new (cZ) IntCell(fun_line);                       break;
             case 3:  {
                        UCS_string fun_and_line(fun_name);
                        fun_and_line.append(UNI_ASCII_L_BRACK);
                        fun_and_line.append_number(fun_line);
                        fun_and_line.append(UNI_ASCII_R_BRACK);
                        new (cZ) PointerCell(Value_P(new Value(fun_and_line, LOC))); 
                      }
                      break;

             case 4:  if (si->get_error().error_code)
                         {
                           const UCS_string & text =
                                        si->get_error().get_error_line_2();
                           new (cZ) PointerCell(Value_P(new Value(text, LOC))); 
                         }
                      else
                         {
                           const UCS_string text = exec->statement_text(PC);
                           new (cZ) PointerCell(Value_P(new Value(text, LOC))); 
                         }
                      break;

             case 5:  new (cZ) IntCell(PC);                             break;
             case 6:  new (cZ) IntCell(pm);                             break;
           }

       }

   Z->set_default(*Value::Zero_P);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_UCS::eval_B(Value_P B)
{
const ShapeItem ec = B->element_count();
Value_P Z(new Value(B->get_shape(), LOC));

   loop(v, ec)
       {
         const Cell & cell_B = B->get_ravel(v);

         if (cell_B.is_character_cell())   // char to Unicode
            {
              const Unicode uni = cell_B.get_char_value();
              new (&Z->get_ravel(v)) IntCell(uni);
              continue;
            }

         if (cell_B.is_integer_cell())
            {
              const APL_Integer bint = cell_B.get_int_value();
              new (&Z->get_ravel(v))   CharCell(Unicode(bint));
              continue;
            }

         Q(cell_B.get_cell_type())
         DOMAIN_ERROR;
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================

