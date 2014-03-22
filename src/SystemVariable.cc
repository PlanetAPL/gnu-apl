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

#include <langinfo.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/resource.h>   // for ⎕WA
#include <sys/time.h>

// FreeBSD 8.x lacks utmpx.h
#ifdef HAVE_UTMPX_H
# include <utmpx.h>           // for ⎕UL
#endif

#include "CDR.hh"
#include "CharCell.hh"
#include "Common.hh"
#include "FloatCell.hh"
#include "IndexExpr.hh"
#include "Input.hh"
#include "IntCell.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "ProcessorID.hh"
#include "StateIndicator.hh"
#include "SystemVariable.hh"
#include "UserFunction.hh"
#include "Value.hh"
#include "Workspace.hh"

UCS_string Quad_QUOTE::prompt;

ShapeItem Quad_SYL::si_depth_limit = 0;
ShapeItem Quad_SYL::value_count_limit = 0;
ShapeItem Quad_SYL::ravel_count_limit = 0;

int Quad_ARG::argc = 0;
const char ** Quad_ARG::argv = 0;

//=============================================================================
void
SystemVariable::assign(Value_P value, const char * loc)
{
   CERR << "SystemVariable::assign() not (yet) implemented for "
        << get_Id() << endl;
   FIXME;
}
//-----------------------------------------------------------------------------
void
SystemVariable::assign_indexed(Value_P X, Value_P value)
{
   CERR << "SystemVariable::assign_indexed() not (yet) implemented for "
        << get_Id() << endl;
   FIXME;
}
//-----------------------------------------------------------------------------
ostream &
SystemVariable::print(ostream & out) const
{
   return out << get_Id();
}
//-----------------------------------------------------------------------------
void
SystemVariable::get_attributes(int mode, Cell * dest) const
{
   switch(mode)
      {
        case 1: // valences (always 1 0 0 for variables)
                new (dest + 0) IntCell(1);
                new (dest + 1) IntCell(0);
                new (dest + 2) IntCell(0);
                break;

        case 2: // creation time (always 7⍴0 for variables)
                new (dest + 0) IntCell(0);
                new (dest + 1) IntCell(0);
                new (dest + 2) IntCell(0);
                new (dest + 3) IntCell(0);
                new (dest + 4) IntCell(0);
                new (dest + 5) IntCell(0);
                new (dest + 6) IntCell(0);
                break;

        case 3: // execution properties (always 4⍴0 for variables)
                new (dest + 0) IntCell(0);
                new (dest + 1) IntCell(0);
                new (dest + 2) IntCell(0);
                new (dest + 3) IntCell(0);
                break;

        case 4: {
                  Value_P val = get_apl_value();
                  const int CDR_type = val->get_CDR_type();
                  const int brutto = val->total_size_brutto(CDR_type);
                  const int data = val->data_size(CDR_type);

                  new (dest + 0) IntCell(brutto);
                  new (dest + 1) IntCell(data);
                }
                break;

        default:  Assert(0 && "bad mode");
      }

}
//=============================================================================
Quad_AV::Quad_AV()
   : RO_SystemVariable(ID_QUAD_AV)
{
   Symbol::assign(Value::AV_P, LOC);
}
//-----------------------------------------------------------------------------
Unicode
Quad_AV::indexed_at(uint32_t pos)
{
   if (pos < MAX_AV)   return Value::AV_P->get_ravel(pos).get_char_value();
   return UNI_AV_MAX;
}
//=============================================================================
Quad_AI::Quad_AI()
   : RO_SystemVariable(ID_QUAD_AI),
     session_start(now()),
     user_wait(0)
{
   Symbol::assign(get_apl_value(), LOC);
}
//-----------------------------------------------------------------------------
Value_P
Quad_AI::get_apl_value() const
{
const int total_ms = (now() - session_start)/1000;
const int user_ms  = user_wait/1000;

Value_P ret(new Value(4, LOC));
   new (&ret->get_ravel(0))   IntCell(ProcessorID::get_own_ID());
   new (&ret->get_ravel(1))   IntCell(total_ms - user_ms);
   new (&ret->get_ravel(2))   IntCell(total_ms);
   new (&ret->get_ravel(3))   IntCell(user_ms);

   ret->check_value(LOC);
   return ret;
}
//=============================================================================
Quad_ARG::Quad_ARG()
   : RO_SystemVariable(ID_QUAD_ARG)
{
}
//-----------------------------------------------------------------------------
Value_P
Quad_ARG::get_apl_value() const
{
Value_P Z(new Value(argc, LOC));
Cell * C = &Z->get_ravel(0);

   loop(a, Quad_ARG::argc)
      {
        const char * arg = Quad_ARG::argv[a];
        const int len = strlen(arg);
        Value_P val(new Value(len, LOC));
        loop(l, len)   new (&val->get_ravel(l))   CharCell(Unicode(arg[l]));

        new (C++)   PointerCell(val);
      }

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_CT::Quad_CT()
   : SystemVariable(ID_QUAD_CT),
     current_ct(DEFAULT_QUAD_CT)
{
Value_P value(new Value(LOC));
   new (&value->get_ravel(0)) FloatCell(current_ct);

   value->check_value(LOC);

   Symbol::assign(value, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_CT::assign(Value_P value, const char * loc)
{
   if (!value->is_skalar_or_len1_vector())
      {
        if (value->get_rank() > 1)   RANK_ERROR;
        else                         LENGTH_ERROR;
      }

const Cell & cell = value->get_ravel(0);
   if (!cell.is_numeric())             DOMAIN_ERROR;
   if (cell.get_imag_value() != 0.0)   DOMAIN_ERROR;

APL_Float val = cell.get_real_value();
   if (val < 0.0)     DOMAIN_ERROR;

   // APL2 "discourages" the use of ⎕CT > 1E¯9.
   // We round down to MAX_QUAD_CT (= 1E¯9) instead.
   //
   if (val > Value::Max_CT_P->get_ravel(0).get_real_value())
      {
        value = Value::Max_CT_P;
      }

   Symbol::assign(value, LOC);
   current_ct = value->get_ravel(0).get_real_value();
}
//=============================================================================
Value_P
Quad_EM::get_apl_value() const
{
   if (error.error_code == E_NO_ERROR)
      {
        // return 3 0⍴' '
        //
        return Value::Str3_0_P;
      }

const UCS_string msg_1 = error.get_error_line_1();
const UCS_string msg_2 = error.get_error_line_2();
const UCS_string msg_3 = error.get_error_line_3();

   // compute max line length of the 3 error lines,
   //
const ShapeItem len_1 = msg_1.size();
const ShapeItem len_2 = msg_2.size();
const ShapeItem len_3 = msg_3.size();

ShapeItem max_len = len_1;
   if (max_len < len_2)   max_len = len_2;
   if (max_len < len_3)   max_len = len_3;

const Shape sh(3, max_len);
Value_P Z(new Value(sh, LOC));
   loop(l, max_len)
      {
        new (&Z->get_ravel(l))
         CharCell((l < len_1) ? msg_1[l] : UNI_ASCII_SPACE);
      }
   loop(l, max_len)
      {
        new (&Z->get_ravel(l + max_len))
         CharCell((l < len_2) ? msg_2[l] : UNI_ASCII_SPACE);
      }
   loop(l, max_len)
      {
        new (&Z->get_ravel(l + 2*max_len))
         CharCell((l < len_3) ? msg_3[l] : UNI_ASCII_SPACE);
      }

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Value_P
Quad_ET::get_apl_value() const
{
Value_P Z(new Value(2, LOC));

   new (&Z->get_ravel(0)) IntCell(Error::error_major(error.error_code));
   new (&Z->get_ravel(1)) IntCell(Error::error_minor(error.error_code));

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_FC::Quad_FC() : SystemVariable(ID_QUAD_FC)
{
   set_default();

Value_P value(new Value(6, LOC));

   loop(c, 6)   new (&value->get_ravel(c))  CharCell(current_fc[c]);

   value->check_value(LOC);

   Symbol::assign(value, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_FC::set_default()
{
   current_fc[0] = UNI_ASCII_FULLSTOP;
   current_fc[1] = UNI_ASCII_COMMA;
   current_fc[2] = UNI_STAR_OPERATOR;
   current_fc[3] = UNI_ASCII_0;
   current_fc[4] = UNI_ASCII_UNDERSCORE;
   current_fc[5] = UNI_OVERBAR;
}
//-----------------------------------------------------------------------------
void
Quad_FC::assign(Value_P value, const char * loc)
{
   if (!value->is_skalar_or_vector())   RANK_ERROR;

ShapeItem value_len = value->element_count();
   if (value_len > 6)   value_len = 6;

   loop(c, value_len)
       if (!value->get_ravel(c).is_character_cell())   DOMAIN_ERROR;

   // value is OK.
   //
   if (value_len < 6)   set_default();

   loop(c, value_len)
      {
        current_fc[c] = value->get_ravel(c).get_char_value();
      }

   // 0123456789,. are forbidden for ⎕FC[4 + ⎕IO]
   //
   if (Bif_F12_FORMAT::is_control_char(current_fc[4]))
      current_fc[4] = UNI_ASCII_SPACE;
}
//-----------------------------------------------------------------------------
void
Quad_FC::assign_indexed(const IndexExpr & IX, Value_P value)
{
Q(LOC)
   if (IX.value_count() != 1)   INDEX_ERROR;

   // at this point we have a one dimensional index. It it were non-empty,
   // then assign_indexed(Value) would have been called instead. Therefore
   // IX must be an elided index (like in ⎕FC[] ← value)
   //
   Assert1(!IX.values[0]);
   assign(value, LOC);   // ⎕FC[]←value
}
//-----------------------------------------------------------------------------
void
Quad_FC::assign_indexed(Value_P X, Value_P value)
{
   // we don't do skalar extension but require indices to match the value.
   //
ShapeItem ec = X->element_count();
   if (ec != value->element_count())   INDEX_ERROR;

   // ignore extra values.
   //
   if (ec > 6)   ec = 6;

const APL_Integer qio = Workspace::get_IO();
   loop(e, ec)
      {
        const APL_Integer idx = X->get_ravel(e).get_int_value() - qio;
        if (idx < 0)   continue;
        if (idx > 5)   continue;

        current_fc[idx] = value->get_ravel(e).get_char_value();
      }

   // 0123456789,. are forbidden for ⎕FC[4 + ⎕IO]
   //
   if (Bif_F12_FORMAT::is_control_char(current_fc[4]))
      current_fc[4] = UNI_ASCII_SPACE;
}
//-----------------------------------------------------------------------------
Value_P Quad_FC::get_apl_value() const
{
Value_P v1(new Value(6, LOC));
   loop(c, 6)   new (&v1->get_ravel(c))   CharCell(current_fc[c]);
   v1->check_value(LOC);
   return v1;
}
//=============================================================================
Quad_IO::Quad_IO()
   : SystemVariable(ID_QUAD_IO),
     current_io(1)
{
Value_P value(new Value(LOC));

   new (&value->get_ravel(0)) IntCell(current_io);

   value->check_value(LOC);

   Symbol::assign(value, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_IO::assign(Value_P value, const char * loc)
{
   if (!value->is_skalar_or_len1_vector())
      {
        if (value->get_rank() > 1)   RANK_ERROR;
        else                         LENGTH_ERROR;
      }

const Cell & cell = value->get_ravel(0);
   current_io = value->get_ravel(0).get_near_bool(0.1);
   Symbol::assign(current_io ? Value::One_P : Value::Zero_P, LOC);
}
//-----------------------------------------------------------------------------
int
Quad_IO::expunge()
{
   current_io = -1;
   Symbol::expunge();
   return 1;
}
//=============================================================================
Quad_L::Quad_L()
 : NL_SystemVariable(ID_QUAD_L)
{
   Symbol::assign(Value::Zero_P, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_L::assign(Value_P value, const char * loc)
{
StateIndicator * si = Workspace::SI_top_fun();
   if (si == 0)   return;

   // ignore assignments if error was SYNTAX ERROR or VALUE ERROR
   //
   if (si->get_error().is_syntax_or_value_error())   return;

   si->set_L(value);
}
//-----------------------------------------------------------------------------
Value_P
Quad_L::get_apl_value() const
{
const StateIndicator * si = Workspace::SI_top_error();
   if (si)
      {
        Value_P ret = si->get_L();
        if (!!ret)   return  ret;
      }

   VALUE_ERROR;
}
//=============================================================================
Quad_LC::Quad_LC()
   : RO_SystemVariable(ID_QUAD_LC)
{
   Symbol::assign(Value::Zero_P, LOC);
}
//-----------------------------------------------------------------------------
Value_P
Quad_LC::get_apl_value() const
{

   // count how many function elements Quad-LC has...
   //
int len = 0;
   for (StateIndicator * si = Workspace::SI_top();
        si; si = si->get_parent())
       {
         if (si->get_executable()->get_parse_mode() == PM_FUNCTION)   ++len;
       }

Value_P Z(new Value(len, LOC));

   for (StateIndicator * si = Workspace::SI_top();
        si; si = si->get_parent())
       {
         if (si->get_executable()->get_parse_mode() == PM_FUNCTION)
            new (Z->next_ravel())   IntCell(si->get_line());
       }

   Z->set_default(*Value::Zero_P);
   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_LX::Quad_LX()
   : NL_SystemVariable(ID_QUAD_LX)
{
   Symbol::assign(Value::Str0_P, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_LX::assign(Value_P value, const char * loc)
{
   if (value->get_rank() > 1)      RANK_ERROR;
   if (!value->is_char_string())   DOMAIN_ERROR;

   Symbol::assign(value, LOC);
}
//=============================================================================
void
Quad_NLT::assign(Value_P value, const char * loc)
{
   if (value->get_rank() > 1)      RANK_ERROR;
   if (!value->is_char_string())   DOMAIN_ERROR;

   // expect language [_territory] [.code‐set] [@modifier]
   // e.g. en_US.utf8
   //
   // we only check that it is 7-bit ASCII and leave it to setlocale()
   // to check if value is OK.
   //
const ShapeItem len = value->element_count();
   if (len > 100)   LENGTH_ERROR;

char new_locale[len + 1];
   loop(l, len)
      {
         new_locale[l] = value->get_ravel(l).get_char_value();
         if (new_locale[l] < ' ')    DOMAIN_ERROR;
         if (new_locale[l] > 0x7E)   DOMAIN_ERROR;
      }

   new_locale[len] = 0;

const char * locale = setlocale(LC_ALL, new_locale);
   if (locale == 0)   DOMAIN_ERROR;

   // setlocale() has not complained, so we can set the ⎕NLT;
   Symbol::assign(value, LOC);
}
//-----------------------------------------------------------------------------
Value_P
Quad_NLT::get_apl_value() const
{
   // get current locale
   //
const char * locale = setlocale(LC_CTYPE, 0);   // e,g, "en_US.utf8"
   if (locale == 0)   return Value::Str0_P;

UCS_string ulocale(locale);

Value_P Z(new Value(ulocale, LOC));
   Z->set_default(*Value::Spc_P);

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_PP::Quad_PP()
   : SystemVariable(ID_QUAD_PP),
     current_pp(DEFAULT_QUAD_PP)
{
Value_P value(new Value(LOC));

   new (&value->get_ravel(0)) IntCell(current_pp);

   value->check_value(LOC);
   Symbol::assign(value, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_PP::assign(Value_P value, const char * loc)
{
   if (!value->is_skalar_or_len1_vector())
      {
        if (value->get_rank() > 1)   RANK_ERROR;
        else                         LENGTH_ERROR;
      }

const Cell & cell = value->get_ravel(0);
const APL_Integer val = cell.get_near_int(0.1);
   if (val < MIN_QUAD_PP)   DOMAIN_ERROR;

   if (val > MAX_QUAD_PP)
      {
        current_pp = MAX_QUAD_PP;
        value = Value::Max_PP_P;
      }
   else
      {
        current_pp = val;
      }

   Symbol::assign(value, LOC);
}
//=============================================================================
Quad_PR::Quad_PR()
   : SystemVariable(ID_QUAD_PR),
     current_pr(UNI_ASCII_SPACE)
{
Value_P value(new Value(current_pr, LOC));

   value->check_value(LOC);
   Symbol::assign(value, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_PR::assign(Value_P value, const char * loc)
{
UCS_string ucs = value->get_UCS_ravel();

   if (ucs.size() > 1)   LENGTH_ERROR;

   current_pr = ucs;

   Symbol::assign(value, LOC);
}
//=============================================================================
Quad_PS::Quad_PS()
   : SystemVariable(ID_QUAD_PS),
     current_ps(PR_APL)
{
   Symbol::assign(Value::Zero_P, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_PS::assign(Value_P value, const char * loc)
{
   if (!value->is_skalar_or_len1_vector())
      {
        if (value->get_rank() > 1)   RANK_ERROR;
        else                         LENGTH_ERROR;
      }

const Cell & cell = value->get_ravel(0);
const APL_Integer val = cell.get_near_int(0.1);
   if      (val == 0)   current_ps = PR_APL;
   else if (val == 1)   current_ps = PR_APL_FUN;
   else if (val == 2)   current_ps = PR_BOXED_CHAR;
   else if (val == 3)   current_ps = PR_BOXED_GRAPHIC;
   else                 DOMAIN_ERROR;

   new (&value->get_ravel(0))  IntCell(val);
   Symbol::assign(value, LOC);
}
//=============================================================================
Quad_PT::Quad_PT()
   : RO_SystemVariable(ID_QUAD_PT)
{
   Symbol::assign(Value::Zero_P, LOC);
}
//-----------------------------------------------------------------------------
Value_P
Quad_PT::get_apl_value() const
{
Value_P Z(new Value(LOC));
   new (&Z->get_ravel(0))   FloatCell(Workspace::get_CT());

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_PW::Quad_PW()
   : SystemVariable(ID_QUAD_PW),
     current_pw(80)
{
Value_P value(new Value(LOC));

   new (&value->get_ravel(0)) IntCell(current_pw);

   value->check_value(LOC);
   Symbol::assign(value, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_PW::assign(Value_P value, const char * loc)
{
   if (!value->is_skalar_or_len1_vector())
      {
        if (value->get_rank() > 1)   RANK_ERROR;
        else                         LENGTH_ERROR;
      }

const Cell & cell = value->get_ravel(0);
const APL_Integer val = cell.get_near_int(0.1);

   // min val is 30. Ignore smaller values.
   if (val < 30)   return;

   // max val is system specific. Ignore larger values.
   if (val > MAX_QUAD_PW)   return;

   current_pw = val;
   Symbol::assign(value, LOC);
}
//=============================================================================
Quad_QUAD::Quad_QUAD()
 : SystemVariable(ID_QUAD_QUAD)
{
}
//-----------------------------------------------------------------------------
void
Quad_QUAD::assign(Value_P value, const char * loc)
{
   // write pending LF from  ⍞ (if any)
   Quad_QUOTE::done(true, LOC);

   value->print(COUT);
}
//-----------------------------------------------------------------------------
void
Quad_QUAD::resolve(Token & token, bool left)
{
   if (left)   return;   // ⎕←xxx

   // write pending LF from  ⍞ (if any)
   Quad_QUOTE::done(true, LOC);

   COUT << UNI_QUAD_QUAD << ":" << endl;

UCS_string in = Input::get_line();

   move_2(token, Bif_F1_EXECUTE::execute_statement(in), LOC);
}
//=============================================================================
Quad_QUOTE::Quad_QUOTE()
 : SystemVariable(ID_QUOTE_QUAD)
{
   // we assign a dummy value so that ⍞ is not undefined.
   //
Value_P dummy(new Value(UCS_string(LOC), LOC));
   dummy->set_complete();
   Symbol::assign(dummy, LOC);
   dummy->set_forever();
}
//-----------------------------------------------------------------------------
void
Quad_QUOTE::done(bool with_LF, const char * loc)
{
   Log(LOG_cork)
      CERR << "Quad_QUOTE::done(" << with_LF << ") called from " << loc
           << " , buffer = [" << prompt << "]" << endl;

   if (prompt.size())
      {
        if (with_LF)   COUT << UNI_ASCII_LF;
        prompt.clear();
      }
}
//-----------------------------------------------------------------------------
void
Quad_QUOTE::assign(Value_P value, const char * loc)
{
   Log(LOG_cork)
      CERR << "Quad_QUOTE::assign() called, buffer = ["
           << prompt << "]" << endl;

PrintContext pctx(PR_QUOTE_QUAD);
PrintBuffer pb(*value, pctx);
   if (pb.get_height() > 1)   // multi line output: flush and restart corking
      {
        loop(y, pb.get_height())
           {
             done(true, LOC);
             prompt = pb.get_line(y).no_pad();
             COUT << prompt;
           }
      }
   else
      {
        COUT << pb.l1();
        prompt.append(pb.l1());
      }

   Log(LOG_cork)
      CERR << "Quad_QUOTE::assign() done, buffer = ["
           << prompt << "]" << endl;
}
//-----------------------------------------------------------------------------
Value_P
Quad_QUOTE::get_apl_value() const
{
   Log(LOG_cork)
      CERR << "Quad_QUOTE::get_apl_value() called, buffer = ["
           << prompt << "]" << endl;

   // get_quad_cr_line() calls done(), so we save the current prompt.
   //
UCS_string old_prompt(prompt);

UCS_string in = Input::get_quad_cr_line(old_prompt);
const UCS_string qpr = Workspace::get_PR();

   if (qpr.size() > 0)   // valid prompt replacement char
      {
        loop(i, in.size())
           {
             if (i >= old_prompt.size())   break;
             if (old_prompt[i] != in[i])   break;
             in[i] = qpr[0];
           }
      }

Value_P Z(new Value(in, LOC));
   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_R::Quad_R()
 : NL_SystemVariable(ID_QUAD_R)
{
   Symbol::assign(Value::Zero_P, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_R::assign(Value_P value, const char * loc)
{
StateIndicator * si = Workspace::SI_top_fun();
   if (si == 0)   return;

   // ignore assignments if error was SYNTAX ERROR or VALUE ERROR
   //
   if (si->get_error().is_syntax_or_value_error())   return;

   si->set_R(value);
}
//-----------------------------------------------------------------------------
Value_P
Quad_R::get_apl_value() const
{
const StateIndicator * si = Workspace::SI_top_error();
   if (si)
      {
        Value_P ret = si->get_R();
        if (!!ret)   return  ret;
      }

   VALUE_ERROR;
}
//=============================================================================
void
Quad_SYL::assign(Value_P value, const char * loc)
{
   // Quad_SYL is mostly read-only, so we only allow assign_indexed() with
   // certain values.
   //
   if (!!value)   SYNTAX_ERROR;

   // this assign is called from the constructor in order to trigger the
   // creation of a symbol for Quad_SYL.
   //
   Symbol::assign(Value::Zero_P, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_SYL::assign_indexed(const IndexExpr & IDX, Value_P value)
{
   //  must be an array index of the form [2; something]
   //
   if (IDX.value_count() != 2)   INDEX_ERROR;

   // IDX is in reverse order: ⎕SYL[X1;X2]
   //
Value_P X2 = IDX.values[0];

const APL_Float qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

   if (!X2)                                             INDEX_ERROR;
   if (X2->element_count() != 1)                        INDEX_ERROR;
   if (!X2->get_ravel(0).is_near_int(qct))              INDEX_ERROR;
   if (X2->get_ravel(0).get_near_int(qct) != qio + 1)   INDEX_ERROR;

Value_P X1 = IDX.values[1];
   delete &IDX;

   assign_indexed(X1, value);
}
//-----------------------------------------------------------------------------
void
Quad_SYL::assign_indexed(Value_P X, Value_P B)
{
const APL_Float qct = Workspace::get_CT();

   if (!(X->is_int_skalar(qct) || X->is_int_vector(qct)))   INDEX_ERROR;
   if (!(B->is_int_skalar(qct) || B->is_int_vector(qct)))   DOMAIN_ERROR;
   if (X->element_count() != B->element_count())            LENGTH_ERROR;

const ShapeItem ec = X->element_count();
const APL_Integer qio = Workspace::get_IO();

   loop(e, ec)
      {
        const APL_Integer x = X->get_ravel(e).get_near_int(qct) - qio;
        const APL_Integer b = B->get_ravel(e).get_near_int(qct);

        if (x == SYL_SI_DEPTH_LIMIT)   // SI depth limit
           {
             const int depth = Workspace::SI_entry_count();
             if (b < (depth + 4))   DOMAIN_ERROR;    // limit too low
             si_depth_limit = b;
           }
        else if (x == SYL_VALUE_COUNT_LIMIT)   // value count limit
           {
             if (b < (Value::value_count + 10))   DOMAIN_ERROR;    // too low
             value_count_limit = b;
           }
        else if (x == SYL_RAVEL_BYTES_LIMIT)   // ravel bytes depth limit
           {
             const int cells = b / sizeof(Cell);
             if (cells < (Value::total_ravel_count + 1000))   DOMAIN_ERROR;

             ravel_count_limit = cells;
           }
        else
           {
             INDEX_ERROR;
           }
      }
}
//-----------------------------------------------------------------------------
Value_P
Quad_SYL::get_apl_value() const
{
const Shape sh(SYL_MAX, 2);
Value_P Z( new Value(sh, LOC));

#define syl2(n, e, v) syl1(n, e, v)
#define syl1(n, _e, v) \
  new (Z->next_ravel()) PointerCell(Value_P(new Value(UCS_string(UTF8_string(n)), LOC))); \
  new (Z->next_ravel()) IntCell(v);
#include "SystemLimits.def"

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_TC::Quad_TC()
   : RO_SystemVariable(ID_QUAD_TC)
{
   Symbol::assign(Value::Quad_TC_P, LOC);
}
//-----------------------------------------------------------------------------
Value_P
Quad_TC::get_apl_value() const
{
   return Value::Quad_TC_P;
}
//=============================================================================
Quad_TS::Quad_TS()
   : RO_SystemVariable(ID_QUAD_TS)
{
   Symbol::assign(Value::Zero_P, LOC);
}
//-----------------------------------------------------------------------------
Value_P
Quad_TS::get_apl_value() const
{
const int offset = Workspace::get_v_quad_TZ().get_offset();
const YMDhmsu time(now() + offset);

Value_P Z(new Value(7, LOC));
   new (&Z->get_ravel(0)) IntCell(time.year);
   new (&Z->get_ravel(1)) IntCell(time.month);
   new (&Z->get_ravel(2)) IntCell(time.day);
   new (&Z->get_ravel(3)) IntCell(time.hour);
   new (&Z->get_ravel(4)) IntCell(time.minute);
   new (&Z->get_ravel(5)) IntCell(time.second);
   new (&Z->get_ravel(6)) IntCell(time.micro / 1000);

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_TZ::Quad_TZ()
   : SystemVariable(ID_QUAD_TZ)
{
   // the GNU/Linux timezone variable conflicts with function timezone() on
   // other systems. timezone.tz_minuteswest in gettimeofday() does not
   // sound reliable either.
   // 
   // We compute localtime() and gmtime() and take the difference (hoping
   // that localtime() and gmtime() are mor portable.
   // 
timeval now;
   gettimeofday(&now, 0);

tm * local = localtime(&now.tv_sec);
const int local_minutes = local->tm_min + 60*local->tm_hour;

tm * gmean = gmtime(&now.tv_sec);
     const int gm_minutes = gmean->tm_min + 60*gmean->tm_hour;
     
const int diff_minutes = local_minutes - gm_minutes;
   offset_seconds = 60*diff_minutes;

Value_P value(new Value(LOC));
   if (offset_seconds % 3600 == 0)   // full hour
      new (&value->get_ravel(0))   IntCell(offset_seconds/3600);
   else
      new (&value->get_ravel(0))   FloatCell(offset_seconds/3600.0);

   value->check_value(LOC);

   Symbol::assign(value, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_TZ::assign(Value_P value, const char * loc)
{
   if (!value->is_skalar())   RANK_ERROR;

   // ignore values outside [-12 ... 12], DOMAIN ERROR for bad types.

Cell & cell = value->get_ravel(0);
   if (cell.is_integer_cell())
      {
        const APL_Integer ival = cell.get_int_value();
        if (ival < -12)   return;
        if (ival > 12)    return;
        offset_seconds = ival*3600;
        Symbol::assign(value, LOC);
        return;
      }

   if (cell.is_float_cell())
      {
        const APL_Float fval = cell.get_real_value();
        if (fval < -12)   return;
        if (fval > 12)    return;
        offset_seconds = int(0.5 + fval*3600);
        Symbol::assign(value, LOC);
        return;
      }

   DOMAIN_ERROR;
}
//=============================================================================
Quad_UL::Quad_UL()
   : RO_SystemVariable(ID_QUAD_UL)
{
   Symbol::assign(get_apl_value(), LOC);
}
//-----------------------------------------------------------------------------
Value_P
Quad_UL::get_apl_value() const
{
int user_count = 0;

#ifdef USER_PROCESS
   setutxent();   // rewind utmp file

   for (;;)
       {
         utmpx * entry = getutxent();
         if (entry == 0)   break;   // end of  user accounting database

         if (entry->ut_type == USER_PROCESS)   ++user_count;
       }

   endutxent();   // close utmp file
#else
   user_count = 1;
#endif

Value_P Z(new Value(LOC));
   new (&Z->get_ravel(0))   IntCell(user_count);
   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_WA::Quad_WA()
   : RO_SystemVariable(ID_QUAD_WA)
{
   Symbol::assign(Value::Zero_P, LOC);
}
//-----------------------------------------------------------------------------
Value_P
Quad_WA::get_apl_value() const
{
Value_P Z(new Value(LOC));

uint64_t total = total_memory;   // max memory as reported by RLIM_INFINITY
uint64_t proc_mem = 0;            // memory as reported proc/mem_info

   {
     FILE * pm = fopen("/proc/meminfo", "r");

     if (pm)   // /proc/meminfo exists
        {
          for (;;)
              {
                char buffer[2000];
                if (fgets(buffer, sizeof(buffer) - 1, pm) == 0)   break;
                buffer[sizeof(buffer) - 1] = 0;
                if (!strncmp(buffer, "MemFree:", 8))
                   {
                     proc_mem = atoi(buffer + 8);
                     proc_mem *= 1024;
                     break;
                   }
              }

          fclose(pm);
        }
   }

   if (proc_mem == 0)   // nothing from /proc/meminfo
      {
        if (total == RLIM_INFINITY)   // no rlimit
           {
             CERR << "Cannot properly determine ⎕WA (process has no memory"
                     " limit and /proc/meminfo provided no info)" << endl;
             total = 1000000;
           }
        else                          // process has rlimit
           {
             total -= Value::total_ravel_count * sizeof(Cell)
                    + Value::value_count * sizeof(Value);
           }
      }
   else
      {
        if (total == RLIM_INFINITY)   // no rlimit
           {
             total = proc_mem;   // use value from .proc/meminfo
           }
        else                          // process has rlimit
           {
             total -= Value::total_ravel_count * sizeof(Cell)
                    + Value::value_count * sizeof(Value);
             if (total > proc_mem)   total = proc_mem;   // use minimum
           }
      }

   new (&Z->get_ravel(0))   IntCell(total);

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
