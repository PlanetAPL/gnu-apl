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

#ifndef __WORKSPACE_HH_DEFINED__
#define __WORKSPACE_HH_DEFINED__

#include <vector>

#include "PrimitiveOperator.hh"
#include "QuadFunction.hh"
#include "Quad_RL.hh"
#include "Quad_SVx.hh"
#include "SkalarFunction.hh"
#include "Symbol.hh"
#include "SymbolTable.hh"
#include "SystemVariable.hh"
#include "UTF8_string.hh"

class Executable;
class StateIndicator;

//-----------------------------------------------------------------------------
/**
 The symbol table of the Workspace. We put it into a base class for Workspace,
 so that it is completely initialized before all other members of Workspace
 (in particular the ⎕-variables).
 **/
class Workspace_0
{
protected:
   /// the symbol table of this workspace.
   SymbolTable symbol_table;
};
//-----------------------------------------------------------------------------
/**
    An APL workspace. This structure contains everyting (variables, functions,
    SI stack, etc.) belonging to a single APL workspace.
 */
class Workspace : public Workspace_0
{
public:
   /// Construct an empty workspace.
   Workspace()
   : WS_name("CLEAR WS"),
//   prompt("-----> "),
     prompt("      "),
     top_SI(0)
   {}


   /// return the current QUAD-CT
   static APL_Float get_CT()
      { return the_workspace.v_quad_CT.current(); }

   /// set the current QUAD-CT
   static void set_CT(APL_Float new_CT)
      { the_workspace.v_quad_FC.current(); }

   /// Return element \b pos of the current QUAD-FC (pos should be 0..5)
   static APL_Char get_FC(int p)
      { return the_workspace.v_quad_FC.current()[p]; }

   /// Return the current QUAD-IO
   static APL_Integer get_IO()
      { return the_workspace.v_quad_IO.current(); }

   /// Return the current QUAD-LX
   static UCS_string get_LX()
      { return UCS_string(*the_workspace.v_quad_LX.get_apl_value()); }

   /// Return the current QUAD-PP
   static APL_Integer get_PP()
      { return the_workspace.v_quad_PP.current(); }

   /// Return the current QUAD-PR
   static const UCS_string & get_PR()
      { return the_workspace.v_quad_PR.current(); }

   /// Return the current QUAD-PS
   static PrintStyle get_PS()
      { return the_workspace.v_quad_PS.current(); }

   /// Return the current QUAD-PW.
   static APL_Integer get_PW()
      { return the_workspace.v_quad_PW.current(); }

   /// Return the QUAD-RL.
   static APL_Integer get_RL()
      { return the_workspace.v_quad_RL.get_random(); }

   /// the number of SI entries
   static int SI_entry_count()
      { return SI_top() ? (SI_top()->get_level() + 1) : 0; }

   /// the top of the SI stack (the SI pushed last)
   static StateIndicator * SI_top()
      { return the_workspace.top_SI; }

   /// the number of symbols in the symbol table
   static uint32_t symbols_allocated()
      { return the_workspace.symbol_table.symbols_allocated(); }

   /// copy all allocated symbols into \b table of size \b table_size
   static void get_all_symbols(Symbol ** table, uint32_t table_size)
      { return the_workspace.symbol_table.get_all_symbols(table, table_size); }

   /// lookup an existing user defined symbol. If not found, create one
   /// (unless this would be a quad symbol)
   static Symbol * lookup_symbol(const UCS_string & symbol_name)
      { return the_workspace.symbol_table.lookup_symbol(symbol_name);}

   /// clear ⎕EM and ⎕ET
   static void clear_error(const char * loc)
      { the_workspace.v_quad_EM.clear(loc);
        the_workspace.v_quad_ET.clear(loc); }

   /// update ⎕EM and ⎕ET according to \b error
   static void update_EM_ET(const Error & error)
      { the_workspace.v_quad_EM.update(error);
        the_workspace.v_quad_ET.update(error); }

   /// increase the wait time for user input as reported in ⎕AI
   static void add_wait(APL_time diff)
      { the_workspace.v_quad_AI.add_wait(diff); }

   /// return information in SI_top()
   static Error * get_error()
      { return SI_top() ? &SI_top()->get_error() : 0; }

   /// return reference to more info about last error
   static UCS_string & more_error()
      { return the_workspace.more_error_info; }

   /// erase the symbols in \b symbols from the symbol table
   static void erase_symbols(ostream & out, const vector<UCS_string> & symbols)
      { the_workspace.symbol_table.erase_symbols(out, symbols); }

   /// list all symbols (of category \b which) with names in \b from_to
   static void list_symbols(ostream & out, ListCategory which,
                     UCS_string from_to)
      { the_workspace.symbol_table.list_symbols(out, which, from_to); }

   /// list all symbols with names in \b buf
   static ostream & list_symbol(ostream & out, const UCS_string & buf)
      { return the_workspace.symbol_table.list_symbol(out, buf); }

   /// add \b ufun to list of that were ⎕EX'ed while on the SI stack
   static void add_expunged_function(const UserFunction * ufun)
      { the_workspace.expunged_functions.push_back(ufun); }

   /// return the symbol table of the current workspace.
   static const SymbolTable & get_symbol_table()
      { return the_workspace.symbol_table; }

   /// return the APL prompt
   static const UCS_string & get_prompt()
      { return the_workspace.prompt; }

   /// return the name of the current workspace.
   static const UCS_string & get_WS_name()
      { return the_workspace.WS_name; }

   /// set the name of the current workspace.
   static void set_WS_name(const UCS_string & new_name)
      { the_workspace.WS_name = new_name; }

   /// Create a new SI-entry on the SI stack.
   static void push_SI(const Executable * fun, const char * loc);

   /// Remove the current SI-entry from the SI stack.
   static void pop_SI(const char * loc);

   /// create and execute one immediate execution context
   // (leave with TOK_ESCAPE)
   static Token immediate_execution(bool exit_on_error);

   /// clear the workspace
   static void clear_WS(ostream & out, bool silent);

   /// clear the SI
   static void clear_SI(ostream & out);

   /// print the SI on \b out
   static void list_SI(ostream & out, SI_mode mode);

   /// the topmost SI with parse mode PM_FUNCTION
   static StateIndicator * SI_top_fun();

   /// the topmost SI with an error
   static const StateIndicator * SI_top_error();

   /// lookup an existing name (user defined or ⎕xx, var or function).
   /// return 0 if not found.
   static NamedObject * lookup_existing_name(const UCS_string & name);

   /// lookup an existing symbol (user defined or ⎕xx).
   static Symbol * lookup_existing_symbol(const UCS_string & symbol_name);

   /// save this workspace
   static void save_WS(ostream & out, vector<UCS_string> & lib_ws);

   /// set or inquire the workspace ID
   static void wsid(ostream & out, UCS_string arg);

   /// load \b lib_ws into the_workspace. Return ⎕LX of the new WS.
   static UCS_string load_WS(ostream & out, const vector<UCS_string> & lib_ws);

   /// copy objects from another workspace
   static void copy_WS(ostream & out, vector<UCS_string> & lib_ws,
                       bool protection);

   /// return a token for system function or variable \b ucs
   static Token get_quad(UCS_string ucs, int & len);

   /// return oldest SI entry that is running \b exex, or 0 if none
   static StateIndicator * oldest_exec(const Executable * exec);

   /// return true iff function \b funname is on the current call stack
   static bool is_called(const UCS_string & funname);

   /// write symbols for )OUT command
   static void write_OUT(FILE * out, uint64_t & seq,
                  const vector<UCS_string> & objects);

   /// clear the marked flag in all values known in this workspace
   static void unmark_all_values();

   /// print all owners of \b value
   static int show_owners(ostream & out, const Value & value);

   /// maybe remove functions for which ⎕EX has failed
   static int cleanup_expunged(ostream & out, bool & erased);

   /// replace old_value by new_value in the next higher SI context. Called
   /// when a variable is assigned and the old value has arg flag set.
   static void replace_arg(Value_P old_value, Value_P new_value);

   // access to system variables.
   //
#define ro_sv_def(x) static Quad_ ## x & get_v_quad_ ## x() \
   { return the_workspace.v_quad_ ## x; }
#define rw_sv_def(x) static Quad_ ## x & get_v_quad_ ## x() \
   { return the_workspace.v_quad_ ## x; }
   rw_sv_def(QUAD)
   rw_sv_def(QUOTE)
#include "SystemVariable.def"

protected:
   UCS_string WS_name;

   // system variables.
   //
#define ro_sv_def(x) Quad_ ## x v_quad_ ## x;
#define rw_sv_def(x) Quad_ ## x v_quad_ ## x;
   rw_sv_def(QUAD)
   rw_sv_def(QUOTE)
#include "SystemVariable.def"

   /// the APL prompt (6 blanks by default)
   UCS_string prompt;

   /// user defined functions that were ⎕EX'ed while on the SI stack
   vector<const UserFunction *> expunged_functions;

   /// more info about last error
   UCS_string more_error_info;

   /// the SI stack. Initially top_SI is 0 (empty stack)
   StateIndicator * top_SI;

   /// the current workspace (for objects that need one but don't have one).
   static Workspace the_workspace;
};
//-----------------------------------------------------------------------------

#endif // __WORKSPACE_HH_DEFINED__
