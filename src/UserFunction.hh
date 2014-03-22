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

#ifndef __USERFUNCTION_HH_DEFINED__
#define __USERFUNCTION_HH_DEFINED__

#include <vector>
#include <sys/types.h>

#include "Executable.hh"
#include "Function.hh"
#include "Error.hh"

//-----------------------------------------------------------------------------
/**
    A user-defined function.
 */
class UserFunction : public Function, public Executable
{
public:
   /// Construct a user defined function
   UserFunction(const UCS_string txt, int & error_line, bool keep_existing,
                const char * loc);

   /// Destructor.
   ~UserFunction();

   /// overloaded Executable::get_ufun()
   virtual const UserFunction * get_ufun() const
   { return this; }

   /// overloaded Function::get_ufun1()
   virtual const UserFunction * get_ufun1() const
   { return this; }

   /// overloaded Executable::get_ufun()
   virtual UserFunction * get_ufun()
   { return this; }

   /// overloaded Function::get_fun_valence()
   virtual int get_fun_valence() const
      {
        if (sym_A)    return 2;   // dyadic function
        if (sym_B)    return 1;   // monadic function
        return 0;                 // niladic function
      }

   /// overloaded Function::get_oper_valence()
   virtual int get_oper_valence() const
      {
        if (sym_RO)   return 2;   // dyadic operator
        if (sym_LO)   return 1;   // monadic operator
        return 0;                 // niladic function
      }

   /// overloaded Function::has_result()
   virtual int has_result() const
      { return sym_Z ? 1 : 0; }

   /// overloaded Function::get_exec_properties()
   virtual const int * get_exec_properties() const
      { return exec_properties; } 

   /// set execution properties (as per A ⎕FX)
   void set_exec_properties(const int * props)
      { loop(i, 4)   exec_properties[i] = props[i]; }

   /// overloaded Executable::print()
   virtual ostream & print(ostream & out) const;

   /// return the name of this function, or ◊ or ⍎
   virtual const UCS_string & get_name() const;

   /// return e.g. 'FOO[10]' 
   UCS_string get_name_and_line(Function_PC pc) const;

   /// Overloaded Function::print_properties()
   virtual void print_properties(ostream & out, int indent) const;

   /// print the local variables for command )SINL
   void print_local_vars(ostream & out) const;

   /// )SAVE this function in the workspace named \b workspace
   /// (in the file system).
   void save(const char * workspace, const char * function);

   /// )LOAD this function into the workspace named \b workspace.
   /// Return a pounter to this newly created function (or 0 on error).
   static UserFunction * load(const char * workspace, const char * function);

   /// Overloaded Function::destroy()
   virtual void destroy();

   /// add a label
   void add_label(Symbol * sym, Function_Line line)
      {
        labVal label = { sym, line };
        label_values.push_back(label);
      }

   /// Load this function into the workspace named \b workspace.
   static void load(const char * workspace, const char * function,
                    UserFunction * & fun);

   /// create a user defined function according to \b data of length \b len
   /// in workspace \b w.
   static UserFunction * fix(const UCS_string & data, int & error_line,
                             bool keep_existing, const char * loc);

   /// return the pc of the first token in line l (valid line), or
   /// the pc of the last token in the function (invalid line)
   Function_PC pc_for_line(int l) const;
   
   /// Overloaded \b Function::is_operator.
   virtual bool is_operator() const;

   /// Overloaded Function::has_alpha()
   virtual bool has_alpha() const   { return true; }

   /// overloaded Executable::get_sym_FUN()
   virtual Symbol * get_sym_FUN()   const   { return sym_FUN; }

   /// overloaded Executable::get_sym_Z()
   virtual Symbol * get_sym_Z()   const   { return sym_Z; }

   /// return the parse context for this function
   virtual ParseMode get_parse_mode() const     { return PM_FUNCTION; }

   /// overloaded Executable::get_line()
   Function_Line get_line(Function_PC pc) const;

   /// Quad_CR of this function
   virtual UCS_string canonical(bool with_lines) const;

   /// overloaded Executable::line_start()
   virtual Function_PC line_start(Function_Line line) const
      { return line_starts[line]; }

   /// Parse the header line of \b this function (copy header_line)
   void parse_header_line(UCS_string header_line);

   /// compute lines 2 and 3 in \b error
   void set_locked_error_info(Error & error) const;

   /// pop all local vars, return Z
   Value_P pop_local_vars() const;

protected:
   /// overladed Function::may_push_SI()
   virtual bool may_push_SI() const   { return true; }

   /// Check that all function params, local vars. and labels are unique.
   void check_duplicate_symbols();

   /// Check that sym occurs at most once in \b Symbol array \b sym.
   void check_duplicate_symbol(Symbol * sym);

   /// push Z (if defined), local variables, and labels.
   void eval_common();

   /// Overloaded Function::eval_()
   virtual Token eval_();

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   /// Overloaded Function::eval_LB.
   virtual Token eval_LB(Token & LO, Value_P B);

   /// Overloaded Function::eval_LXB()
   virtual Token eval_LXB(Token & LO, Value_P X, Value_P B);

   /// Overloaded Function::eval_ALB.
   virtual Token eval_ALB(Value_P A, Token & LO, Value_P B);

   /// Overloaded Function::eval_ALXB()
   virtual Token eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B);

   /// Overloaded Function::eval_LRB()
   virtual Token eval_LRB(Token & LO, Token & RO, Value_P B);

   /// Overloaded Function::eval_LRXB()
   virtual Token eval_LRXB(Token & LO, Token & RO, Value_P X, Value_P B);

   /// Overloaded Function::eval_ALRB()
   virtual Token eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B);

   /// Overloaded Function::eval_ALRXB()
   virtual Token eval_ALRXB(Value_P A, Token & LO, Token & RO,
                            Value_P X, Value_P B);

   /// helper function to print token with Function or Value content
   static ostream & print_val_or_fun(ostream & out, Token & tok);

   /// "[nn] " prefix
   static UCS_string line_prefix(Function_Line l);

   Symbol * sym_Z;     ///< optional result
   Symbol * sym_A;     ///< optional left function arg
   Symbol * sym_LO;    ///< optional left operator function
   Symbol * sym_FUN;   ///< mandatory function arg (function name)
   Symbol * sym_RO;    ///< optional right operator function
   Symbol * sym_X;    ///< optional right operator function axis
   Symbol * sym_B;     ///< optional right function arg

   /// The local variables of \b this function.
   vector<Symbol *> local_vars;

   /** Offsets to the first token in every line (for jumps).
       lines[0] points to the last line, which is automatically
       added and is TOK_RETURN_VOID for void functions
       and TOK_RETURN_VALUET for functions returning a value.

       An N line function:

       [0] R←A FOO B
       [1] L1: xxx
       ...
       [N-1]   yyy

      will, for example, have a N+1 element jump vector:

      [0] goto end of function        --------------+
      [1] L1: xxx                                   |
      ...                                           |
      [N-1]   yyy                                   |
      [N] TOK_RETURN_SYMBOL or TOK_RETURN_VOID   <--+

   **/
   vector<Function_PC> line_starts;

   /// A single label. A label is a local variable (sym) with an integer
   /// function line (in which sym: was specified as the first tokens
   /// in the line)
   struct labVal
   {
      Symbol      * sym;    ///< The symbol for the label variable.
      Function_Line line;   ///< The line number of the label.
   };

   /// The labels of \b this function.
   vector<labVal> label_values;

   /// execution properties as per 3⎕AT
   int exec_properties[4];
};
//-----------------------------------------------------------------------------

#endif // __USERFUNCTION_HH_DEFINED__
