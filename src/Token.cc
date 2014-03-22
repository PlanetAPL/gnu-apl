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

#include <string.h>

#include "IndexExpr.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "Symbol.hh"
#include "Token.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
Token::Token(const Token & other)
   : tag(TOK_VOID)
{
   copy_1(*this, other, "Token::Token(other)");
}
//-----------------------------------------------------------------------------
Token::Token(TokenTag tg, IndexExpr & idx)
{
   Assert(&idx);

   if (tg == TOK_PINDEX)
      {
        // this token is a partial index in the prefix parser.
        // use it as is.
        //
        tag = TOK_PINDEX;
        value.index_val = &idx;
      }
   else
      {
        // this token is a complete index
        //
        Assert(tg == TOK_INDEX);
        if (idx.value_count() < 2)   // [idx] or []
           {
             tag = TOK_AXES;
             if (idx.value_count() == 0)   // []
                {
                  new (&value._apl_val()) Value_P;
                }
             else                          // [x]
                {
                  new (&value._apl_val())   Value_P(idx.extract_value(0));
                }
           }
        else                         // [idx; ...]
           {
             tag = TOK_INDEX;
             value.index_val = &idx;
           }
      }
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, TokenTag tag)
{
   if (tag > 0xFFFF)   out << HEX(tag);
   else                out << HEX4(tag);

   return out;
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, TokenClass tc)
{
#define tcc(x) case x: out << #x;   break;
   switch(tc)
      {
        tcc(TC_INVALID)
        tcc(TC_END)
        tcc(TC_RETURN)
        tcc(TC_VALUE)
        tcc(TC_INDEX)
        tcc(TC_PINDEX)
        tcc(TC_SYMBOL)
        tcc(TC_FUN0)
        tcc(TC_FUN12)
        tcc(TC_OPER1)
        tcc(TC_OPER2)
        tcc(TC_R_PARENT)
        tcc(TC_L_PARENT)
        tcc(TC_R_BRACK)
        tcc(TC_L_BRACK)
        tcc(TC_R_ARROW)
        tcc(TC_ASSIGN)
        tcc(TC_LINE)
        tcc(TC_VOID)
        tcc(TC_DIAMOND)
        tcc(TC_NUMERIC)
        tcc(TC_SPACE)
        tcc(TC_NEWLINE)
        tcc(TC_COLON)
        tcc(TC_QUOTE)
        tcc(TC_OFF)
        tcc(TC_SI_LEAVE)
      }
#undef tcc

   return out;
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, const Token & token)
{
   if (token.get_tag() == TOK_CHARACTER)
      {
        out << "CHAR«" << token.get_char_val() << "»";
        return out;
      }

   if (token.get_tag() == TOK_APL_VALUE1 ||
       token.get_tag() == TOK_APL_VALUE2 ||
       token.get_tag() == TOK_APL_VALUE3)
      {
        if      (token.get_tag() == TOK_APL_VALUE1)   out << "VALUE1";
        else if (token.get_tag() == TOK_APL_VALUE2)   out << "VALUE2";
        else if (token.get_tag() == TOK_APL_VALUE3)   out << "VALUE3";
        else                                          out << "VALUE???";
        Value_P value = token.get_apl_val();
        Assert(value);
        const Depth depth = value->compute_depth();
        out << "«";
        for (Depth d = 0; d < depth; ++d)   out << "≡";

        if (value->get_rank())   out << value->get_shape();

        const PrintContext pctx(PR_APL);
        PrintBuffer pb(*value, pctx);
        bool more = pb.get_height() > 1;
        if (pb.get_height() > 0)
           {
             UCS_string ucs = pb.get_line(0).no_pad();
             if (ucs.size() > 20)
                {
                  ucs.shrink(20);
                  more = true;
                }
             out << ucs;
           }
        if (more)   out << "...";
        return out << "»";
      }

   if (token.get_tag() == TOK_ERROR)
      {
        return out << Error::error_name(ErrorCode(token.get_int_val()));
      }

   if (token.get_tag() == TOK_BRANCH)
      {
        return out << token.get_Id() << token.get_int_val();
      }

   if (token.get_Id() > ID_No_ID2)   return out << token.get_Id();

   switch(token.get_Class())
      {
        case TC_END:      return out << "END";
        case TC_RETURN:
             if (token.get_tag() == TOK_RETURN_EXEC)
                          return out << "RETURN ⍎";
             if (token.get_tag() == TOK_RETURN_STATS)
                          return out << "RETURN ◊";
             if (token.get_tag() == TOK_RETURN_VOID)
                          return out << "RETURN ∇FUN";
             if (token.get_tag() == TOK_RETURN_SYMBOL)
                          return out << "RETURN Z←FUN";
             return out << "RETURN ???";

        case TC_VALUE:    return token.print_value(out);

        case TC_INDEX:
        case TC_PINDEX:
             if (token.get_tag() == TOK_INDEX)
                return out << token.get_index_val();
             else if (token.get_tag() == TOK_AXES)
                return out << *token.get_axes();
             else
                FIXME;

        case TC_SYMBOL:   if (token.get_tag() == TOK_LSYMB)
                             {
                               token.get_sym_ptr()->print(out << "'");
                               out << UNI_LEFT_ARROW;
                             }
                          else if (token.get_tag() == TOK_LSYMB2)
                             {
                               token.get_sym_ptr()->print(out << "'(... ");
                               out << ")←";
                             }
                          else
                             {
                               token.get_sym_ptr()->print(out << "'");
                             }
                          return out;
        case TC_FUN0:
        case TC_FUN12:
        case TC_OPER1:
        case TC_OPER2:    return token.print_function(out);
        case TC_LINE:     return out << "\n[" << token.get_fun_line() << "] ";
        case TC_NUMERIC:  return out << "Numeric";
        case TC_NEWLINE:  return out << "{LFeed}";
      }

   return out <<  "{-unknown Token " << token.get_tag() << "-}";
}
//-----------------------------------------------------------------------------
void
Token::ChangeTag(TokenTag new_tag)
{
   Assert((tag & TV_MASK) == (new_tag & TV_MASK));
   // tag is ia const TokenTag, so we cheat a little here.
   (TokenTag &)tag = new_tag;
}
//-----------------------------------------------------------------------------
ostream &
Token::print_function(ostream & out) const
{
   switch(tag)
      {
        case TOK_QUAD_TC:
        case TOK_QUAD_TS:
        case TOK_QUAD_UL:
        case TOK_QUAD_WA:
        case TOK_QUAD_AF:
        case TOK_QUAD_CR:
        case TOK_QUAD_DL:
        case TOK_QUAD_EC:
        case TOK_QUAD_EX:
        case TOK_QUAD_SVQ:
        case TOK_QUAD_SVR:
        case TOK_QUAD_SVS:
        case TOK_QUAD_UCS:
        case TOK_QUAD_AT:
        case TOK_QUAD_EA:
        case TOK_QUAD_SVC:
        case TOK_QUAD_TF:
        case TOK_QUAD_ES:
        case TOK_QUAD_FX:
        case TOK_QUAD_NA:
        case TOK_QUAD_NL:
        case TOK_QUAD_SI:
        case TOK_QUAD_SVO:      return print_quad(out);

        case TOK_F1_EXECUTE:    return out << UNI_EXECUTE;

        case TOK_F2_LESS:       return out << UNI_ASCII_LESS;
        case TOK_F2_FIND:       return out << UNI_EPSILON_UBAR;
        case TOK_F2_EQUAL:      return out << UNI_ASCII_EQUAL;
        case TOK_F2_GREATER:    return out << UNI_ASCII_GREATER;
        case TOK_ASSIGN:        return out << UNI_LEFT_ARROW;
        case TOK_F2_AND:        return out << UNI_AND;
        case TOK_F2_OR:         return out << UNI_OR;
        case TOK_F2_INDEX:      return out << UNI_SQUISH_QUAD;
        case TOK_F2_LEQ:        return out << UNI_LESS_OR_EQUAL;
        case TOK_F2_MEQ:        return out << UNI_MORE_OR_EQUAL;
        case TOK_F2_UNEQ:       return out << UNI_NOT_EQUAL;
        case TOK_F2_NOR:        return out << UNI_NOR;
        case TOK_F2_NAND:       return out << UNI_NAND;

        case TOK_F12_BINOM:     return out << UNI_ASCII_EXCLAM;
        case TOK_F12_CIRCLE:    return out << UNI_CIRCLE;
        case TOK_F12_COMMA:     return out << UNI_ASCII_COMMA;
        case TOK_F12_COMMA1:    return out << UNI_COMMA_BAR;
        case TOK_F12_DECODE:    return out << UNI_UP_TACK;
        case TOK_F12_DIVIDE:    return out << UNI_DIVIDE;
        case TOK_F12_DOMINO:    return out << UNI_QUAD_DIVIDE;
        case TOK_F12_DROP:      return out << UNI_DOWN_ARROW;
        case TOK_F12_ELEMENT:   return out << UNI_ELEMENT;
        case TOK_F12_ENCODE:    return out << UNI_DOWN_TACK;
        case TOK_F12_EQUIV:     return out << UNI_EQUIVALENT;
        case TOK_F12_FORMAT:    return out << UNI_FORMAT;
        case TOK_F12_INDEX_OF:  return out << UNI_IOTA;
        case TOK_F12_LOGA:      return out << UNI_LOGARITHM;
        case TOK_F12_MINUS:     return out << UNI_ASCII_MINUS;
        case TOK_F12_PARTITION: return out << UNI_SUBSET;
        case TOK_F12_PICK:      return out << UNI_SUPERSET;
        case TOK_F12_PLUS:      return out << UNI_ASCII_PLUS;
        case TOK_F12_POWER:     return out << UNI_STAR_OPERATOR;
        case TOK_F12_RHO:       return out << UNI_RHO;
        case TOK_F12_RND_DN:    return out << UNI_LEFT_FLOOR;
        case TOK_F12_RND_UP:    return out << UNI_LEFT_CEILING;
        case TOK_F12_ROLL:      return out << UNI_ASCII_QUESTION;
        case TOK_F12_ROTATE:    return out << UNI_CIRCLE_STILE;
        case TOK_F12_ROTATE1:   return out << UNI_CIRCLE_BAR;
        case TOK_F12_SORT_ASC:  return out << UNI_SORT_ASCENDING;
        case TOK_F12_SORT_DES:  return out << UNI_SORT_DECENDING;
        case TOK_F12_STILE:     return out << UNI_DIVIDES;
        case TOK_F12_TAKE:      return out << UNI_UP_ARROW;
        case TOK_F12_TIMES:     return out << UNI_MULTIPLY;
        case TOK_F12_TRANSPOSE: return out << UNI_TRANSPOSE;
        case TOK_F12_WITHOUT:   return out << UNI_TILDE_OPERATOR;

        case TOK_JOT:           return out << UNI_RING_OPERATOR;

        case TOK_OPER1_EACH:
        case TOK_OPER1_REDUCE:
        case TOK_OPER1_REDUCE1:
        case TOK_OPER1_SCAN:
        case TOK_OPER1_SCAN1:
        case TOK_OPER2_PRODUCT: return out << get_Id();

        case TOK_FUN0:      return get_function()->print(out << "USER-F0 ");
        case TOK_FUN2:      return get_function()->print(out << "USER-F2 ");
        case TOK_OPER1:     return get_function()->print(out << "USER-OP1 ");
        case TOK_OPER2:     return get_function()->print(out << "USER-OP2 ");
      }

   // unknown tag.

   out << "{ unknown function Token " << tag;

   if (get_Id() != ID_No_ID)   // tag has an Id
      {
        out << ", Id = " << get_Id();
      }
   return out <<  ", }";
}
//-----------------------------------------------------------------------------
ostream &
Token::print_value(ostream & out) const
{
   switch(tag)
      {
        case TOK_VARIABLE:  return value.sym_ptr->print(out);
        case TOK_FUN0:      return out <<  "{ fun/0 }";
        case TOK_CHARACTER: return out << value.char_val;
        case TOK_INTEGER:   return out << value.int_val;
        case TOK_REAL:      return out << value.flt_val;

        case TOK_COMPLEX:   return out << value.complex_val.real << "J"
                                       << value.complex_val.imag;

        case TOK_APL_VALUE1:
        case TOK_APL_VALUE3:
             {
               Value_P v = value._apl_val();
               if (v->get_rank() == 0)   out << "''";
               loop(r, v->get_rank())
                   {
                     if (r)   out <<  " ";
                     out << v->get_shape_item(r);
                   }
               out << UNI_RHO;
               loop(e, v->element_count())
                 {
                   if (e == 4)
                      {
                        out << "...";
                        break;
                      }

                   const PrintContext pctx(PR_APL, 2, 0.02, 80);
                   out << UCS_string(v->get_ravel(e)
                                       .character_representation(pctx), 0);
                   out << " ";
                 }
               return out;
             }

        case TOK_QUAD_QUAD: return out << UNI_QUAD_QUAD;

        case TOK_QUAD_AI:
        case TOK_QUAD_AV:
        case TOK_QUAD_CT:
        case TOK_QUAD_EM:
        case TOK_QUAD_ET:
        case TOK_QUAD_FC:
        case TOK_QUAD_IO:
        case TOK_QUAD_LC:
        case TOK_QUAD_LX:
        case TOK_QUAD_L:
        case TOK_QUAD_NLT:
        case TOK_QUAD_PP:
        case TOK_QUAD_PR:
        case TOK_QUAD_PS:
        case TOK_QUAD_PW:
        case TOK_QUAD_R:
        case TOK_QUAD_RL:
        case TOK_QUAD_SVE:
        case TOK_QUAD_TZ:
        case TOK_QUAD_WA:   return print_quad(out);
      }

   out << "{ unknown value Token " << tag;

   if (get_Id() != ID_No_ID)   // tag has an Id
      {
        out << ", Id = " << get_Id();
      }

   return out <<  ", }";
}
//-----------------------------------------------------------------------------
ostream &
Token::print_quad(ostream & out) const
{
   return out << UNI_QUAD_QUAD << get_Id();
}
//-----------------------------------------------------------------------------
UCS_string
Token::canonical(PrintStyle style) const
{
UCS_string ucs;

   switch(get_Class())
      {
        case TC_ASSIGN:
             ucs.append(UNI_LEFT_ARROW);
             break;

        case TC_R_ARROW:
             ucs.append(UNI_RIGHT_ARROW);
             break;

        case TC_L_BRACK:
             if (get_tag() == TOK_L_BRACK)
                ucs.append(UNI_ASCII_L_BRACK);
             else if (get_tag() == TOK_SEMICOL)
                ucs.append(UNI_ASCII_SEMICOLON);
             else
                Assert(0);
                break;

        case TC_R_BRACK:
             ucs.append(UNI_ASCII_R_BRACK);
             break;

        case TC_END:
             ucs.append(UNI_DIAMOND);
             break;

        case TC_RETURN:                                                  break;
        case TC_LINE:
             ucs.append(UNI_ASCII_LF);
             break;

        case TC_VALUE:
             {
               PrintContext pctx(style, 16, 1.0E-18, 80);
               PrintBuffer pbuf(*get_apl_val(), pctx);
               if (pbuf.get_height() == 0)   return ucs;
               return pbuf.l1();
             }

        case TC_SYMBOL:
             ucs.append(get_sym_ptr()->get_name());
             break;


        case TC_FUN0:
        case TC_FUN12:
        case TC_R_PARENT:
        case TC_L_PARENT:
        case TC_OPER1:
        case TC_OPER2:
             if (get_Id() != ID_No_ID)       return id_name(get_Id());

             CERR << "Token: " << HEX4(tag) << " " << *this
                  << " at " << LOC << endl;
             FIXME;
             break;


        default:
             CERR << "Token: " << HEX4(tag) << " " << *this
                  << " at " << LOC << endl;
             Q((void *)get_Class())
             Backtrace::show(__FILE__, __LINE__);
             FIXME;
      }

   return ucs;
}
//-----------------------------------------------------------------------------
UCS_string
Token::tag_name() const
{
   switch(get_tag())
      {
#define TD(tag, _tc, _tv, _id) case tag: return UCS_string( #tag );
#include "Token.def"
      }

char cc[40];
   snprintf(cc, sizeof(cc), "0x%X", get_tag());
UCS_string ucs(cc);
   return ucs;
}
//-----------------------------------------------------------------------------
int
Token::error_info(UCS_string & ucs) const
{
UCS_string can = canonical(PR_APL_FUN).remove_pad();

const Unicode c1 = ucs.back();
const Unicode c2 = can.size() ? can[0] : Invalid_Unicode;

   // conditions when we don't need a space
   //
bool need_space = ! (Avec::no_space_after(c1) || Avec::no_space_before(c2));

   if (need_space)   ucs.append(UNI_ASCII_SPACE);

   ucs.append(can);
   return need_space ? -can.size() : can.size();
}
//-----------------------------------------------------------------------------
const char * 
Token::short_class_name(TokenClass cls)
{
   switch(cls)
      {
        case TC_INVALID:   return "INV";
        case TC_END:      return "END";
        case TC_RETURN:    return "RET";
        case TC_VALUE:     return "VAL";
        case TC_INDEX:     return "IDX";
        case TC_PINDEX:    return "PIDX";
        case TC_SYMBOL:    return "SYM";
        case TC_FUN0:      return "F0";
        case TC_FUN12:     return "F12";
        case TC_OPER1:     return "OP1";
        case TC_OPER2:     return "OP2";
        case TC_R_PARENT:  return ")";
        case TC_R_BRACK:   return "]";
        case TC_L_PARENT:  return "(";
        case TC_L_BRACK:   return "[/;";
        case TC_R_ARROW:   return "→";
        case TC_ASSIGN:    return "←";
        case TC_LINE:      return "LINE";
        case TC_VOID:      return "VOID";
        case TC_DIAMOND:   return "◊";
        case TC_NUMERIC:   return "NUMB";
        case TC_SPACE:     return "SPACE";
        case TC_NEWLINE:   return "LF";
        case TC_COLON:     return ":";
        case TC_QUOTE:     return "QUOTE";
        case TC_OFF:       return "OFF";
        case TC_SI_LEAVE:  return "LEAVE";
      }

   return "???";
}
//-----------------------------------------------------------------------------
void
Token::print_token_list(ostream & out, const Source<Token> & src)
{
const int len = src.rest();
   loop(t, len)   out << "`" << src[t] << "  ";
   out << endl;
}
//-----------------------------------------------------------------------------
const char *
Token::class_name(TokenClass tc)
{
#define tcn(x) case x: return #x;

   switch(tc)
      {
        tcn( TC_INVALID  )
        tcn( TC_END      )
        tcn( TC_RETURN   )
        tcn( TC_VALUE    )
        tcn( TC_INDEX    )
        tcn( TC_PINDEX   )
        tcn( TC_SYMBOL   )
        tcn( TC_FUN0     )
        tcn( TC_FUN12    )
        tcn( TC_OPER1    )
        tcn( TC_OPER2    )
        tcn( TC_R_PARENT )
        tcn( TC_R_BRACK  )
        tcn( TC_L_PARENT )
        tcn( TC_L_BRACK  )
        tcn( TC_R_ARROW  )
        tcn( TC_ASSIGN   )
        tcn( TC_LINE     )
        tcn( TC_VOID     )
        tcn( TC_DIAMOND  )
        tcn( TC_NUMERIC  )
        tcn( TC_SPACE    )
        tcn( TC_NEWLINE  )
        tcn( TC_COLON    )
        tcn( TC_QUOTE    )
        tcn( TC_OFF      )
        tcn( TC_SI_LEAVE )
      }

   return "*** Obscure token class ***";
}
//-----------------------------------------------------------------------------
inline void 
Token::copy_N(const Token & src)
{
   tag = src.tag;
   switch(src.get_ValueType())
      {
        case TV_NONE:  value.int_val    = 0;                              break;
        case TV_CHAR:  value.char_val   = src.value.char_val;             break;
        case TV_INT:   value.int_val    = src.value.int_val;              break;
        case TV_FLT:   value.flt_val    = src.value.flt_val;              break;
        case TV_CPX:   value.complex_val.real = src.value.complex_val.real;
                       value.complex_val.imag = src.value.complex_val.imag;
                                                                          break;
        case TV_SYM:   value.sym_ptr    = src.value.sym_ptr;              break;
        case TV_LIN:   value.fun_line   = src.value.fun_line;             break;
        case TV_VAL:   value._apl_val() = src.value._apl_val();           break;
        case TV_INDEX: value.index_val  = src.value.index_val;            break;
        case TV_FUN:   value.function   = src.value.function;             break;
        default:       FIXME;
      }
}
//-----------------------------------------------------------------------------
void
copy_1(Token & dst, const Token & src, const char * loc)
{
   dst.clear(loc);
   if (src.is_apl_val())
      {
        Value * val = src.get_apl_val_pointer();
        if (val)
           {
             ADD_EVENT(val, VHE_TokCopy1, src.value_use_count(), loc);
           }
        else
           {
             ADD_EVENT(val, VHE_TokCopy1, -1, loc);
           }
      }

   dst.copy_N(src);
}
//-----------------------------------------------------------------------------
void
move_1(Token & dst, Token & src, const char * loc)
{
   dst.clear(loc);
   dst.copy_N(src);

   if (src.is_apl_val())
      {
        Value * val = src.get_apl_val_pointer();
	ADD_EVENT(val, VHE_TokMove1, src.value_use_count() - 1, loc);
        src.clear(loc);
      }
}
//-----------------------------------------------------------------------------
void
move_2(Token & dst, const Token & src, const char * loc)
{
   dst.clear(loc);
   dst.copy_N(src);

   if (src.is_apl_val())
      {
        Value * val = src.get_apl_val_pointer();
        ADD_EVENT(val, VHE_TokMove2, src.value_use_count() - 1, loc);
        ((Token &)src).clear(loc);
      }
}
//-----------------------------------------------------------------------------
