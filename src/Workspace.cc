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

#include <errno.h>
#include <string.h>
#include <fstream>

using namespace std;

#include "Archive.hh"
#include "Command.hh"
#include "Input.hh"
#include "LibPaths.hh"
#include "main.hh"
#include "Output.hh"
#include "Quad_FX.hh"
#include "Quad_TF.hh"
#include "TestFiles.hh"
#include "UserFunction.hh"
#include "Workspace.hh"

// Workspace::the_workspace is defined in StaticObjects.cc

//-----------------------------------------------------------------------------
void
Workspace::push_SI(const Executable * fun, const char * loc)
{
   Assert1(fun);


   if (Quad_SYL::si_depth_limit && SI_top() &&
       Quad_SYL::si_depth_limit <= SI_top()->get_level())
      {
        Workspace::more_error() = UCS_string(
        "the system limit on SI depth (as set in ⎕SYL) was reached\n"
        "(and to avoid lock-up, the limit in ⎕SYL was automatically cleared).");

        Quad_SYL::si_depth_limit = 0;
        attention_raised = interrupt_raised = true;
      }

   the_workspace.top_SI = new StateIndicator(fun, SI_top());

   Log(LOG_StateIndicator__push_pop)
      {
        CERR << "Push  SI[" <<  SI_top()->get_level() << "] "
             << "pmode=" << fun->get_parse_mode()
             << " exec=" << fun << " "
             << fun->get_name();

        CERR << " new SI is " << (const void *)SI_top()
             << " at " << loc << endl;
      }
}
//-----------------------------------------------------------------------------
void
Workspace::pop_SI(const char * loc)
{
   Assert(SI_top());
const Executable * exec = SI_top()->get_executable();
   Assert1(exec);


   Log(LOG_StateIndicator__push_pop)
      {
        CERR << "Pop  SI[" <<  SI_top()->get_level() << "] "
             << "pmode=" << exec->get_parse_mode()
             << " exec=" << exec << " ";

        if (exec->get_ufun())   CERR << exec->get_ufun()->get_name();
        else                    CERR << SI_top()->get_parse_mode_name();
        CERR << " " << (const void *)SI_top() << " at " << loc << endl;
      }

   // remove the top SI
   //
StateIndicator * del = SI_top();
   the_workspace.top_SI = del->get_parent();
   delete del;
}
//-----------------------------------------------------------------------------
StateIndicator *
Workspace::SI_top_fun()
{
   for (StateIndicator * si = SI_top(); si; si = si->get_parent())
       {
         if (si->get_executable()->get_parse_mode() == PM_FUNCTION)   return si;
       }

   return 0;   // no context wirh parse mode PM_FUNCTION
}
//-----------------------------------------------------------------------------
const StateIndicator *
Workspace::SI_top_error()
{
   for (const StateIndicator * si = SI_top(); si; si = si->get_parent())
       {
         if (si->get_error().error_code != E_NO_ERROR)   return si;
       }

   return 0;   // no context with an error
}
//-----------------------------------------------------------------------------
Token
Workspace::immediate_execution(bool exit_on_error)
{
   for (;;)
       {
         try
           {
              Command::process_line();
           }
         catch (Error err)
            {
              if (!err.get_print_loc())
                 {
                   if (err.error_code != E_DEFN_ERROR)
                      {
                        err.print_em(UERR, LOC);
                        CERR << __FUNCTION__ << "() caught APL error "
                             << HEX(err.error_code) << " ("
                             << err.error_name(err.error_code) << ")" << endl;

                        TestFiles::apl_error(LOC);
                      }
                 }
              if (exit_on_error)   return Token(TOK_OFF);
            }
         catch (...)
            {
              CERR << "*** " << __FUNCTION__
                   << "() caught other exception ***" << endl;
              TestFiles::apl_error(LOC);
              if (exit_on_error)   return Token(TOK_OFF);
            }
      }

   return Token(TOK_ESCAPE);
}
//-----------------------------------------------------------------------------
NamedObject *
Workspace::lookup_existing_name(const UCS_string & name)
{
   if (Avec::is_quad(name[0]))   // distinguished name
      {
        int len;
        Token tok = get_quad(name, len);
        if (len == 1)                          return 0;
        if (name.size() != len)                return 0;
        if (tok.get_Class() == TC_SYMBOL)      return tok.get_sym_ptr();
        if (tok.get_Class() == TC_FUN0)        return tok.get_function();
        if (tok.get_Class() == TC_FUN1)        return tok.get_function();
        if (tok.get_Class() == TC_FUN2)        return tok.get_function();

        Assert(0);
      }

   // user defined variable or function
   //
   return the_workspace.symbol_table.lookup_existing_name(name);
}
//-----------------------------------------------------------------------------
Symbol *
Workspace::lookup_existing_symbol(const UCS_string & symbol_name)
{
   if (symbol_name.size() == 0)   return 0;

   if (Avec::is_quad(symbol_name[0]))   // distinguished name
      {
        int len;
        Token tok = get_quad(symbol_name, len);
        if (symbol_name.size() != len)         return 0;
        if (tok.get_Class() != TC_SYMBOL)      return 0;   // system function

        return tok.get_sym_ptr();
      }

   return the_workspace.symbol_table.lookup_existing_symbol(symbol_name);
}
//-----------------------------------------------------------------------------
Token
Workspace::get_quad(UCS_string ucs, int & len)
{
   if (ucs.size() == 0 || !Avec::is_quad(ucs[0]))
     {
       len = 0;
       return Token();
     }

const Unicode av_0 = (ucs.size() > 1) ? ucs[1] : Invalid_Unicode;
const Unicode av_1 = (ucs.size() > 2) ? ucs[2] : Invalid_Unicode;
const Unicode av_2 = (ucs.size() > 3) ? ucs[3] : Invalid_Unicode;

#define var(t, l) { len = l + 1; \
   return Token(TOK_QUAD_ ## t, &the_workspace.v_quad_ ## t); }

#define f0(t, l) { len = l + 1; \
   return Token(TOK_QUAD_ ## t, &Quad_ ## t::fun); }

#define f1(t, l) { len = l + 1; \
   return Token(TOK_QUAD_ ## t, &Quad_ ## t::fun); }

#define f2(t, l) { len = l + 1; \
   return Token(TOK_QUAD_ ## t, &Quad_ ## t::fun); }

   switch(av_0)
      {
        case UNI_ASCII_A:
             if (av_1 == UNI_ASCII_F)        f1 (AF, 2)
             if (av_1 == UNI_ASCII_I)        var(AI, 2)
             if (av_1 == UNI_ASCII_R)
                {
                  if (av_2 == UNI_ASCII_G)   var(ARG, 3)
                }
             if (av_1 == UNI_ASCII_T)        f2 (AT, 2)
             if (av_1 == UNI_ASCII_V)        var(AV, 2)
             break;

        case UNI_ASCII_C:
             if (av_1 == UNI_ASCII_R)        f1 (CR, 2)
             if (av_1 == UNI_ASCII_T)        var(CT, 2)
             break;

        case UNI_ASCII_D:
             if (av_1 == UNI_ASCII_L)        f1 (DL, 2)
             break;

        case UNI_ASCII_E:
             if (av_1 == UNI_ASCII_A)        f2 (EA, 2)
             if (av_1 == UNI_ASCII_C)        f1 (EC, 2)
             if (av_1 == UNI_ASCII_M)        var(EM, 2)
             if (av_1 == UNI_ASCII_N)
                {
                  if (av_2 == UNI_ASCII_V)   f1(ENV, 3)
                }
             if (av_1 == UNI_ASCII_S)        f2 (ES, 2)
             if (av_1 == UNI_ASCII_T)        var(ET, 2)
             if (av_1 == UNI_ASCII_X)        f1 (EX, 2)
             break;

        case UNI_ASCII_F:
             if (av_1 == UNI_ASCII_C)        var(FC, 2)
             if (av_1 == UNI_ASCII_X)        f2 (FX, 2)
             break;

        case UNI_ASCII_I:
             if (av_1 == UNI_ASCII_N)
                {
                  if (av_2 == UNI_ASCII_P)   f2(INP, 3)
                }

             if (av_1 == UNI_ASCII_O)        var(IO, 2)
             break;

        case UNI_ASCII_L:
             if (av_1 == UNI_ASCII_C)        var(LC, 2)
             if (av_1 == UNI_ASCII_X)        var(LX, 2)
                                             var(L, 1)
             break;

        case UNI_ASCII_N:
             if (av_1 == UNI_ASCII_A)        f2 (NA, 2);
             if (av_1 == UNI_ASCII_C)        f2 (NC, 2);
             if (av_1 == UNI_ASCII_L)
                {
                  if (av_2 == UNI_ASCII_T)   var(NLT, 3)
                                             f1 (NL, 2)
                }
             break;

        case UNI_ASCII_P:
             if (av_1 == UNI_ASCII_P)        var(PP, 2)
             if (av_1 == UNI_ASCII_R)        var(PR, 2)
             if (av_1 == UNI_ASCII_S)        var(PS, 2)
             if (av_1 == UNI_ASCII_T)        var(PT, 2)
             if (av_1 == UNI_ASCII_W)        var(PW, 2)
             break;

        case UNI_ASCII_R:
             if (av_1 == UNI_ASCII_L)        var(RL, 2)
                                             var(R, 1)
             break;

        case UNI_ASCII_S:
             if (av_1 == UNI_ASCII_I)      f1 (SI, 2)
             if (av_1 == UNI_ASCII_V)
                {
                  if (av_2 == UNI_ASCII_C)   f2 (SVC, 3)
                  if (av_2 == UNI_ASCII_E)   var(SVE, 3)
                  if (av_2 == UNI_ASCII_O)   f2 (SVO, 3)
                  if (av_2 == UNI_ASCII_Q)   f2 (SVQ, 3)
                  if (av_2 == UNI_ASCII_R)   f1 (SVR, 3)
                  if (av_2 == UNI_ASCII_S)   f1 (SVS, 3)
                }
             if (av_1 == UNI_ASCII_Y)
                {
                  if (av_2 == UNI_ASCII_L)   var(SYL, 3)
                }
             break;

        case UNI_ASCII_T:
             if (av_1 == UNI_ASCII_C)        var(TC, 2)
             if (av_1 == UNI_ASCII_F)        f2 (TF, 2)
             if (av_1 == UNI_ASCII_S)        var(TS, 2)
             if (av_1 == UNI_ASCII_Z)        var(TZ, 2)
             break;

        case UNI_ASCII_U:
             if (av_1 == UNI_ASCII_C &&
                 av_2 == UNI_ASCII_S)        f1 (UCS, 3)
             if (av_1 == UNI_ASCII_L)        var(UL, 2)
             break;

        case UNI_ASCII_W:
             if (av_1 == UNI_ASCII_A)        var(WA, 2)
             break;
      }

   var(QUAD, 0);

#undef var
#undef f0
#undef f1
#undef f2
}
//-----------------------------------------------------------------------------
StateIndicator *
Workspace:: oldest_exec(const Executable * exec)
{
StateIndicator * ret = 0;

   for (StateIndicator * si = SI_top(); si; si = si->get_parent())
       if (exec == si->get_executable())   ret = si;   // maybe not yet oldest

   return ret;
}
//-----------------------------------------------------------------------------
bool
Workspace::is_called(const UCS_string & funname)
{
   for (const StateIndicator * si = SI_top(); si; si = si->get_parent())
      {
        UCS_string si_fun = si->function_name();
       if (funname == si_fun)   return true;
      }

   return false;
}
//-----------------------------------------------------------------------------
void
Workspace::write_OUT(FILE * out, uint64_t & seq, const vector<UCS_string>
                     & objects)
{
   // if objects is empty then write all user define objects and some system
   // variables
   //
   if (objects.size() == 0)   // all objects plus some system variables
      {
        get_v_quad_CT().write_OUT(out, seq);
        get_v_quad_FC().write_OUT(out, seq);
        get_v_quad_IO().write_OUT(out, seq);
        get_v_quad_LX().write_OUT(out, seq);
        get_v_quad_PP().write_OUT(out, seq);
        get_v_quad_PR().write_OUT(out, seq);
        get_v_quad_RL().write_OUT(out, seq);

        get_symbol_table().write_all_symbols(out, seq);
      }
   else                       // only specified objects
      {
         loop(o, objects.size())
            {
              NamedObject * obj = lookup_existing_name(objects[o]);
              if (obj == 0)   // not found
                 {
                   COUT << ")OUT: " << objects[o] << " NOT SAVED (not found)"
                        << endl;
                   continue;
                 }

              const Id obj_id = obj->get_Id();
              if (obj_id == ID_USER_SYMBOL)   // user defined name
                 {
                   const Symbol * sym = lookup_existing_symbol(objects[o]);
                   Assert(sym);
                   sym->write_OUT(out, seq);
                 }
              else                            // distinguished name
                 {
                   const Symbol * sym = obj->get_symbol();
                   if (sym == 0)
                      {
                        COUT << ")OUT: " << objects[o]
                             << " NOT SAVED (not a variable)" << endl;
                        continue;
                      }

                   sym->write_OUT(out, seq);
                 }
            }
      }
}
//-----------------------------------------------------------------------------
void
Workspace::unmark_all_values()
{
   // unmark user defined symbols
   //
   the_workspace.symbol_table.unmark_all_values();

   // unmark system variables
   //
#define rw_sv_def(x) the_workspace.v_quad_ ## x.unmark_all_values();
#define ro_sv_def(x) the_workspace.v_quad_ ## x.unmark_all_values();
#include "SystemVariable.def"
   the_workspace.v_quad_QUAD .unmark_all_values();
   the_workspace.v_quad_QUOTE.unmark_all_values();

   // unmark token reachable vi SI stack
   //
   for (StateIndicator * si = SI_top(); si; si = si->get_parent())
       si->unmark_all_values();

   // unmark token in (failed) ⎕EX functions
   //
   loop(f, the_workspace.expunged_functions.size())
      the_workspace.expunged_functions[f]->unmark_all_values();
}
//-----------------------------------------------------------------------------
int
Workspace::show_owners(ostream & out, const Value & value)
{
int count = 0;

   // user defined variabes
   //
   count += the_workspace.symbol_table.show_owners(out, value);

   // system variabes
   //
#define rw_sv_def(x) count += get_v_quad_ ## x().show_owners(out, value);
#define ro_sv_def(x) count += get_v_quad_ ## x().show_owners(out, value);
#include "SystemVariable.def"
   count += the_workspace.v_quad_QUAD .show_owners(out, value);
   count += the_workspace.v_quad_QUOTE.show_owners(out, value);

   for (StateIndicator * si = SI_top(); si; si = si->get_parent())
       count += si->show_owners(out, value);

   loop(f, the_workspace.expunged_functions.size())
       {
         char cc[100];
         snprintf(cc, sizeof(cc), "    ⎕EX[%lld] ", f);
         count += the_workspace.expunged_functions[f]->show_owners(cc, out, value);
       }

   return count;
}
//-----------------------------------------------------------------------------
int
Workspace::cleanup_expunged(ostream & out, bool & erased)
{
const int ret = the_workspace.expunged_functions.size();

   if (SI_entry_count() > 0)
      {
        out << "SI not cleared (size " << SI_entry_count()
            << "): not deleting ⎕EX'ed functions (try )SIC first)" << endl;
        erased = false;
        return ret;
      }

   while(the_workspace.expunged_functions.size())
       {
         const UserFunction * ufun = the_workspace.expunged_functions.back();
         the_workspace.expunged_functions.pop_back();
         out << "finally deleting " << ufun->get_name() << "...";
         delete ufun;
         out << " OK" << endl;
       }

   erased = true;
   return ret;
}
//-----------------------------------------------------------------------------
void
Workspace::replace_arg(Value_P old_value, Value_P new_value)
{
   for (StateIndicator * si = SI_top(); si; si = si->get_parent())
       {
        if (si->replace_arg(old_value, new_value))   break;
       }
}
//-----------------------------------------------------------------------------
void
Workspace::clear_WS(ostream & out, bool silent)
{
   // clear the SI (pops all localized symbols)
   //
   clear_SI(out);

   // clear the symbol table
   //
   the_workspace.symbol_table.clear(out);

   // clear the )MORE error info
   //
   more_error().clear();

   // clear the read/write system variables...
   //
#define rw_sv_def(x) get_v_quad_ ## x().clear_vs();
#define ro_sv_def(x)
#include "SystemVariable.def"

   // at this point all values should have been gone.
   // complain about those that still exist, and remove them.
   //
// Value::erase_all(out);

#define rw_sv_def(x) new  (&get_v_quad_ ##x())  Quad_ ##x;
#define ro_sv_def(x)
#include "SystemVariable.def"

   set_WS_name(UCS_string("CLEAR WS"));
   if (!silent)   out << "CLEAR WS" << endl;
}
//-----------------------------------------------------------------------------
void
Workspace::clear_SI(ostream & out)
{
   while (SI_top())
      {
        SI_top()->clear(out);
        pop_SI(LOC);
      }
}
//-----------------------------------------------------------------------------
void
Workspace::list_SI(ostream & out, SI_mode mode)
{
   for (const StateIndicator * si = SI_top(); si; si = si->get_parent())
       si->list(out, mode);

   if (mode && SIM_debug)   out << endl;
}
//-----------------------------------------------------------------------------
void
Workspace::save_WS(ostream & out, vector<UCS_string> & lib_ws)
{
   // )SAVE
   // )SAVE wsname
   // )SAVE libnum wsname

   if (lib_ws.size() == 0)   // no argument: use )WSID value
      {
         lib_ws.push_back(the_workspace.WS_name);
      }
   else if (lib_ws.size() > 2)   // too many arguments
      {
        out << "BAD COMMAND" << endl;
        more_error() = UCS_string("too many parameters in command )SAVE");
        return;
      }

   // at this point, lib_ws.size() is 1 or 2.

LibRef libref = LIB_NONE;
UCS_string wname = lib_ws.back();
   if (lib_ws.size() == 2)   libref = (LibRef)(lib_ws.front().atoi());
UTF8_string filename = LibPaths::get_lib_filename(libref, wname, false, "xml");


   // append an .xml extension unless there is one already
   //
   if (filename.size() < 5                  ||
       filename[filename.size() - 4] != '.' ||
       filename[filename.size() - 3] != 'x' ||
       filename[filename.size() - 2] != 'm' ||
       filename[filename.size() - 1] != 'l' )
      {
        // filename does not end with .xml, so we try filename.xml
        //
        filename.append(UTF8_string(".xml"));
      }


   if (wname.compare(UCS_string("CLEAR WS")) == 0)   // don't save CLEAR WS
      {
        COUT << "NOT SAVED: THIS WS IS " << wname << endl;
        more_error() = UCS_string(
        "the workspace was not saved because 'CLEAR WS' is a special \n"
        "workspace name that cannot be saved. Use )WSID <name> first.");
        return;
      }

   // dont save if workspace names differ and file exists
   //
   {
     if (access(filename.c_str(), F_OK) == 0)   // file exists
        {
          if (wname.compare(the_workspace.WS_name) != 0)   // names differ
             {
               COUT << "NOT SAVED: THIS WS IS " << wname << endl;
               UCS_string & t4 = more_error();
               t4.clear();
               t4.append_utf8("the workspace was not saved because:\n"
                      "   the workspace name '");
               t4.append(the_workspace.WS_name);
               t4.append_utf8("' of )WSID\n   does not match the name '");
               t4.append(wname);
               t4.append_utf8("' used in the )SAVE command\n"
                      "   and the workspace file\n   ");
               t4.append_utf8(filename.c_str());
               t4.append_utf8("\n   already exists. Use )WSID ");
               t4.append(wname);
               t4.append_utf8(" first."); 
               return;
             }
        }
   }

   // at this point it is OK to rename and save the workspace
   //
   the_workspace.WS_name = wname;

ofstream outf(filename.c_str(), ofstream::out);
   if (!outf.is_open())   // open failed
      {
        CERR << "Unable to )SAVE workspace '"
             << the_workspace.WS_name << "'." << endl;
        return;
      }

XML_Saving_Archive ar(outf);
   ar.save();

   // print time and date to COUT
   {
     const int offset = get_v_quad_TZ().get_offset();
     const YMDhmsu time(now());
const char * tz_sign = (offset < 0) ? "" : "+";

     COUT << setfill('0') << time.year  << "-"
          << setw(2)      << time.month << "-"
          << setw(2)      << time.day   << "  " 
          << setw(2)      << time.hour  << ":"
          << setw(2)      << time.minute << ":"
          << setw(2)      << time.second << " (GMT"
          << tz_sign      << offset/3600 << ")"
          << setfill(' ') << endl;
   }
}
//-----------------------------------------------------------------------------
// return ⎕LX of loaded WS on success
UCS_string 
Workspace::load_WS(ostream & out, const vector<UCS_string> & lib_ws)
{
   if (lib_ws.size() < 1 || lib_ws.size() > 2)   // no or too many argument(s)
      {
        out << "BAD COMMAND" << endl;
        return UCS_string();
      }

LibRef libref = LIB_NONE;
   if (lib_ws.size() == 2)   libref = (LibRef)(lib_ws.front().atoi());
UCS_string wname = lib_ws.back();
UTF8_string filename = LibPaths::get_lib_filename(libref, wname, true, "xml");

XML_Loading_Archive in(filename.c_str());

   if (!in.is_open())   // open failed: try filename.xml unless already .xml
      {
        if (filename.size() < 5                  ||
            filename[filename.size() - 4] != '.' ||
            filename[filename.size() - 3] != 'x' ||
            filename[filename.size() - 2] != 'm' ||
            filename[filename.size() - 1] != 'l' )
           {
             // filename does not end with .xml, so we try filename.xml
             //
             filename.append(UTF8_string(".xml"));
             new (&in) XML_Loading_Archive(filename.c_str());
           }

        if (!in.is_open())   // open failed again: give up
           {
             CERR << ")LOAD " << wname << " (file " << filename
                  << ") failed: " << strerror(errno) << endl;
             return UCS_string();
           }
      }

   Log(LOG_command_IN)   CERR << "LOADING " << wname << " from file '"
                              << filename << "' ..." << endl;

   // got open file. We assume that from here on everything will be fine.
   // clear current WS and load it from file
   //
   the_workspace.clear_WS(CERR, true);
   in.read_Workspace();
   return Workspace::get_LX();
}
//-----------------------------------------------------------------------------
void
Workspace::copy_WS(ostream & out, vector<UCS_string> & lib_ws_objects,
                   bool protection)
{
   if (lib_ws_objects.size() < 1)   // at least workspace name is required
      {
        out << "BAD COMMAND" << endl;
        return;
      }

   // move the first one or two items in lib_ws_objects to lib_ws
   //
vector<UCS_string> lib_ws;
const bool with_lib = Command::is_lib_ref(lib_ws_objects.front());

   lib_ws.push_back(lib_ws_objects[0]);
   lib_ws_objects.erase(lib_ws_objects.begin());
   if (with_lib)   // lib wname
      {
        lib_ws.push_back(lib_ws_objects[0]);
        lib_ws_objects.erase(lib_ws_objects.begin());
      }

LibRef libref = LIB_NONE;
   if (lib_ws.size() == 2)   libref = (LibRef)(lib_ws.front().atoi());
UCS_string wname = lib_ws.back();
UTF8_string filename = LibPaths::get_lib_filename(libref, wname, true, "xml");

XML_Loading_Archive in(filename.c_str());

   if (!in.is_open())   // open failed: try filename.xml unless already .xml
      {
        if (filename.size() < 5                  ||
            filename[filename.size() - 4] != '.' ||
            filename[filename.size() - 3] != 'x' ||
            filename[filename.size() - 2] != 'm' ||
            filename[filename.size() - 1] != 'l' )
           {
             // filename does not end with .xml, so we try filename.xml
             //
             filename.append(UTF8_string(".xml"));
             new (&in) XML_Loading_Archive(filename.c_str());
           }

        if (!in.is_open())   // open failed again: give up
           {
             CERR << ")COPY " << wname << " (file " << filename
                  << ") failed: " << strerror(errno) << endl;
             return;
           }
      }

   Log(LOG_command_IN)   CERR << "LOADING " << wname << " from file '"
                              << filename << "' ..." << endl;

   in.set_protection(protection, lib_ws_objects);
   in.read_vids();
   in.read_Workspace();
}
//-----------------------------------------------------------------------------
void
Workspace::wsid(ostream & out, UCS_string arg)
{
   while (arg.size() && arg[0] <= ' ')       arg.remove_front();
   while (arg.size() && arg.back() <= ' ')   arg.erase(arg.size() - 1);

   if (arg.size() == 0)   // inquire workspace name
      {
        out << "IS " << the_workspace.WS_name << endl;
        return;
      }

   loop(a, arg.size())
      {
        if (arg[a] <= ' ')
           {
             out << "Bad WS name '" << arg << "'" << endl;
             return;
           }
      }

   out << "WAS " << the_workspace.WS_name << endl;
   the_workspace.WS_name = arg;
}
//=============================================================================
PrintContext::PrintContext()
: style(Workspace::get_PS()),
  precision(Workspace::get_PP()),
  width(Workspace::get_PW())
{
}
//=============================================================================

