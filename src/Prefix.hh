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

#ifndef __PREFIX_HH_DEFINED__
#define __PREFIX_HH_DEFINED__

#include "Common.hh"
#include "PrintOperator.hh"
#include "Token.hh"

class Prefix;
class StateIndicator;
class Token_string;
struct ReduceArg;

/// the max. number of token in one reduction
enum { MAX_REDUCTION_LEN = 6 };

//-----------------------------------------------------------------------------
/// how to continue after return from a reduce_XXX() function
enum R_action
{
   /// repeat phrase matching with current stack
   RA_CONTINUE = 0,

   /// repeat phrase matching with current stack (stack is empty)
   RA_NEXT_STAT = 1,

   /// return from parser with result arg[0]
   RA_RETURN = 2,

   /// return from parser and continue in pushed SI
   RA_SI_PUSHED = 3,

   /// internal error: action was not set by reduce_XXX() function
   RA_FIXME  = 4,
};
//-----------------------------------------------------------------------------
/// a class for reducing all statements of an Executable
class Prefix
{
   friend class XML_Loading_Archive;
   friend class XML_Saving_Archive;

public:
   /// constructor
   Prefix(StateIndicator & _si, const Token_string & _body);

   /// destructor
   void cleanup();

   /// max. number of lookahead token
   enum { MAX_CONTENT   = 3*MAX_REDUCTION_LEN,
          MAX_CONTENT_1 = MAX_CONTENT - 1 };

   /// print the state of this parser
   void print(ostream & out, int indent) const;

   /// throw an E_LEFT_SYNTAX_ERROR or an E_SYNTAX_ERROR
   void syntax_error(const char * loc);

   /// prevent or allow erase() of values on vstacks
   void lock_values(bool lock);

   /// clear the mark flag of all values in \b this Prefix
   void unmark_all_values() const;

   /// print all owners of \b value
   int show_owners(const char * prefix, ostream & out, const Value & value) const;

   /// highest PC in current statement
   Function_PC get_range_high() const;

   /// lowest PC in current statement
   Function_PC get_range_low() const;

   /// print the current range (PC from - PC to) on out
   void print_range(ostream & out) const;

   /// replace the left or right argument of a function (for retry) and
   /// return true on success
   bool replace_AB(Value_P old_value, Value_P new_value);

   /// execute one context (user defined function or operator, execute,
   /// or immediate execution)
   Token reduce_statements();

   /// return the number of token currently in the FIFO
   int size() const
      { return put; }

   /// return true if there is space left in the FIFO
   bool has_space() const
      { return size() < MAX_CONTENT_1; }

   /// return the leftmost token (e.g. A in A←B)
   const Token & at0() const
      { Assert1(size() > 0);   return content[put - 1].tok; }

   /// return the leftmost token (e.g. A in A←B)
   Token & at0()
      { Assert1(size() > 0);   return content[put - 1].tok; }

   /// return the second token from the left (e.g. ← in A←B)
   Token & at1()
      { Assert1(size() > 1);   return content[put - 2].tok; }

   /// return the third token from the left (e.g. B in A←B)
   Token & at2()
      { Assert1(size() > 2);   return content[put - 3].tok; }

   /// return the fourth token from the left (e.g. B in A+/B)
   Token & at3()
      { Assert1(size() > 3);   return content[put - 4].tok; }

   enum Assign_state
     {
       ASS_none       = 0,   ///< no assignment (right of ←)
       ASS_arrow_seen = 1,   ///< ← seen but no variable yet
       ASS_var_seen   = 2,   ///< var and ← seen
     };

   /// return the current assignment state
   Assign_state get_assign_state() const
      { return assign_state; }

   /// set the current assignment state
   void set_assign_state(Assign_state new_state)
      { assign_state = new_state; }

   /// return the highest PC seen in the current statement
   Function_PC get_lookahead_high() const
      { return lookahead_high; }

   /// set the highest PC seen in the current statement
   void set_lookahead_high(Function_PC lah)
      { lookahead_high = lah; }

   /// read one more token (don't store yet)
   Token_loc lookahead()
      {
        const Function_PC old_pc = PC++;
        if (old_pc < body.size())   return Token_loc(body[old_pc], old_pc);
        return Token_loc(Token(), --PC);   // end of function
      }

   /// store one more token
   void push(const Token_loc & tl)
      {
        Assert1(has_space());

        content[put++].copy(tl, LOC);
      }

   /// pop \b prefix_len items and push \b result
   void pop_args_push_result(const Token & result)
        {
          Assert1(size() >= prefix_len);
          copy_1(content[put - prefix_len].tok, result, LOC);
          content[put - prefix_len].pc = content[put - 1].pc;
          put -= prefix_len - 1;
        }

   /// remove the leftmost token (e,g, A in A←B) from the stack and return it
   Token_loc & pop()
      {  Assert1(size() > 0);   return content[--put]; }

   /// discard the leftmost token (e,g, A in A←B) from the stack and return it
   void pop_and_discard()
      {  Assert1(size() > 0);   --put; }

   /// the number of TOK_LSYMB2 tokens ahead (excluding the leftmost one
   /// already read into content)
   int vector_ass_count() const;

   /// reset statement to empty state (e.g. after →N)
   void reset(const char * loc)
      { cleanup();   put = 0;   assign_state = ASS_none;
        lookahead_high = Function_PC_invalid;
        saved_lookahead.tok.clear(LOC); }

   /// print the current stack
   void print_stack(ostream & out, const char * loc) const;

   /// set a new PC
   void set_PC(Function_PC new_pc)
      { reset(LOC);   PC = new_pc; }

   /// return the current PC
   Function_PC get_PC() const
      { return PC; }

   /// set action according to (result-) Token type
   void set_action(const Token & result)
      {
        switch(result.get_Class())
           {
             case TC_VALUE:
             case TC_VOID:
             case TC_END:
             case TC_FUN2:
                  action = RA_CONTINUE;
                  return;

             case TC_RETURN:
                  // result was TOK_RETURN_EXEC, TOK_RETURN_STATS,
                  // TOK_RETURN_VOID, or TOK_RETURN_SYMBOL. The current
                  // context is xomplete and may or may not have produced
                  // a value.
                  //
                  action = RA_RETURN;
                  return;

             case TC_SI_LEAVE:
                  //
                  // result was TOK_SI_PUSHED or TOK_ERROR
                  if (result.get_tag() == TOK_ERROR)   action = RA_RETURN;
                  else                                 action = RA_SI_PUSHED;
                  return;

             default: CERR << "CLASS = " << result.get_Class()
                           << " at " << LOC << endl;
                      TODO;
           }
      }

   /// read and resolve the token class left of [ ... ], PC is at ]
   bool is_value_bracket() const;

   /// return the leftmost (top-of-stack) Token_loc (at put position)
   Token_loc & tos()
       { Assert1(put);   return content[put - 1]; }

   /// return the idx'th Token_loc (from put position)
   Token_loc & at(int idx)
       { Assert1(idx < put);   return content[put - idx - 1]; }

   /// return the idx'th Token_loc (from put position)
   const Token_loc & at(int idx) const
       { Assert1(idx < put);   return content[put - idx - 1]; }

protected:
   /// the StateIndicator that contains this parser
   StateIndicator & si;

#include "Prefix.def"

   /// put pointer (for the next token at PC)
   int put;

   /// the lookahead token (read but not yet reduced)
   Token_loc content[MAX_CONTENT];

   /// the X token (leftmost token in MISC phrase, if any)
   Token_loc saved_lookahead;

   /// the function body. This parser parses tokens body[pc_from] ...
   const Token_string & body;

   /// the current pc (+1)
   Function_PC PC;

   /// assignment state
   Assign_state assign_state;

   /// the highest PC seen in the current statement
   Function_PC lookahead_high;

   /// length of matched prefix (before calling a reduce_XXX() function)
   int prefix_len;

   /// the action to be taken after returning from a reduce_XXX() function
   R_action action;

   /// the phrase being reduced (only valid if a phrase was matched)
   const Phrase * best;
};
//-----------------------------------------------------------------------------

#endif // __PREFIX_HH_DEFINED__
