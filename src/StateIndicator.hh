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

#ifndef __STATE_INDICATOR_HH_DEFINED__
#define __STATE_INDICATOR_HH_DEFINED__

#include "Common.hh"
#include "DerivedFunction.hh"
#include "EOC_handler_args.hh"
#include "Error.hh"
#include "Parser.hh"
#include "Prefix.hh"
#include "PrintOperator.hh"

class Executable;

//-----------------------------------------------------------------------------
/**
    One entry of the state indicator (SI) of the APL interpreter.
    Compared to e.g.  C++, the state indicator is (one element of) the
    function call stack of the interpreter.
 */
class StateIndicator
{
   friend class XML_Loading_Archive;
   friend class XML_Saving_Archive;

public:
   /// constructor
   StateIndicator(const Executable * exec, StateIndicator * _par);

   /// destructor
   ~StateIndicator();

   /// clear all eval_arg_XXX variables
   void clear_args(const char * loc);

   /// set eval_arg_XXX pointers and remember arg bits set
   void set_args(Token * A, Token & F, Token * X, Token * B);

   /// continue this StateIndicator (to line N after →N back into it)
   void goon(Function_Line N, const char * loc);

   /// retry this StateIndicator (after →'')
   void retry(const char * loc);

   /// Return the function name, or "*" for an immediate execution context
   UCS_string function_name() const;

   /// remove the stack entry (for command )SIC)
   void clear(ostream & out);

   /// list the stack entry (for commands ]SI, )SI, and )SIS)
   void list(ostream & out, SI_mode mode) const;

   /// list the stack entry (for command ]SI)
   void print(ostream & out) const;

   /// print spaces according to level
   ostream & indent(ostream & out) const;

   /// return pointer to the current user function, statements, or execute
   const Executable * get_executable() const
      { return executable; }

   /// return the name of the parse mode
   Unicode get_parse_mode_name() const;

   /// return the current PC
   Function_PC get_PC() const
      { return current_stack.get_PC(); }

   /// set the current PC
   void set_PC(Function_PC new_pc)
      { current_stack.set_PC(new_pc); }

   /// evaluate a →N statement. Update pc return true iff context has changed
   Token jump(Value_P val);

   /// return the nesting level (oldest SI has level 0, next has level 1, ...)
   int get_level() const   { return level; };

   /// return the current line number
   Function_Line get_line() const;

   /// Maybe print B (according to tag) and erase B
   void statement_result(Token & result);

   /// update error information in \b error and copy it to SI
   void update_error_info(Error & err);

   /// Escape from \b user function (exit from each invocation until
   /// immediate execution is reached)
   void escape();

   /// execute token in body...
   Token run();

   /// clear the marked bit in all parsers
   void unmark_all_values() const;

   /// print all owners of \b value
   int show_owners(ostream & out, const Value & value) const;

   /// print a short debug info
   void info(ostream & out, const char * loc) const;

   /// return the error related info in this context
   Error & get_error() { return error; }

   /// return the error related info in this context
   const Error & get_error() const { return error; }

   /// return eval_arg_L if eval_arg_F is a primitive function
   Value_P get_L() const;

   /// change eval_arg_L
   void set_L(Value_P value);

   /// return eval_arg_X if eval_arg_F is a primitive function
   Value_P get_X() const;

   /// change eval_arg_X
   void set_X(Value_P value);

   /// return eval_arg_R if eval_arg_F is a primitive function
   Value_P get_R() const;

   /// change eval_arg_R
   void set_R(Value_P value);

   /// replace all occurences of old_value int his SI by new_value
   bool replace_arg(Value_P old_value, Value_P new_value);

   /// clean up after some eval_XXX() function has thrown an error
   void recover_from_error(const char * loc);

   /// call eoc_handler(). Return false if there is none or else the result of
   /// the eoc_handler.
   bool call_eoc_handler(Token & token)
      {
        if (eoc_handler)
           {
             EOC_HANDLER eoc = eoc_handler;
             eoc_handler = 0;   // clear handler
             return eoc(token, eoc_arg);   // false: success, true: retry
           }

        return false;   // no eoc_handler
      }

   /// set the eoc handler
   void set_eoc_handler(EOC_HANDLER handler)
      { eoc_handler = handler; }

   /// return the eoc handler argument
   EOC_arg & get_eoc_arg()
      { return eoc_arg; }

   /// return safe_execution mode
   bool get_safe_execution() const
      { return safe_execution; }

   /// set safe_execution mode
   void set_safe_execution(bool on_off)
      { safe_execution = on_off; }

   /// call fun with no arguments
   Token eval_(Token & fun);

   /// call fun with argument B, remembering args on error.
   Token eval_B(Token & fun, Token & B);

   /// call fun with arguments A and B, remembering args on error.
   Token eval_AB(Token & A, Token & fun, Token & B);

   /// call axis fun with arguments A and B, axix X, remembering args on error.
   Token eval_XB(Token & fun, Token & X, Token & B);

   /// call axis fun with arguments A and B, axix X, remembering args on error.
   Token eval_AXB(Token & A, Token & fun, Token & X, Token & B);

   /// call monadic operator with function L and argument B, remembering args
   Token eval_LB(Token & LO, Token & oper, Token & B);

   /// call monadic operator with function L and arguments A and B
   Token eval_ALB(Token & A, Token & LO, Token & oper, Token & B);

   /// call monadic axis operator with function L and argument B
   Token eval_LXB(Token & LO, Token & oper, Token & X, Token & B);

   /// call monadic axis operator with function L and arguments A and B
   Token eval_ALXB(Token & A, Token & LO, Token & oper, Token & X, Token & B);

   /// call dyadic operator with functions L and R, and argument B
   Token eval_LRB(Token & LO, Token & oper, Token & RO, Token & B);

   /// call dyadic operator with functions L and R, and arguments A and B
   Token eval_ALRB(Token & A, Token & LO, Token & oper,
                   Token & RO, Token & B);

   /// call dyadic axis operator with functions L and R, and argument B
   Token eval_LRXB(Token & LO, Token & oper,
                   Token & RO, Token & X, Token & B);

   /// call dyadic axis operator with functions L and R, and arguments A and B
   Token eval_ALRXB(Token & A, Token & LO, Token & oper,
                    Token & RO, Token & X, Token & B);

   /// get the current prefix parser
   Prefix & get_prefix()
      { return current_stack; }

   /// return the SI that has called this one
   StateIndicator * get_parent() const
      { return parent; }

   /// a small storage for DerivedFunction objects.
  DerivedFunctionCache fun_oper_cache;

protected:
   /// the user function that is being executed
   const Executable * executable;

   /// true iff this context is in safe execution mode (⎕EA, ⎕EC)
   bool safe_execution;

   /// an optional function that is called at the end of execution of this
   /// context. Used by ⎕EA and ⎕EC
   EOC_HANDLER eoc_handler;

   /// the left argument for eoc_handler()
   EOC_arg eoc_arg;

   /// The nesting level (of sub-executions)
   const int level;

   /// details of the last error in this context.
   Error error;

   /// the current-stack of this context.
   Prefix current_stack;

   /// the left eval_XXX argument (0 if none)
   Value_P eval_arg_A;

   /// the axis argument of eval_XXX (0 if none)
   Value_P eval_arg_X;

   /// the eval_XXX function (or operator) argument
   Function * eval_arg_F;

   /// the right eval_XXX argument (0 if none)
   Value_P eval_arg_B;

   /// the StateIndicator that has called this one
   StateIndicator * parent;
};
//-----------------------------------------------------------------------------

#endif // __STATE_INDICATOR_HH_DEFINED__
