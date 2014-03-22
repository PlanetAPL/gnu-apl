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

#include "CharCell.hh"
#include "ComplexCell.hh"
#include "Common.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "Output.hh"
#include "Parser.hh"
#include "PointerCell.hh"
#include "PrintOperator.hh"
#include "Symbol.hh"
#include "SystemLimits.hh"
#include "SystemVariable.hh"
#include "Value.hh"
#include "Tokenizer.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
ErrorCode
Parser::parse(const UCS_string & input, Token_string & tos)
{
   // convert input characters into token
   //
Token_string tos1;

   {
     Tokenizer tokenizer(pmode, LOC);
     ErrorCode ec = tokenizer.tokenize(input, tos1);
     if (ec != E_NO_ERROR)   return ec;
   }

   Log(LOG_parse)
      {
        CERR << "parse 1 [" << tos1.size() << "]: ";
        print_token_list(CERR, tos1, 0);
        CERR << endl;
      }

   // split tos1 into statements.
   //
vector<Token_string *> statements;
   {
     Source<Token> src(tos1);
     Token_string  * stat = new Token_string();
     while (src.rest())
        {
          const Token tok = src.get();
          if (tok.get_tag() == TOK_DIAMOND)
             {
               statements.push_back(stat);
               stat = new Token_string();
             }
          else
             {
               stat->append(tok, LOC);
             }
        }
     statements.push_back(stat);
   }

   loop(s, statements.size())
      {
        Token_string * stat = statements[s];
        ErrorCode err = parse_statement(*stat);
        if (err)   return err;

        if (s)   tos.append(Token(TOK_DIAMOND));

        loop(t, stat->size())
           {
             tos.append(Token());
             move_1(tos[tos.size() - 1], (*stat)[t], LOC);
           }
        delete stat;
      }

   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
Parser::parse_statement(Token_string & tos)
{
   // 1. convert (X) into X and ((X...)) into (X...)
   //
   remove_nongrouping_parantheses(tos);
   remove_void_token(tos);

   Log(LOG_parse)
      {
        CERR << "parse 2 [" << tos.size() << "]: ";
        print_token_list(CERR, tos, 0);
      }

   // 2. convert groups like '(' val val...')' into single APL values
   // A group is defined as 2 or more values enclosed in '(' and ')'.
   // For example: (1 2 3) or (4 5 (6 7) 8 9)
   //
   for (int progress = true; progress;)
      {
        progress = collect_groups(tos);
        if (progress)   remove_void_token(tos);
      }

   Log(LOG_parse)
      {
        CERR << "parse 3 [" << tos.size() << "]: ";
        print_token_list(CERR, tos, 0);
      }

   // 3. convert vectors like 1 2 3 or '1' 2 '3' into single APL values
   //
   collect_constants(tos);
   remove_void_token(tos);

   Log(LOG_parse)
      {
        CERR << "parse 4 [" << tos.size() << "]: ";
        print_token_list(CERR, tos, 0);
      }

   // 4. mark symbol left of ← as LSYMB
   //
   mark_lsymb(tos);
   Log(LOG_parse)
      {
        CERR << "parse 5 [" << tos.size() << "]: ";
        print_token_list(CERR, tos, 0);
      }

   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
void
Parser::collect_constants(Token_string & tos)
{
   Log(LOG_collect_constants)
      {
        CERR << "collect_constants [" << tos.size() << " token] in: ";
        print_token_list(CERR, tos, 0);
      }

   // convert several items in vector notation into a single APL value
   //
   loop (t, tos.size())
      {
        int32_t to;
        switch(tos[t].get_tag())
           {
             case TOK_CHARACTER:
             case TOK_INTEGER:
             case TOK_REAL:
             case TOK_COMPLEX:
             case TOK_APL_VALUE1:
             case TOK_APL_VALUE3:
                  break;          // continue below

             default: continue;   // nex token
           }

        // at this point, t is the first item. Collect subsequenct items.
        //
        for (to = t + 1; to < tos.size(); ++to)
            {
#if 1
              // Note: ISO 13751 gives an example with 1 2 3[2] ↔ 2
              // 
              // In contrast, the binding rules stated in IBM's apl2lrm.pdf
              // would give 3[2] ↔ RANK ERROR.
              // 
              // We follow apl2lrm; to enable IBM behavior write #if 0 above.
              // 

              // if tos[to + 1] is [ then [ binds stronger than
              // vector notation and we stop collecting.
              //
              if ((to + 1) < tos.size() && tos[to + 1].get_tag() == TOK_L_BRACK)
                 break;
#endif
              const TokenTag tag = tos[to].get_tag();

              if (tag == TOK_CHARACTER)    continue;
              if (tag == TOK_INTEGER)      continue;
              if (tag == TOK_REAL)         continue;
              if (tag == TOK_COMPLEX)      continue;
              if (tag == TOK_APL_VALUE1)   continue;
              if (tag == TOK_APL_VALUE3)   continue;

              // no more values: stop collecting
              //
              break;
            }

        create_value(tos, t, to - t);
      }

   Log(LOG_collect_constants)
      {
        CERR << "collect_constants [" << tos.size() << " token] out: ";
        print_token_list(CERR, tos, 0);
      }
}
//-----------------------------------------------------------------------------
bool
Parser::collect_groups(Token_string & tos)
{
   Log(LOG_collect_constants)
      {
        CERR << "collect_groups [" << tos.size() << " token] in: ";
        print_token_list(CERR, tos, 0);
      }

int opening = -1;
   loop(t, tos.size())
       {
         switch(tos[t].get_tag())
            {
              case TOK_CHARACTER:
              case TOK_INTEGER:
              case TOK_REAL:
              case TOK_COMPLEX:
              case TOK_APL_VALUE1:
              case TOK_APL_VALUE3:
                   continue;

              case TOK_L_PARENT:
                   // we remember the last opening '(' so that we know where
                   // the group started when we see a ')'. This causes the
                   // deepest ( ... ) to be grouped first.
                   opening = t;
                   continue;

              case TOK_R_PARENT:
                   if (opening == -1)   continue;   // ')' without ')'

                   tos[opening].clear(LOC);  // invalidate '('
                   tos[t].clear(LOC);        // invalidate ')'
                   create_value(tos, opening + 1, t - opening - 1);

                   // we removed parantheses, so we close the value.
                   //
                   if (tos[opening + 1].get_tag() == TOK_APL_VALUE3)
                      tos[opening + 1].ChangeTag(TOK_APL_VALUE1);

                   Log(LOG_collect_constants)
                      {
                        CERR << "collect_groups [" << tos.size()
                             << " token] out: ";
                        print_token_list(CERR, tos, 0);
                      }
                   return true;

              default: // nothing group'able
                   opening = -1;
                   continue;
            }
       }

   return false;
}
//-----------------------------------------------------------------------------
int32_t
Parser::find_closing_bracket(Token_string & tos, int32_t pos)
{
   Assert(tos[pos].get_tag() == TOK_L_BRACK);

int others = 0;

   for (int32_t p = pos + 1; p < tos.size(); ++p)
       {
         Log(LOG_find_closing)
            CERR << "find_closing_bracket() sees " << tos[p] << endl;

         if (tos[p].get_tag() == TOK_R_BRACK)
            {
             if (others == 0)   return p;
             --others;
            }
         else if (tos[p].get_tag() == TOK_R_BRACK)   ++others;
       }

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
int32_t
Parser::find_opening_bracket(Token_string & tos, int32_t pos)
{
   Assert(tos[pos].get_tag() == TOK_R_BRACK);

int others = 0;

   for (int32_t p = pos - 1; p >= 0; --p)
       {
         Log(LOG_find_closing)
            CERR << "find_opening_bracket() sees " << tos[p] << endl;

         if (tos[p].get_tag() == TOK_L_BRACK)
            {
             if (others == 0)   return p;
             --others;
            }
         else if (tos[p].get_tag() == TOK_R_BRACK)   ++others;
       }

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
int32_t
Parser::find_closing_parent(Token_string & tos, int32_t pos)
{
   Assert1(tos[pos].get_Class() == TC_L_PARENT);

int others = 0;

   for (int32_t p = pos + 1; p < tos.size(); ++p)
       {
         Log(LOG_find_closing)
            CERR << "find_closing_bracket() sees " << tos[p] << endl;

         if (tos[p].get_Class() == TC_R_PARENT)
            {
             if (others == 0)   return p;
             --others;
            }
         else if (tos[p].get_Class() == TC_R_PARENT)   ++others;
       }

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
int32_t
Parser::find_opening_parent(Token_string & tos, int32_t pos)
{
   Assert(tos[pos].get_Class() == TC_R_PARENT);

int others = 0;

   for (int32_t p = pos - 1; p >= 0; --p)
       {
         Log(LOG_find_closing)
            CERR << "find_opening_bracket() sees " << tos[p] << endl;

         if (tos[p].get_Class() == TC_L_PARENT)
            {
             if (others == 0)   return p;
             --others;
            }
         else if (tos[p].get_Class() == TC_R_PARENT)   ++others;
       }

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
void
Parser::remove_nongrouping_parantheses(Token_string & tos)
{
   //
   // 1. replace ((X...)) by: (X...)
   // 2. replace (X)      by: X for a single token X,
   //
   for (bool progress = true; progress; progress = false)
       {
         loop(t, int(tos.size()) - 2)
             {
               if (tos[t].get_Class() != TC_L_PARENT)   continue;

               const int closing = find_closing_parent(tos, t);

               // check for case 1.
               //
               if (tos[t + 1].get_Class() == TC_L_PARENT)
                  {
                    // tos[t] is (( ...
                    //
                    const int closing_1 = find_closing_parent(tos, t + 1);
                    if (closing == (closing_1 + 1))
                       {
                         // tos[closing_1] is )) ...
                         // We have case 1. but not for example ((...)(...))
                         // remove redundant tos[t] and tos[closing] because
                         // ((...)) are not "not separating"
                         //
                         progress = true;
                         tos[t].clear(LOC);
                         tos[closing].clear(LOC);
                         continue;
                       }
                  }

               // check for case 2.
               //
               if (closing != (t + 2))   continue;


               // case 2. We have tos[t] = ( X ) ...
               //
               // (X) : "not grouping" if X is a skalar. 
               // If X is non-skalar, enclose it
               //
               progress = true;
               move_1(tos[t + 2], tos[t + 1], LOC);
               tos[t + 1].clear(LOC);
               tos[t].clear(LOC);
               ++t;   // skip tos[t + 1]
             }
       }
}
//-----------------------------------------------------------------------------
void
Parser::remove_void_token(Token_string & tos)
{
uint32_t dst = 0;

   loop(src, tos.size())
       {
         if (tos[src].get_tag() == TOK_VOID)   continue;
         if (src != dst)   move_1(tos[dst], tos[src], LOC);
         ++dst;
       }

   tos.shrink(dst);
}
//-----------------------------------------------------------------------------
void
Parser::create_value(Token_string & tos, uint32_t pos, uint32_t count)
{
   Log(LOG_create_value)
      {
        CERR << "create_value(" << __LINE__ << ") tos[" << tos.size()
             <<  "]  pos " << pos << " count " << count << " in:";
        print_token_list(CERR, tos, 0);
      }

   if (count == 1)   create_skalar_value(tos[pos]);
   else              create_vector_value(tos, pos, count);

   Log(LOG_create_value)
      {
        CERR << "create_value(" << __LINE__ << ") tos[" << tos.size()
             <<  "]  pos " << pos << " count " << count << " out:";
        print_token_list(CERR, tos, 0);
      }
}
//-----------------------------------------------------------------------------
void
Parser::create_skalar_value(Token & output)
{
   switch(output.get_tag())
      {
        case TOK_CHARACTER:
             {
               Value_P skalar(new Value(LOC));

               new (&skalar->get_ravel(0))  CharCell(output.get_char_val());
               skalar->check_value(LOC);
               Token tok(TOK_APL_VALUE3, skalar);
               move_1(output, tok, LOC);
             }
             return;

        case TOK_INTEGER:
             {
               Value_P skalar(new Value(LOC));

               new (&skalar->get_ravel(0))   IntCell(output.get_int_val());
               skalar->check_value(LOC);
               Token tok(TOK_APL_VALUE3, skalar);
               move_1(output, tok, LOC);
             }
             return;

        case TOK_REAL:
             {
               Value_P skalar(new Value(LOC));

               new (&skalar->get_ravel(0))  FloatCell(output.get_flt_val());
               skalar->check_value(LOC);
               Token tok(TOK_APL_VALUE3, skalar);
               move_1(output, tok, LOC);
             }
             return;

        case TOK_COMPLEX:
             {
               Value_P skalar(new Value(LOC));

               new (&skalar->get_ravel(0))   ComplexCell(output.get_cpx_real(),
                                                         output.get_cpx_imag());
               skalar->check_value(LOC);
               Token tok(TOK_APL_VALUE3, skalar);
               move_1(output, tok, LOC);
             }
             return;

        case TOK_APL_VALUE1:
        case TOK_APL_VALUE3:
             return;
      }

   CERR << "Unexpected token " << output.get_tag() << ": " << output << endl;
   Assert(0 && "Unexpected token");
}
//-----------------------------------------------------------------------------
void
Parser::create_vector_value(Token_string & tos, uint32_t pos, uint32_t count)
{
Value_P vector(new Value(count, LOC));

   loop(l, count)
       {
         Cell * addr = &vector->get_ravel(l);
         Token & tok = tos[pos + l];

         switch(tok.get_tag())
            {
              case TOK_CHARACTER:
                   new (addr) CharCell(tok.get_char_val());
                   tok.clear(LOC);   // invalidate token
                   break;

              case TOK_INTEGER:
                   new (addr) IntCell(tok.get_int_val());
                   tok.clear(LOC);   // invalidate token
                   break;

              case TOK_REAL:
                   new (addr) FloatCell(tok.get_flt_val());
                   tok.clear(LOC);   // invalidate token
                   break;

              case TOK_COMPLEX:
                   new (addr) ComplexCell(tok.get_cpx_real(),
                                          tok.get_cpx_imag());
                   tok.clear(LOC);   // invalidate token
                   break;

            case TOK_APL_VALUE1:
            case TOK_APL_VALUE3:
                 new (addr) PointerCell(tok.get_apl_val());
                 tok.clear(LOC);   // invalidate token
                 break;
            }
       }

   vector->check_value(LOC);
Token tok(TOK_APL_VALUE3, vector);

   move_1(tos[pos], tok, LOC);

   Log(LOG_create_value)
      {
        CERR << "create_value [" << tos.size() << " token] out: ";
        print_token_list(CERR, tos, 0);
      }
}
//-----------------------------------------------------------------------------
void
Parser::mark_lsymb(Token_string & tos)
{
   loop(ass, tos.size())
      {
        if (tos[ass].get_tag() != TOK_ASSIGN)   continue;

        // found ←. move backwards. Before that we handle the special case of
        // vector specification, i.e. (SYM SYM ... SYM) ← value
        //
        if (ass >= 3 && tos[ass - 1].get_Class() == TC_R_PARENT &&
                        tos[ass - 2].get_Class() == TC_SYMBOL &&
                        tos[ass - 3].get_Class() == TC_SYMBOL)
           {
             // first make sure that this is really a vector specification.
             //
             const int syms_to = ass - 2;
             int syms_from = ass - 3;
             bool is_vector_spec = true;
             for (int a1 = ass - 4; a1 >= 0; --a1)
                 {
                   if (tos[a1].get_Class() == TC_L_PARENT)
                      {
                        syms_from = a1 + 1;
                        break;
                      }
                   if (tos[a1].get_Class() == TC_SYMBOL)     continue; 
                   is_vector_spec = false;
                   break;
                 }

             // if this is a vector specification, then mark all symbols
             // inside ( ... ) as TOK_LSYMB2.
             //
             if (is_vector_spec)
                {
                  for (int a1 = syms_from; a1 <= syms_to; ++a1)
                      tos[a1].ChangeTag(TOK_LSYMB2);
                }

             continue;
           }

        int bracks = 0;
        for (int prev = ass - 1; prev >= 0; --prev)
            {
              switch(tos[prev].get_Class())
                 {
                   case TC_R_BRACK: ++bracks;     break;
                   case TC_L_BRACK: --bracks;     break;
                   case TC_SYMBOL:  if (bracks)   break;   // inside [ ... ]
                                    if (tos[prev].get_tag() == TOK_SYMBOL)
                                       {
                                         tos[prev].ChangeTag(TOK_LSYMB);
                                       }
                                    // it could be that a system variable
                                    // is assigned, e.g. ⎕SVE←0. In that case
                                    // (actually always) we simply stop here.
                                    //

                                    /* fall-through */

                   case TC_END:     prev = 0;   // stop prev loop
                                    break;
                 }
            }
      }
}
//-----------------------------------------------------------------------------
void
Parser::print_token_list(ostream & out, const Token_string & tos, uint32_t from)
{
   loop(t, tos.size() - from)   out << "`" << tos[from + t] << "  ";

   out << endl;
}
//-----------------------------------------------------------------------------
