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

#include "CDR.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "PointerCell.hh"
#include "Quad_FX.hh"
#include "Quad_TF.hh"
#include "Symbol.hh"
#include "Tokenizer.hh"
#include "Workspace.hh"

Quad_TF  Quad_TF::fun;

//-----------------------------------------------------------------------------
Token
Quad_TF::eval_AB(Value_P A, Value_P B)
{
   // A should be an integer skalar or 1-element_vector with value 1 or 2
   //
   if (A->get_rank() > 0)         RANK_ERROR;
   if (A->element_count() != 1)   LENGTH_ERROR;

const APL_Integer mode = A->get_ravel(0).get_int_value();
const UCS_string symbol_name(*B.get());

Value_P Z;

   if (mode == 1)        Z = tf1(symbol_name);
   else if (mode == 2)   return tf2(symbol_name);
   else if (mode == 3)   Z = tf3(symbol_name);
   else                  DOMAIN_ERROR;

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Quad_TF::tf1(const UCS_string & name)
{
NamedObject * obj = Workspace::lookup_existing_name(name);
   if (obj == 0)   return Value::Str0_P;

const Function * function = obj->get_function();

   if (function)
      {
        if (obj->is_user_defined())   return tf1(name, *function);

        // quad function or primitive: return ''
        return Value::Str0_P;
      }

Symbol * symbol = obj->get_symbol();
   if (symbol)
      {
        Value_P value = symbol->get_apl_value();
        if (!!value)   return tf1(name, value);

        /* not a variable: fall through */
      }

   return Value::Str0_P;
}
//-----------------------------------------------------------------------------
Value_P
Quad_TF::tf1(const UCS_string & var_name, Value_P val)
{
   // check for inverse 1 ⎕TF and return it if applicable.
   //
   {
     Value_P Z = tf1_inv(val);
     if (!!Z)   return Z;
   }

const bool is_char_array = val->get_ravel(0).is_character_cell();
UCS_string ucs(is_char_array ? UNI_ASCII_C : UNI_ASCII_N);

   ucs.append(var_name);
   ucs.append(UNI_ASCII_SPACE);
   ucs.append_number(val->get_rank());   // rank

   loop(r, val->get_rank())
      {
        ucs.append(UNI_ASCII_SPACE);
        ucs.append_number(val->get_shape_item(r));   // shape
      }

const ShapeItem ec = val->element_count();

   if (is_char_array)
      {
        ucs.append(UNI_ASCII_SPACE);

        loop(e, ec)
           {
             const Cell & cell = val->get_ravel(e);
             if (!cell.is_character_cell())
                {
                  return  Value::Str0_P;
                }

             ucs.append(cell.get_char_value());
           }
      }
   else   // number
      {
        const APL_Float qct = Workspace::get_CT();
        loop(e, ec)
           {
             ucs.append(UNI_ASCII_SPACE);

             const Cell & cell = val->get_ravel(e);
             if (cell.is_integer_cell())
                {
                  ucs.append_number(cell.get_int_value());
                }
             else // if (cell.is_near_real(qct))
                {
                  PrintContext pctx(PR_APL_MIN, MAX_QUAD_PP, 0.0, MAX_QUAD_PW);
                  const APL_Float value = cell.get_real_value();
                  bool scaled = true;
                  UCS_string ucs1(value, scaled, pctx);
                  ucs.append(ucs1);
                }
           }
      }

   return Value_P(new Value(ucs, LOC));
}
//-----------------------------------------------------------------------------
Value_P
Quad_TF::tf1(const UCS_string & fun_name, const Function & fun)
{
const UCS_string text = fun.canonical(false);
vector<UCS_string> lines;
const size_t max_len = text.to_vector(lines);

UCS_string ucs;
   ucs.append(UNI_ASCII_F);
   ucs.append(fun_name);
   ucs.append(UNI_ASCII_SPACE);
   ucs.append_number(2);   // rank
   ucs.append(UNI_ASCII_SPACE);
   ucs.append_number(lines.size());   // rows
   ucs.append(UNI_ASCII_SPACE);
   ucs.append_number(max_len);        // cols
   ucs.append(UNI_ASCII_SPACE);

   loop(l, lines.size())
      {
       ucs.append(lines[l]);
       loop(c, max_len - lines[l].size())   ucs.append(UNI_ASCII_SPACE);
      }

   return Value_P(new Value(ucs, LOC));
}
//-----------------------------------------------------------------------------
Value_P
Quad_TF::tf1_inv(Value_P val)
{
   if (!val->is_char_vector())   return Value_P();

UCS_string ravel(*val.get());

const size_t len = ravel.size();
   if (len < 2)   return Value_P();

   // mode should be 'F', 'N', or 'C'.
const Unicode mode = ravel[0];
   if (mode != 'C' && mode != 'F' && mode != 'N')   return Value_P();

UCS_string name(ravel[1]);
   if (!Avec::is_quad(name[0]) && !Avec::is_first_symbol_char(name[0]))
      return Value_P();

ShapeItem idx = 2;
   while (idx < len && Avec::is_symbol_char(ravel[idx]))   name.append(ravel[idx++]);

   if (Avec::is_quad(name[0]) && name.size() == 1)   return Value_P();
   if (ravel[idx++] != UNI_ASCII_SPACE)   return Value_P();

ShapeItem idx0 = idx;
Rank rank = 0;
   while (idx < len && Avec::is_digit(ravel[idx]))
      rank = 10 * rank + ravel[idx++] - UNI_ASCII_0;

   if (rank == 0 && idx0 == idx)   return Value_P();   // no rank
   if (ravel[idx++] != UNI_ASCII_SPACE)   return Value_P();
   if (rank > MAX_RANK)    return Value_P();

Shape shape;
   loop(r, rank)
      {
        idx0 = idx;
        ShapeItem sh = 0;
        while (idx < len && Avec::is_digit(ravel[idx]))
           sh = 10 * sh + ravel[idx++] - UNI_ASCII_0;
        if (sh == 0 && idx0 == idx)   return Value_P();   // shape
        if (ravel[idx++]!= UNI_ASCII_SPACE)  return Value_P();
        shape.add_shape_item(sh);
      }

Symbol * symbol = 0;
NameClass nc = NC_UNUSED_USER_NAME;
NamedObject * sym_or_fun = Workspace::lookup_existing_name(name); 
   if (sym_or_fun)   // existing name
      {
        symbol = sym_or_fun->get_symbol();
        nc = sym_or_fun->get_nc();
        if (symbol && symbol->is_readonly())   return Value_P();
     }

const size_t data_chars = len - idx;

   if (mode == UNI_ASCII_F)   // function
      {
        if (rank != 2)   return Value_P();

        if (nc != NC_UNUSED_USER_NAME && nc != NC_FUNCTION && nc != NC_OPERATOR)
           return Value_P();

        if (data_chars != shape.get_volume())   return Value_P();

        Value_P new_val(new Value(shape, LOC));
        loop(d, data_chars)
           new (&new_val->get_ravel(d)) CharCell(ravel[idx + d]);
        new_val->check_value(LOC);

         Token t = Quad_FX::fun.eval_B(new_val);
      }
   else if (mode == UNI_ASCII_C)   // char array
      {
        if (data_chars != shape.get_volume())   return Value_P();
        if (nc != NC_UNUSED_USER_NAME && nc != NC_VARIABLE)   return Value_P();

        Value_P new_val(new Value(shape, LOC));
        loop(d, data_chars)
           new (&new_val->get_ravel(d)) CharCell(ravel[idx + d]);

        new_val->check_value(LOC);

        if (symbol == 0)
           symbol = Workspace::lookup_symbol(name);

        symbol->assign(new_val, LOC);
      }
   else if (mode == UNI_ASCII_N)   // numeric array
      {
        if (nc != NC_UNUSED_USER_NAME && nc != NC_VARIABLE)   return Value_P();

        UCS_string data(ravel, idx, len - idx);
        Tokenizer tokenizer(PM_EXECUTE, LOC);
        Token_string tos;
        if (tokenizer.tokenize(data, tos) != E_NO_ERROR)   return Value_P();
        if (tos.size() != shape.get_volume())           return Value_P();

        // check that all token are numeric...
        //
        loop(t, tos.size())
           {
             const TokenTag tag = tos[t].get_tag();
             if (tag == TOK_INTEGER)   continue;
             if (tag == TOK_REAL)      continue;
             if (tag == TOK_COMPLEX)   continue;
             return Value_P();
           }

        // at this point, we have a valid inverse 1 ⎕TF.
        //
        Value_P new_val(new Value(shape, LOC));

        loop(t, tos.size())
           {
             const TokenTag tag = tos[t].get_tag();
             if (tag == TOK_INTEGER)
                new (&new_val->get_ravel(t)) IntCell(tos[t].get_int_val());
             else if (tag == TOK_REAL)
                new (&new_val->get_ravel(t)) FloatCell(tos[t].get_flt_val());
             else if (tag == TOK_COMPLEX)
                new (&new_val->get_ravel(t)) ComplexCell(tos[t].get_cpx_real(),
                                                        tos[t].get_cpx_imag());
             else Assert(0);   // since checked above
           }

        new_val->check_value(LOC);

        if (!symbol)   symbol = Workspace::lookup_symbol(name);

        symbol->assign(new_val, LOC);
      }
   else Assert(0);   // since checked above
   
   return Value_P(new Value(name, LOC));
}
//-----------------------------------------------------------------------------
Token
Quad_TF::tf2(const UCS_string & name)
{
NamedObject * obj = Workspace::lookup_existing_name(name);
   if (obj == 0)   return Token(TOK_APL_VALUE1, Value::Str0_P);

const Function * function = obj->get_function();

   if (function)
      {
        if (obj->is_user_defined())
           {
             UCS_string ucs = tf2_fun(name, *function);
             Value_P Z(new Value(ucs, LOC));
             Z->check_value(LOC);
             return Token(TOK_APL_VALUE1, Z);
           }

        // quad function or primitive: return ''
        return Token(TOK_APL_VALUE1, Value::Str0_P);
      }

Symbol * symbol = obj->get_symbol();
   if (symbol)
      {
        Value_P value = symbol->get_apl_value();
        if (!!value)   return tf2_var(name, value);

        /* not a variable: fall through */
      }

   return Token(TOK_APL_VALUE1, Value::Str0_P);
}
//-----------------------------------------------------------------------------
Token
Quad_TF::tf2_var(const UCS_string & var_name, Value_P value)
{
   // try inverse 2 ⎕TF and return it if successful
   //
   if (value->is_apl_char_vector())
      {
        UCS_string data(*value.get());
        UCS_string new_var_or_fun;

        tf2_parse(data, new_var_or_fun);
        if (new_var_or_fun.size())   // if data is an inverse TF2
           {
             Value_P Z(new Value(new_var_or_fun, LOC));
             Z->check_value(LOC);
             return Token(TOK_APL_VALUE1, Z);
          }
      }

UCS_string ucs(var_name);
   ucs.append(UNI_LEFT_ARROW);

const bool error = tf2_ravel(0, ucs, value);

   // return '' for invalid values.
   //
   if (error)   return Token(TOK_APL_VALUE1, Value::Str0_P);

Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Quad_TF::tf2_parse(const UCS_string & ucs, UCS_string & new_var_or_fun)
{
Token_string tos;
Parser parser(PM_EXECUTE, LOC);

   try          { parser.parse(ucs, tos); }
   catch(...)   { return; }

   if (tos.size() < 2)   return;   // too short for an inverse 2⎕TF

   // we expect either VAR←VALUE ... or ⎕FX fun-text. Try VAR←VALUE.
   //
   if (tos[0].get_Class() == TC_SYMBOL && tos[1].get_tag() == TOK_ASSIGN)
      {
        // simplify tos as much as possible. We replace A⍴B by reshaped B
        // and glue values together.
        //
        tf2_remove_UCS(tos);

        for (bool progress = true; progress;)
            {
              progress = false;
              tf2_remove_RHO(tos, progress);
              tf2_remove_parentheses(tos, progress);
              tf2_glue(tos, progress);
            }

        // at this point, we expect SYM←VALUE.
        //
        if (tos.size() != 3)                  return;
        if (tos[2].get_Class() != TC_VALUE)   return;

         tos[0].get_sym_ptr()->assign(tos[2].get_apl_val(), LOC);
         new_var_or_fun = tos[0].get_sym_ptr()->get_name();   // valid 2⎕TF
         return;
      }

   // Try ⎕FX fun-text
   //
   if (tos.size() != 2)                   return;
   if (tos[0].get_tag() != TOK_QUAD_FX)   return;
   if (tos[1].get_Class() != TC_VALUE)    return;

   {
     Value_P function_text =  tos[1].get_apl_val();
     const Token tok = Quad_FX::fun.eval_B(function_text);

   if (tok.get_Class() == TC_VALUE)   // ⎕FX successful
      {
        Value_P val = tok.get_apl_val();
        new_var_or_fun = UCS_string(*val.get());
      }
   }
}
//-----------------------------------------------------------------------------
void
Quad_TF::tf2_shape(UCS_string & ucs, const Shape & shape)
{
   // dont print anything if shape is a skalar or non-empty vector
   if (shape.get_volume() != 0 && shape.get_rank() <=1)   return;

   loop(r, shape.get_rank())
      {
        if (r)   ucs.append(UNI_ASCII_SPACE);
        ucs.append_number(shape.get_shape_item(r));
      }

   ucs.append(UNI_RHO);
}
//-----------------------------------------------------------------------------
bool
Quad_TF::tf2_ravel(int level, UCS_string & ucs, Value_P value)
{
const ShapeItem ec = value->element_count();

   // special case: empty vector (including '' and ⍳0)
   //
   if (ec == 0)
      {
        if (value->get_ravel(0).is_character_cell())   // ''
           {
              ucs.append(UNI_SINGLE_QUOTE);
              ucs.append(UNI_SINGLE_QUOTE);
              return  false;
           }

        if (value->get_ravel(0).is_numeric())          // ⍳0
           {
              ucs.append(UNI_ASCII_L_PARENT);
              ucs.append(UNI_ASCII_0);
              ucs.append(UNI_RHO);
              ucs.append(UNI_ASCII_0);
              ucs.append(UNI_ASCII_R_PARENT);
              return  false;
           }

        // other empty values: print ( shape ⍴ prototype )
        ucs.append(UNI_ASCII_L_PARENT);

        tf2_shape(ucs, value->get_shape());

        Value_P proto = value->prototype(LOC);
        tf2_ravel(level + 1, ucs, proto);
        ucs.append(UNI_ASCII_R_PARENT);
        return  false;
      }

   // special case: char-only ravel.
   //
   if (!value->NOTCHAR())
      {
        tf2_shape(ucs, value->get_shape());

        // if all elements are in ⎕AV then print 'elements' otherwise use ⎕UCS.
        bool use_UCS = false;
        loop(e, ec)
           {
             const Unicode uni = value->get_ravel(e).get_char_value();
             if (Avec::find_char(uni) == Invalid_CHT)   // uni not in ⎕AV
                {
                  use_UCS = true;
                  break;
                }
           }

        if (use_UCS)
           {
             if (level)   ucs.append(UNI_ASCII_L_PARENT);
             ucs.append(UNI_QUAD_QUAD);
             ucs.append(UNI_ASCII_U);
             ucs.append(UNI_ASCII_C);
             ucs.append(UNI_ASCII_S);
             loop(e, ec)
                {
                  ucs.append(UNI_ASCII_SPACE);
                  const Unicode uni = value->get_ravel(e).get_char_value();
                  ucs.append_number(uni);
                }
             if (level)   ucs.append(UNI_ASCII_R_PARENT);
           }
        else
           {
             ucs.append(UNI_SINGLE_QUOTE);
             loop(e, ec)
                {
                  const Unicode uni = value->get_ravel(e).get_char_value();
                  ucs.append(uni);
                  if (uni == UNI_SINGLE_QUOTE)   ucs.append(UNI_SINGLE_QUOTE);
                }
             ucs.append(UNI_SINGLE_QUOTE);
           }
        return  false;
      }

const Depth depth = value->compute_depth();
   if (depth && level)   ucs.append(UNI_ASCII_L_PARENT);

   // maybe print shape followed by ⍴
   //
   tf2_shape(ucs, value->get_shape());

   loop(e, ec)
      {
        if (e)   ucs.append(UNI_ASCII_SPACE);
        const Cell & cell = value->get_ravel(e);

        if (cell.is_pointer_cell())
           {
             if (tf2_ravel(level + 1, ucs, cell.get_pointer_value()))
                return true;
           }
        else if (cell.is_lval_cell())
           {
             return true;
           }
        else if (cell.is_complex_cell())
           {
             PrintContext pctx(PR_APL_MIN, MAX_QUAD_PP, 0.0, MAX_QUAD_PW);
             bool scaled = true;
             UCS_string ucs1(cell.get_real_value(), scaled, pctx);
             UCS_string ucs2(cell.get_imag_value(), scaled, pctx);
             ucs.append(ucs1);
             ucs.append(UNI_ASCII_J);
             ucs.append(ucs1);
           }
        else if (cell.is_float_cell())
           {
             PrintContext pctx(PR_APL_MIN, MAX_QUAD_PP, 0.0, MAX_QUAD_PW);
             bool scaled = true;
             UCS_string ucs1(cell.get_real_value(), scaled, pctx);
             ucs.append(ucs1);
           }
        else
           {
             PrintContext pctx(PR_APL_FUN);
             const PrintBuffer pb = cell.character_representation(pctx);

             ucs.append(pb.l1());
           }
      }

   if (depth && level)   ucs.append(UNI_ASCII_R_PARENT);

   return false;
}
//-----------------------------------------------------------------------------
int
Quad_TF::tf2_remove_UCS(Token_string & tos)
{
int d = 0;
bool ucs_active = false;

   loop(s, tos.size())
      {
        if (s < (tos.size() - 1) && tos[s].get_tag() == TOK_QUAD_UCS)
           {
              ucs_active = true;
              continue;
           }

        if (d != s)   move_1(tos[d], tos[s], LOC);   // dont copy to itself

        if (ucs_active)   // translate
           {
              Assert(tos[d].get_Class() == TC_VALUE);
              tos[d].get_apl_val()->toggle_UCS();
              ucs_active = false;
           }

        ++d;
      }

   tos.shrink(d);
   return 0;   // no error(s)
}
//-----------------------------------------------------------------------------
int
Quad_TF::tf2_remove_RHO(Token_string & tos, bool & progress)
{
int d = 0;

   loop(s, tos.size())
      {
        // we replace A⍴B by B reshaped to A. But only if the element count in
        // B agrees with A (otherwise we need to glue first).
        //
        if (s < (tos.size() - 2)                  &&
            tos[s    ].get_Class() == TC_VALUE    &&                // A
            tos[s + 1].get_tag()   == TOK_F12_RHO &&                // ⍴
            tos[s + 2].get_Class() == TC_VALUE)                     // B 
           {
             Value_P aval = tos[s].get_apl_val();
             Value_P bval = tos[s + 2].get_apl_val();
             Shape sh(aval, 0, 0);
             if (sh.get_volume() == bval->element_count())   // same shape
                {
                  Token t(TOK_APL_VALUE1, bval);   // grouped value
                  move_1(tos[d], t, LOC);
                  bval->set_shape(sh);
                  progress = true;
                  ++d;
                  s += 2;
                 continue;
                }
             else
                {
                  move_2(tos[d], Bif_F12_RHO::fun.do_reshape(sh, bval), LOC);
                  progress = true;
                  ++d;
                  s += 2;
                 continue;
                }
           }

        if (d != s)   move_1(tos[d], tos[s], LOC);   // dont copy to itself
        ++d;
      }

   tos.shrink(d);
   return 0;   // no error(s)
}
//-----------------------------------------------------------------------------
int
Quad_TF::tf2_remove_parentheses(Token_string & tos, bool & progress)
{
int d = 0;

   loop(s, tos.size())   // ⍴ must not be first or last
      {
        // we replace A⍴B by B reshaped to A. But only if the element count in
        // B agrees with A (otherwise we need to glue first).
        //
        if (s < (tos.size() - 2)                 &&
            tos[s].get_tag()       == TOK_L_PARENT &&
            tos[s + 1].get_Class() == TC_VALUE     &&
            tos[s + 2].get_tag()   == TOK_R_PARENT)   // ( B )
           {
             move_1(tos[d], tos[s + 1], LOC);   // override A of A⍴B
             progress = true;
             ++d;
             s += 2;
             continue;
           }

        if (d != s)   move_1(tos[d], tos[s], LOC);   // dont copy to itself
        ++d;
      }

   tos.shrink(d);
   return 0;   // no error(s)
}
//-----------------------------------------------------------------------------
int
Quad_TF::tf2_glue(Token_string & tos, bool & progress)
{
int d = 0;

   loop(s, tos.size())   // ⍴ must not be first or last
      {
        if (d != s)   move_1(tos[d], tos[s], LOC);   // dont copy to itself

        if (tos[d].get_Class() == TC_VALUE)
           {
             while ((s + 1) < tos.size() && tos[s + 1].get_Class() == TC_VALUE)
                {
                   Value::glue(tos[d], tos[d], tos[s + 1], LOC);
                   progress = true;
                  ++s;
                }
           }

        ++d;
      }

   tos.shrink(d);
   return 0;   // no error(s)
}
//-----------------------------------------------------------------------------
UCS_string
Quad_TF::tf2_fun(const UCS_string & fun_name, const Function & fun)
{
UCS_string ucs;
   tf2_fun_ucs(ucs, fun_name, fun);

   return ucs;
}
//-----------------------------------------------------------------------------
void
Quad_TF::tf2_fun_ucs(UCS_string & ucs, const UCS_string & fun_name,
                    const Function & fun)
{
const UCS_string text = fun.canonical(false);
vector<UCS_string> lines;
   text.to_vector(lines);

   ucs.append(UNI_QUAD_QUAD);
   ucs.append(UNI_ASCII_F);
   ucs.append(UNI_ASCII_X);

   loop(l, lines.size())
      {
        ucs.append(UNI_ASCII_SPACE);
        ucs.append(UNI_SINGLE_QUOTE);

        loop(c, lines[l].size())
            {
              const Unicode uni = lines[l][c];
              ucs.append(uni);
              if (uni == UNI_SINGLE_QUOTE)   ucs.append(UNI_SINGLE_QUOTE);
            }
        ucs.append(UNI_SINGLE_QUOTE);
      }

}
//-----------------------------------------------------------------------------
Value_P
Quad_TF::tf3(const UCS_string & name)
{
NamedObject * obj = Workspace::lookup_existing_name(name);
   if (obj == 0)   return Value::Str0_P;

   if (obj->get_function())
      {
        // 3⎕TF is not defined for functions.
        //
        return Value::Str0_P;
      }

Symbol * symbol = obj->get_symbol();
   if (symbol)
      {
        Value_P value = symbol->get_apl_value();
        if (!!value)
           {
             CDR_string cdr;
             CDR::to_CDR(cdr, value);

             return Value_P(new Value(cdr, LOC));
           }

        /* not a variable: fall through */
      }

   return Value::Str0_P;
}
//-----------------------------------------------------------------------------
