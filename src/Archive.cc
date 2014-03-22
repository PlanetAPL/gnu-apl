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

#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "Archive.hh"
#include "Common.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "Executable.hh"
#include "IntCell.hh"
#include "FloatCell.hh"
#include "Function.hh"
#include "IndexExpr.hh"
#include "LvalCell.hh"
#include "NativeFunction.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "PrintOperator.hh"
#include "Symbol.hh"
#include "Token.hh"
#include "UCS_string.hh"
#include "UserFunction.hh"
#include "Value.hh"
#include "Workspace.hh"

using namespace std;

#define NEED(x, cm)   if (space < (x))                                   \
                         { if (cm)   out << UNI_PAD_U0;   out << "\n";   \
                           space = do_indent(); char_mode = false; } out

//-----------------------------------------------------------------------------
bool
XML_Saving_Archive::xml_allowed(Unicode uni)
{
   if (uni < ' ')   return false;    // control chars and negative
   if (uni > '~')   return false;    // allowed, but may not print properly
   if (uni == '<')   return false;   // < not allowed
   if (uni == '&')   return false;   // < not allowed
   if (uni == '"')   return false;   // not allowed in "..."
   return true;
}
//-----------------------------------------------------------------------------
const char *
XML_Saving_Archive::decr(int & counter, const char * str)
{
   counter -= strlen(str);
   return str;
}
//-----------------------------------------------------------------------------
int
XML_Saving_Archive::do_indent()
{
const int spaces = indent * INDENT_LEN;
   loop(s, spaces)   out << " ";

   return 78 - spaces;
}
//-----------------------------------------------------------------------------
int
XML_Saving_Archive::find_owner(const Cell * cell)
{
   loop(v, values.size())
      {
        const Value & val = values[v]._val;
        if (cell < &val.get_ravel(0))      continue;
        if (cell >= val.get_ravel_end())   continue;
        return v;
      }

   FIXME;
   return -1;
}
//-----------------------------------------------------------------------------
int
XML_Saving_Archive::find_vid(const Value & val)
{
   loop(v, values.size())
      {
         if (&val == &values[v]._val)   return v;
      }

   return -1;
}
//-----------------------------------------------------------------------------
void
XML_Saving_Archive::emit_unicode(Unicode uni, int & space, bool & char_mode)
{
   NEED(2, char_mode) << "";
   if (uni == UNI_ASCII_LF)
      {
        NEED(2, char_mode) << UNI_PAD_U1 << "A";
        char_mode = false;
        NEED(space + 4, char_mode);   // force new line
      }
   else if (!xml_allowed(uni))
      {
        char cc[40];
        snprintf(cc, sizeof(cc), "%X", uni);
        NEED(1 + strlen(cc), char_mode) << UNI_PAD_U1 << decr(space, cc);
        space--;   // PAD_U1
        char_mode = false;
      }

   else if (!char_mode)
      {
        NEED(2, char_mode) << UNI_PAD_U2 << uni;
        space -= 2;   // PAD_U2 + uni
        char_mode = true;
      }
   else 
      {
        NEED(1, char_mode) << uni;
        space--;   // uni
      }
}
//-----------------------------------------------------------------------------
XML_Saving_Archive &
XML_Saving_Archive::operator <<(const UCS_string & ucs)
{
int space = do_indent();
char cc[40];
bool char_mode = false;
   out << decr(space, "<UCS uni=\"");
   ++indent;
   loop(u, ucs.size())
      {
        const Unicode uni = ucs[u];
        emit_unicode(ucs[u], space, char_mode);
      }

   NEED(3, false) << "\"/>" << endl;
   --indent;

   return *this;
}
//-----------------------------------------------------------------------------
XML_Saving_Archive &
XML_Saving_Archive::save_shape(const Value & v)
{
char cc[80];

   do_indent();
   snprintf(cc, sizeof(cc), "<Value flg=\"%X\" vid=\"%d\"",
                 v.get_flags(), vid);
   out << cc;

const unsigned int sub_vid = find_vid(v);
   Assert(sub_vid < values.size());
unsigned int parent_vid = -1;

   if (&values[sub_vid]._par)   parent_vid = find_vid(values[sub_vid]._par);

   snprintf(cc, sizeof(cc), " parent=\"%d\"", parent_vid);
   out << cc;
   snprintf(cc, sizeof(cc), " rk=\"%u\"", v.get_rank());
   out << cc;

   loop (r, v.get_rank())
      {
        snprintf(cc, sizeof(cc), " sh-%llu=\"%llu\"", r, v.get_shape_item(r));
        out << cc;
      }

   out << "/>" << endl;
   return *this;
}
//-----------------------------------------------------------------------------
XML_Saving_Archive &
XML_Saving_Archive::save_ravel(const Value & v)
{
int space = do_indent();
bool char_mode = false;

char cc[80];
   snprintf(cc, sizeof(cc), "<Ravel vid=\"%d\" cells=\"", vid);
   out << decr(space, cc);

   ++indent;
const ShapeItem len = v.nz_element_count();
const Cell * C = &v.get_ravel(0);
   loop(l, len)
      {
        if (char_mode && C->get_cell_type() != CT_CHAR)
           {
             out << UNI_PAD_U0;
             char_mode = false;
             --space;
           }

        emit_cell(*C++, space, char_mode);
      }

   NEED(3, false) << "\"/>" << endl;
   --indent;

   return *this;
}
//-----------------------------------------------------------------------------
void
XML_Saving_Archive::emit_cell(const Cell & cell, int & space, bool & char_mode)
{
char cc[80];
   switch(cell.get_cell_type())
      {
            case CT_CHAR:   // UNI_PAD_U0, UNI_PAD_U1, and UNI_PAD_U2
                 emit_unicode(cell.get_char_value(), space, char_mode);
                 break;

            case CT_INT:
                 snprintf(cc, sizeof(cc), "%lld", cell.get_int_value());
                 NEED(1 + strlen(cc), false) << UNI_PAD_U3 << decr(space, cc);
                 space--;   // PAD_U3
                 break;

            case CT_FLOAT:
                 snprintf(cc, sizeof(cc), "%g", cell.get_real_value());
                 NEED(1 + strlen(cc), false) << UNI_PAD_U4 << decr(space, cc);
                 space--;   // PAD_U4
                 break;

            case CT_COMPLEX:
                 snprintf(cc, sizeof(cc), "%gJ%g",
                          cell.get_real_value(), cell.get_imag_value());
                 NEED(1 + strlen(cc), false) << UNI_PAD_U5 << decr(space, cc);
                 space--;   // PAD_U5
                 break;

            case CT_POINTER:
                 {
                   const int vid = find_vid(*cell.get_pointer_value());
                   snprintf(cc, sizeof(cc), "%d", vid);
                   NEED(1 + strlen(cc), false) << UNI_PAD_U6 << decr(space, cc);
                   space--;   // PAD_U6
                 }
                 break;

            case CT_CELLREF:
                 {
                   Cell * cp = cell.get_lval_value();
                   const int vid = find_owner(cp);
                   const Value & val = values[vid]._val;
                   const ShapeItem offset = val.get_offset(cp);
                   snprintf(cc, sizeof(cc), "%d[%lld]", vid, offset);
                   NEED(1 + strlen(cc), false) << UNI_PAD_U7 << decr(space, cc);
                   space--;   // PAD_U6
                 }
                 break;
      }
}
//-----------------------------------------------------------------------------
XML_Saving_Archive &
XML_Saving_Archive::operator <<(const Function & fun)
{
   do_indent();
   out << "<Function";
   if (fun.is_native())   out << " native=\"1\"";
   out << ">" << endl;
   ++indent;

   *this <<  fun.canonical(false);

   --indent;
   do_indent();
   out << "</Function>" << endl;

   return *this;
}
//-----------------------------------------------------------------------------
XML_Saving_Archive &
XML_Saving_Archive::operator <<(const Prefix & prefix)
{
   do_indent();
   out << "<Parser assign-pending=\"" << prefix.get_assign_state()
       << "\" lookahead-high=\""      << prefix.get_lookahead_high()
       << "\">" << endl;

   // write the lookahead token, starting at the fifo's get position
   //
   ++indent;
   loop(s, prefix.size())
      {
        const Token_loc & tloc = prefix.at(s);
        *this << tloc;
      }
   --indent;

   do_indent();
   out << "</Parser>" << endl;
   return *this;
}
//-----------------------------------------------------------------------------
XML_Saving_Archive &
XML_Saving_Archive::operator <<(const SymbolTable & symtab)
{
   // collect all symbols in this symbol table.
   //
int symbol_count = symtab.symbols_allocated();
Symbol * all_symbols[symbol_count];
   symtab.get_all_symbols(all_symbols, symbol_count);

   // remove erased symbols
   //
   for (int s = 0; s < symbol_count;)
      {
        const Symbol * sym = all_symbols[s];

        if (sym->is_erased())
            {
              all_symbols[s] = all_symbols[--symbol_count];
              continue;
            }

        ++s;
      }

   do_indent();
   out << "<SymbolTable size=\"" << symbol_count << "\">" << endl;

   ++indent;

   while (symbol_count > 0)
      {
        // set idx to the alphabetically smallest name
        //
        int idx = 0;
        for (int i = 1; i < symbol_count; ++i)
            {
              if (all_symbols[idx]->compare(*all_symbols[i]) > 0)   idx = i;
            }

        const Symbol * sym = all_symbols[idx];
        *this << *sym;

        all_symbols[idx] = all_symbols[--symbol_count];
      }

   --indent;

   do_indent();
   out << "</SymbolTable>" << endl << endl;

   return *this;
}
//-----------------------------------------------------------------------------
XML_Saving_Archive &
XML_Saving_Archive::operator <<(const StateIndicator & si)
{
     const Executable & exec = *si.get_executable();

   do_indent();
   out << "<SI-entry level=\"" << (si.get_level() - 1)
       << "\" pc=\"" << si.get_PC()
       << "\" line=\"" << exec.get_line(si.get_PC()) << "\"" << flush;

   if (si.eval_arg_F)   // function pending
      {
        if (!!si.eval_arg_A)   // function has a left arg
           {
             const unsigned int vid_A = find_vid(*si.eval_arg_A.get());
             out << " vid_arg_A=\"" << vid_A << "\"";
           }

        {
          char cc[80];
          snprintf(cc, sizeof(cc), " Id_arg_F=\"%X\"", si.eval_arg_F->get_Id());
          out << cc;
        }

        if (!!si.eval_arg_B)   // function has a right arg
           {
             const int vid_B = find_vid(*si.eval_arg_B.get());
             out << " vid_arg_B=\"" << vid_B << "\"";
           }
      }

   out <<">" << endl;

   ++indent;
   do_indent();
   switch(exec.get_parse_mode())
      {
        case PM_FUNCTION:
             {
               Symbol * sym = Workspace::lookup_symbol(exec.get_name());
               Assert(sym);
               const UserFunction * ufun = exec.get_ufun();
               Assert(ufun);
               const int sym_depth = sym->get_ufun_depth(ufun);

               out << "<UserFunction ufun-name=\"" << sym->get_name()
                 << "\" symbol-level=\"" << sym_depth << "\"/>" << endl;
             }
             break;

        case PM_STATEMENT_LIST:
             out << "<Statements>" << endl;
             ++indent;
             *this << exec.get_text(0);
             --indent;
             do_indent();
             out << "</Statements>" << endl;
               break;

        case PM_EXECUTE:
             out << "<Execute>" << endl;
             ++indent;
             *this << exec.get_text(0);
             --indent;
             do_indent();
             out << "</Execute>" << endl;
             break;

          default: FIXME;
      }

   // print the parser states...
   //
   *this << si.current_stack;

   --indent;

   do_indent();
   out << "</SI-entry>" << endl << endl;

   return *this;
}
//-----------------------------------------------------------------------------
XML_Saving_Archive &
XML_Saving_Archive::operator <<(const Symbol & sym)
{
   do_indent();
   out << "<Symbol name=\"" << sym.get_name() << "\" stack-size=\""
       << sym.value_stack_size() << "\">" << endl;

   ++indent;
   loop(v, sym.value_stack_size())  *this << sym[v];
   --indent;

   do_indent();
   out << "</Symbol>" << endl << endl;

   return *this;
}
//-----------------------------------------------------------------------------
XML_Saving_Archive &
XML_Saving_Archive::operator <<(const Token & tok)
{
char cc[80];
   snprintf(cc, sizeof(cc), "%X", tok.get_tag());

   do_indent();
   out << "<Token tag=\"" << cc << "\"";
   emit_token_val(tok);

   out << ">" << endl;
   return *this;
}
//-----------------------------------------------------------------------------
XML_Saving_Archive &
XML_Saving_Archive::operator <<(const Token_loc & tloc)
{
char cc[80];
   snprintf(cc, sizeof(cc), "%X", tloc.tok.get_tag());

   do_indent();
   out << "<Token pc=\"" << tloc.pc
       << "\" tag=\"" << cc << "\"";
   emit_token_val(tloc.tok);

   out << ">" << endl;
   return *this;
}
//-----------------------------------------------------------------------------
void
XML_Saving_Archive::emit_token_val(const Token & tok)
{
   switch(tok.get_ValueType())
      {
        case TV_NONE:  out << "/";
                       break;

        case TV_CHAR:  out << " char=\"" << int(tok.get_char_val()) << "\"/";
                       break;

        case TV_INT:   out << " int=\"" << tok.get_int_val() << "\"/";
                       break;

        case TV_FLT:   out << " float=\"" << tok.get_flt_val() << "\"/";
                       break;

        case TV_CPX:   out << " real=\"" << tok.get_cpx_real()
                           << "\" imag=\"" << tok.get_cpx_imag() << "\"/";
                       break;

        case TV_SYM:   {
                         Symbol * sym = tok.get_sym_ptr();
                         const UCS_string name = sym->get_name();
                         out << " sym=\"" << name << "\"/";
                       }
                       break;

        case TV_LIN:   out << " line=\"" << tok.get_fun_line() << "\"/";
                       break;

        case TV_VAL:   { const int vid = find_vid(*tok.get_apl_val());
                         out << " vid=\"" << vid << "\"/";
                       }
                       break;

        case TV_INDEX: { const IndexExpr & idx = tok.get_index_val();
                         const size_t rank = idx.value_count();
                         out << " index=\"";
                         loop(i, rank)
                             {
                               if (i)   out << ",";
                               const Value & val = *idx.values[i].get();
                               if (&val)   out << "vid_" << find_vid(val);
                               else       out << "-";
                                out << "\"/>";
                             }
                       }
                       break;

        case TV_FUN:   {
                         Function * fun = tok.get_function();
                         Assert1(fun);
                         const UserFunction * ufun = fun->get_ufun1();
                         if (ufun)   // user defined function
                            {
                              const UCS_string & fname = ufun->get_name();
                              Symbol * sym = Workspace::lookup_symbol(fname);
                              Assert(sym);
                              const int sym_depth = sym->get_ufun_depth(ufun);
                              out << " ufun-name=\"" << fname
                                  << "\" symbol-level=\"" << sym_depth
                                  << "\"/>" << endl;
                            }
                         else        // primitive or quad function
                            {
                              char cc[40];
                              snprintf(cc, sizeof(cc), "%X", fun->get_Id());
                              out << " fun-id=\"" << cc;
                            }
                       }
                       out << "\"/";
                       break;

        default:       FIXME;

      }
}
//-----------------------------------------------------------------------------
XML_Saving_Archive &
XML_Saving_Archive::operator <<(const ValueStackItem & vsi)
{
   switch(vsi.name_class)
      {
        case NC_UNUSED_USER_NAME:
             do_indent();
             out << "<unused-name/>" << endl;
             break;

        case NC_LABEL:
             do_indent();
             out << "<Label value=\"" << vsi.sym_val.label << "\"/>" << endl;
             break;

        case NC_VARIABLE:
             do_indent();
             out << "<Variable vid=\"" << find_vid(*vsi.apl_val.get())
                 << "\"/>" << endl;
             break;

        case NC_FUNCTION:
        case NC_OPERATOR:
             *this << *vsi.sym_val.function;
             break;

        case NC_SHARED_VAR:
             do_indent();
             out << "<Shared-Variable key=\"" << vsi.sym_val.sv_key
                 << "\"/>" << endl;
             break;

      }

   return *this;
}
//-----------------------------------------------------------------------------
XML_Saving_Archive &
XML_Saving_Archive::save()
{
tm * t;
   {
     timeval now;
     gettimeofday(&now, 0);
     t = gmtime(&now.tv_sec);
   }

   out <<
"<?xml version='1.0' encoding='UTF-8' standalone='yes'?>\n"
"\n"
"<!DOCTYPE Workspace\n"
"[\n"
"    <!ELEMENT Workspace (Value*,Ravel*,SymbolTable,Symbol*,StateIndicator)>\n"
"    <!ATTLIST Workspace  wsid     CDATA #REQUIRED>\n"
"    <!ATTLIST Workspace  year     CDATA #REQUIRED>\n"
"    <!ATTLIST Workspace  month    CDATA #REQUIRED>\n"
"    <!ATTLIST Workspace  day      CDATA #REQUIRED>\n"
"    <!ATTLIST Workspace  hour     CDATA #REQUIRED>\n"
"    <!ATTLIST Workspace  minute   CDATA #REQUIRED>\n"
"    <!ATTLIST Workspace  second   CDATA #REQUIRED>\n"
"    <!ATTLIST Workspace  timezone CDATA #REQUIRED>\n"
"\n"
"        <!ELEMENT Value (#PCDATA)>\n"
"        <!ATTLIST Value flg    CDATA #REQUIRED>\n"
"        <!ATTLIST Value vid    CDATA #REQUIRED>\n"
"        <!ATTLIST Value parent CDATA #IMPLIED>\n"
"        <!ATTLIST Value rk     CDATA #REQUIRED>\n"
"        <!ATTLIST Value sh-0   CDATA #IMPLIED>\n"
"        <!ATTLIST Value sh-1   CDATA #IMPLIED>\n"
"        <!ATTLIST Value sh-2   CDATA #IMPLIED>\n"
"        <!ATTLIST Value sh-3   CDATA #IMPLIED>\n"
"        <!ATTLIST Value sh-4   CDATA #IMPLIED>\n"
"        <!ATTLIST Value sh-5   CDATA #IMPLIED>\n"
"        <!ATTLIST Value sh-6   CDATA #IMPLIED>\n"
"        <!ATTLIST Value sh-7   CDATA #IMPLIED>\n"
"\n"
"        <!ELEMENT Ravel (#PCDATA)>\n"
"        <!ATTLIST Ravel vid    CDATA #REQUIRED>\n"
"        <!ATTLIST Ravel cells  CDATA #REQUIRED>\n"
"\n"
"        <!ELEMENT SymbolTable (Symbol*)>\n"
"        <!ATTLIST SymbolTable size CDATA #REQUIRED>\n"
"\n"
"            <!ELEMENT Symbol (unused-name|Variable|Function|Label|Shared-Variable)*>\n"
"            <!ATTLIST Symbol name       CDATA #REQUIRED>\n"
"            <!ATTLIST Symbol stack-size CDATA #REQUIRED>\n"
"\n"
"                <!ELEMENT unused-name EMPTY>\n"
"\n"
"                <!ELEMENT Variable (#PCDATA)>\n"
"                <!ATTLIST Variable vid CDATA #REQUIRED>\n"
"\n"
"                <!ELEMENT Function (UCS)>\n"
"\n"
"                <!ELEMENT Label (#PCDATA)>\n"
"                <!ATTLIST Label value CDATA #REQUIRED>\n"
"\n"
"                <!ELEMENT Shared-Variable (#PCDATA)>\n"
"                <!ATTLIST Shared-Variable key CDATA #REQUIRED>\n"
"\n"
"        <!ELEMENT UCS (#PCDATA)>\n"
"        <!ATTLIST UCS uni CDATA #REQUIRED>\n"
"\n"
"        <!ELEMENT StateIndicator (SI-entry*)>\n"
"        <!ATTLIST StateIndicator levels CDATA #REQUIRED>\n"
"\n"
"            <!ELEMENT SI-entry ((Execute|Statements|UserFunction),Parser+)>\n"
"            <!ATTLIST SI-entry level     CDATA #REQUIRED>\n"
"            <!ATTLIST SI-entry pc        CDATA #REQUIRED>\n"
"            <!ATTLIST SI-entry line      CDATA #REQUIRED>\n"
"            <!ATTLIST SI-entry vid_arg_A CDATA #IMPLIED>\n"
"            <!ATTLIST SI-entry Id_arg_F  CDATA #IMPLIED>\n"
"            <!ATTLIST SI-entry vid_arg_B CDATA #IMPLIED>\n"
"\n"
"                <!ELEMENT Statements (UCS)>\n"
"\n"
"                <!ELEMENT Execute (UCS)>\n"
"\n"
"                <!ELEMENT UserFunction (#PCDATA)>\n"
"                <!ATTLIST UserFunction ufun-name    CDATA #REQUIRED>\n"
"                <!ATTLIST UserFunction symbol-level CDATA #REQUIRED>\n"
"\n"
"                <!ELEMENT Parser (Token*)>\n"
"                <!ATTLIST Parser assign-pending CDATA #REQUIRED>\n"
"                <!ATTLIST Parser lookahead-high CDATA #REQUIRED>\n"

"                    <!ELEMENT Token (#PCDATA)>\n"
"                    <!ATTLIST Token pc           CDATA #REQUIRED>\n"
"                    <!ATTLIST Token tag          CDATA #REQUIRED>\n"
"                    <!ATTLIST Token char         CDATA #IMPLIED>\n"
"                    <!ATTLIST Token int          CDATA #IMPLIED>\n"
"                    <!ATTLIST Token float        CDATA #IMPLIED>\n"
"                    <!ATTLIST Token real         CDATA #IMPLIED>\n"
"                    <!ATTLIST Token imag         CDATA #IMPLIED>\n"
"                    <!ATTLIST Token sym          CDATA #IMPLIED>\n"
"                    <!ATTLIST Token line         CDATA #IMPLIED>\n"
"                    <!ATTLIST Token vid          CDATA #IMPLIED>\n"
"                    <!ATTLIST Token index        CDATA #IMPLIED>\n"
"                    <!ATTLIST Token fun-id       CDATA #IMPLIED>\n"
"                    <!ATTLIST Token ufun-name    CDATA #IMPLIED>\n"
"                    <!ATTLIST Token symbol-level CDATA #IMPLIED>\n"
"                    <!ATTLIST Token comment  CDATA #IMPLIED>\n"
"\n"
"]>\n"
"\n"
"\n"
"    <!-- hour/minute/second is )SAVE time in UTC (aka. GMT).\n"
"         timezone is offset to UTC in seconds.\n"
"         local time is UTC + offset -->\n"
"<Workspace wsid=\""     << Workspace::get_WS_name()
     << "\" year=\""     << (t->tm_year + 1900)
     << "\" month=\""    << (t->tm_mon  + 1)
     << "\" day=\""      <<  t->tm_mday << "\"" << endl <<
"           hour=\""     <<  t->tm_hour
     << "\" minute=\""   <<  t->tm_min
     << "\" second=\""   <<  t->tm_sec
     << "\" timezone=\"" << Workspace::get_v_quad_TZ().get_offset()
     << "\">\n" << endl;

   ++indent;

   // collect all values to be saved. We mark the values to avoid
   // saving of stale values and unmark the used values
   //
   Value::mark_all_dynamic_values();
   Workspace::unmark_all_values();

   for (const DynamicObject * obj = DynamicObject::get_all_values()->get_next();
        obj != DynamicObject::get_all_values(); obj = obj->get_next())
       {
         const Value & val(*(const Value *)obj);

         if (val.is_forever())   continue;   // e.g. ⍞
         if (val.is_marked())    continue;   // stale

         val.clear_marked();
         values.push_back(_val_par(val));
       }

   // set up parents of values
   //
   loop(p, values.size())
      {
        const Value & parent = values[p]._val;
        const ShapeItem ec = parent.nz_element_count();
        const Cell * cP = &parent.get_ravel(0);
        loop(e, ec)
            {
              if (cP->is_pointer_cell())
                 {
                   const Value & sub = *cP->get_pointer_value().get();
                   Assert1(&sub);
                   unsigned int sub_idx = find_vid(sub);
                   Assert(sub_idx < values.size());
                   Assert(!&values[sub_idx]._par);
                   values[sub_idx] = _val_par(values[sub_idx]._val,  parent);
                 }
              else if (cP->is_lval_cell())
                 {
CERR << "LVAL CELL in " << p << " at " LOC << endl;
                 }
              ++cP;
            }
      }

   // save all values (without their ravel)
   //
   for (vid = 0; vid < values.size(); ++vid)   save_shape(values[vid]._val);

   // save ravels of all values
   //
   for (vid = 0; vid < values.size(); ++vid)   save_ravel(values[vid]._val);

   // save user defined symbols
   //
   *this << Workspace::get_symbol_table();

   // save certain system variables
   //
#define rw_sv_def(x) *this << Workspace::get_v_quad_ ## x();
#define ro_sv_def(x) *this << Workspace::get_v_quad_ ## x();
#include "SystemVariable.def"

   // save state indicator
   //
   {
     const int levels = Workspace::SI_entry_count();
     do_indent();
     out << "<StateIndicator levels=\"" << levels << "\">" << endl;

     ++indent;

     loop(l, levels)
        {
          for (const StateIndicator * si = Workspace::SI_top();
               si; si = si->get_parent())
              {
                if (si->get_level() == l)
                   {
                     *this << *si;
                     break;
                   }
              }
        }

     --indent;

     do_indent();
     out << "</StateIndicator>" << endl << endl;
   }

   --indent;

   do_indent();

   // write closing tag and a few 0's so that string functions
   // can be used on mmaped file.
   //
   out << "</Workspace>" << endl
       << char(0) << char(0) <<char(0) <<char(0) << endl;
}
//=============================================================================
XML_Loading_Archive::XML_Loading_Archive(const char * filename)
   : line(1),
     data(0),
     end(0),
     copying(false),
     protection(false),
     reading_vids(false)
{
   fd = open(filename, O_RDONLY);
   if (fd == -1)   return;

struct stat st;
   if (fstat(fd, &st))
      {
        CERR << "fstat() failed: " << strerror(errno) << endl;
        close(fd);
        fd = -1;
        return;
      }

void * map = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
   if (map == MAP_FAILED)
      {
        CERR << "mmap() failed: " << strerror(errno) << endl;
        close(fd);
        fd = -1;
      }

   // success
   //
   file_start = (const UTF8 *)map;
   end = file_start + st.st_size;
   reset();
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::reset()
{
   line_start = data = file_start;
   line = 1;
   next_tag(LOC);
}
//-----------------------------------------------------------------------------
bool
XML_Loading_Archive::skip_to_tag(const char * tag)
{
   for (;;)
      {
         if (next_tag(LOC))   return true;
         if (is_tag(tag))   break;
      }

   return false;
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_vids()
{
   reset();   // skips to <Workspace>
   reading_vids = true;
   read_Workspace();
   reading_vids = false;

   reset();
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::where(ostream & out)
{
   out << "line=" << line << "+" << (data - line_start) << " '";

   loop(j, 40)   { if (data[j] == 0x0A)   break;   out << data[j]; }
   out << "'" << endl;
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::where_att(ostream & out)
{
   out << "line=" << line << "+" << (attributes - line_start) << " '";

   loop(j, 40)
      {
        if (attributes[j] == 0x0A)        break;
        if (attributes + j >= end_attr)   break;
         out << attributes[j];
      }

   out << "'" << endl;
}
//-----------------------------------------------------------------------------
bool
XML_Loading_Archive::get_uni()
{
   if (data > end)   return true;   // EOF

int len = 0;
   current_char = UTF8_string::toUni(data, len);
   data += len;
   if (current_char == 0x0A)   { ++line;   line_start = data; }
   return false;
}
//-----------------------------------------------------------------------------
bool
XML_Loading_Archive::is_tag(const char * prefix) const
{
   return !strncmp((const char *)tag_name, prefix, strlen(prefix));
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::expect_tag(const char * prefix, const char * loc) const
{
   if (!is_tag(prefix))
      {
        CERR << "   Got tag ";
        print_tag(CERR);
        CERR << " when expecting tag " << prefix
             << " at " << loc << "  line " << line << endl;
        DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::print_tag(ostream & out) const
{
   loop(t, attributes - tag_name)   out << tag_name[t];
}
//-----------------------------------------------------------------------------
const UTF8 *
XML_Loading_Archive::find_attr(const char * att_name, bool optional)
{
const int att_len = strlen(att_name);

   for (const UTF8 * d = attributes; d < end_attr; ++d)
       {
         if (strncmp(att_name, (const char *)d, att_len))   continue;
         const UTF8 * dd = d + att_len;
         while (*dd <= ' ')   ++dd;   // skip whitespaces
         if (*dd++ != '=')   continue;

         // attribute= found. find value.
         while (*dd <= ' ')   ++dd;   // skip whitespaces
         Assert(*dd == '"');
         return dd + 1;
       }

   // not found
   //
   if (!optional)
      {
         CERR << "Attribute name '" << att_name << "' not found in:" << endl;
         where_att(CERR);
         DOMAIN_ERROR;
      }
   return 0;   // not found
}
//-----------------------------------------------------------------------------
int64_t
XML_Loading_Archive::find_int_attr(const char * attrib, bool optional, int base)
{
const UTF8 * value = find_attr(attrib, optional);
   if (value == 0)   return -1;   // not found

const int64_t val = strtoll((const char *)value, 0, base);
   return val;
}
//-----------------------------------------------------------------------------
double
XML_Loading_Archive::find_float_attr(const char * attrib)
{
const UTF8 * value = find_attr(attrib, false);
const double val = strtod((const char *)value, 0);
   return val;
}
//-----------------------------------------------------------------------------
bool
XML_Loading_Archive::next_tag(const char * loc)
{
again:

   // read chars up to (including) '<'
   //
   while (current_char != '<')
      {
         if (get_uni())   return true;
      }

   tag_name = data;

   // read char after <
   //
   if (get_uni())   return true;

   if (current_char == '?')   goto again;   // processing instruction
   if (current_char == '!')   goto again;   // comment
   if (current_char == '/')   get_uni();    // / at start of name

   // read chars before attributes (if any)
   //
   for (;;)
       {
         if (current_char == ' ')   break;
         if (current_char == '/')   break;
         if (current_char == '>')   break; 
         if (get_uni())   return true;
       }

   attributes = data;

   // read chars before end of tag
   //
   while (current_char != '>')
      {
         if (get_uni())   return true;
      }

   end_attr = data;

/*
   CERR << "See tag ";
   for (const UTF8 * t = tag_name; t < attributes; ++t)   CERR << (char)*t;
   CERR << " at " << loc << " line " << line << endl;
*/

   return false;
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_Workspace()
{
   expect_tag("Workspace", LOC);

const UTF8 * wsid  = find_attr("wsid",         false);
const int year     = find_int_attr("year",     false, 10);
const int month    = find_int_attr("month",    false, 10);
const int day      = find_int_attr("day",      false, 10);
const int hour     = find_int_attr("hour",     false, 10);
const int minute   = find_int_attr("minute",   false, 10);
const int second   = find_int_attr("second",   false, 10);
const int tzone    = find_int_attr("timezone", false, 10);

const char * tag_order[] = { "Value",          "Ravel",
                             "SymbolTable",    "Symbol",
                             "StateIndicator", "/Workspace", 0 };
const char ** tag_pos = tag_order;

   for (;;)
       {
         next_tag(LOC);

         // make sure that we do not move backwards in tag_order
         //
         if (!is_tag(*tag_pos))   // new tag
            {
              tag_pos++;
              for (;;)
                  {
                     if (*tag_pos == 0)      DOMAIN_ERROR;   // end of list
                     if (is_tag(*tag_pos))   break;          // found
                  }
            }

         if      (is_tag("Value"))            read_Value();
         else if (is_tag("Ravel"))            read_Ravel();
         else if (is_tag("SymbolTable"))      read_SymbolTable();
         else if (is_tag("Symbol"))           read_Symbol();
         else if (is_tag("StateIndicator"))   read_StateIndicator();
         else if (is_tag("/Workspace"))       break;
         else    /* complain */               expect_tag("UNEXPECTED", LOC);
       }

   // loaded workspace can contain stale variables, e.g. shared vars.
   // remove them.
   //
   Value::erase_stale(LOC);

const char * tz_sign = (tzone < 0) ? "" : "+";
   COUT << "SAVED " << year << "-" << month << "-" << day
        << "  " << hour << ":" << minute << ":" << second
        << " (GMT" << tz_sign << tzone/3600 << ")" << endl;

const UTF8 * end = wsid;
   while (*end != '"')   ++end;

   Workspace::set_WS_name(UCS_string(UTF8_string(wsid, end - wsid)));
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_Value()
{
   expect_tag("Value", LOC);

const int  flg = find_int_attr("flg", false, 16);
const int  vid = find_int_attr("vid", false, 10);
const int  parent = find_int_attr("parent", true, 10);
const int  rk  = find_int_attr("rk",  false, 10);

   if (reading_vids)
      {
         parents.push_back(parent);
         return;
      }

Shape sh_value;
   loop(r, rk)
      {
        char sh[20];
        snprintf(sh, sizeof(sh), "sh-%d", int(r));
        const UTF8 * sh_r = find_attr(sh, false);
        sh_value.add_shape_item(atoll((const char *)sh_r));
      }

   // if we do )COPY or )PCOPY and vid is not in vids_COPY list, then we
   // push 0 (so that indexing with vid still works) and ignore such
   // values in read_Ravel.
   //
bool no_copy = false;   // assume the value is needed
   if (copying)
      {
        // if vid is a sub-value then find its topmost owner
        //
        int parent = vid;
        for (;;)
            {
              Assert(parent < parents.size());
              if (parents[parent] == -1)   break;   // topmost owner found
              parent = parents[parent];
            }

        no_copy = true;   // assume the value is not needed
        loop(v, vids_COPY.size())
           {
              if (parent == vids_COPY[v])   // vid is in the list: copy
                 {
                   no_copy = false;
                   break;
                 }
           }
      }

   if (no_copy)
      {
        values.push_back(Value_P());
      }
   else
      {
        Assert(vid == values.size());

        Value_P val(new Value(sh_value, LOC));
        values.push_back(val);
      }
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_Cells(Cell * & C, const UTF8 * & first)
{
   while (*first <= ' ')   ++first;

int len;
const Unicode type = UTF8_string::toUni(first, len);

   switch (type)
      {
        case UNI_PAD_U0: // end of UNI_PAD_U2
             first += len;
             break;

        case UNI_PAD_U1: // hex Unicode
        case UNI_PAD_U2: // printable ASCII
             {
               UCS_string ucs;
               read_chars(ucs, first);
               loop(u, ucs.size())   new (C++) CharCell(ucs[u]);
             }
             break;

        case UNI_PAD_U3: // integer
             first += len;
             {
               char * end = 0;
               const long long int val = strtoll((const char *)first, &end, 10);
               new (C++) IntCell(val);
               first = (const UTF8 *)end;
             }
             break;

        case UNI_PAD_U4: // real
             first += len;
             {
               char * end = 0;
               const double val = strtod((const char *)first, &end);
               new (C++) FloatCell(val);
               first = (const UTF8 *)end;
             }
             break;

        case UNI_PAD_U5: // complex
             first += len;
             {
               char * end = 0;
               const double real = strtod((const char *)first, &end);
               first = (const UTF8 *)end;
               Assert(*end == 'J');
               ++end;
               const double imag = strtod(end, &end);
               new (C++) ComplexCell(real, imag);
               first = (const UTF8 *)end;
             }
             break;

        case UNI_PAD_U6: // pointer
             first += len;
             {
               char * end = 0;
               const int vid = strtol((const char *)first, &end, 10);
               Assert(vid >= 0);
               Assert(vid < values.size());
               new (C++) PointerCell(values[vid]);
               first = (const UTF8 *)end;
             }
             break;

        case UNI_PAD_U7: // cellref
             first += len;
             {
               char * end = 0;
               const int vid = strtol((const char *)first, &end, 16);
               Assert(vid >= 0);
               Assert(vid < values.size());
               Assert(*end == '[');   ++end;
               const ShapeItem offset = strtoll(end, &end, 16);
               Assert(*end == ']');   ++end;
               new (C++) LvalCell(&values[vid]->get_ravel(offset));
               first = (const UTF8 *)end;
             }
             break;

        default: Q(type) Q(line) DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_chars(UCS_string & ucs, const UTF8 * & utf)
{
   while (*utf <= ' ')   ++utf;   // skip leading whitespace

   for (bool char_mode = false;;)
       {
         if (*utf == '"')   break;   // end of attribute value

          if (char_mode && *utf < 0x80)
             {
               ucs.append(Unicode(*utf++));
               continue;
             }

         int len;
         const Unicode type = UTF8_string::toUni(utf, len);

         if (type == UNI_PAD_U2)   // start of char_mode
            {
              utf += len;   // skip UNI_PAD_U2
              char_mode = true;
              continue;
            }

         if (type == UNI_PAD_U1)   // start of hex mode
            {
              utf += len;   // skip UNI_PAD_U1
              char_mode = false;
              char * end = 0;
              const int hex = strtol((const char *)utf, &end, 16);
              ucs.append(Unicode(hex));
              utf = (const UTF8 *)end;
              continue;
            }

         if (type == UNI_PAD_U0)   // end of char_mode
            {
              utf += len;   // skip UNI_PAD_U0
              char_mode = false;
              continue;
            }

         break;
       }

   while (*utf <= ' ')   ++utf;   // skip trailing whitespace
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_Ravel()
{
   if (reading_vids)   return;

const int vid = find_int_attr("vid", false, 10);
const UTF8 * cells = find_attr("cells", false);

   Assert(vid < values.size());
Value_P val = values[vid];

   if (!val)   // )COPY with vids_COPY or static value
      {
        return;
      }

const ShapeItem count = val->nz_element_count();
Cell * C = &val->get_ravel(0);
Cell * end = C + count;

   while (C < end)
      {
        read_Cells(C, cells);
        while (*cells <= ' ')   ++cells;   // skip trailing whitespace
      }

   Assert(C == end);   // all cells read

   {
     int len = 0;
     const Unicode next = UTF8_string::toUni(cells, len);
     Assert(next == '"');
   }

   val->check_value(LOC);
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_unused_name(int d, Symbol & symbol)
{
   if (d == 0)   return;   // Symbol::Symbol has already created the top level

   symbol.push();
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_Variable(int d, Symbol & symbol)
{
const unsigned int vid = find_int_attr("vid", false, 10);
   Assert(vid < values.size());

   if (d != 0)   symbol.push();

   // symbol.assign() can fail, for example if symbol is a system variable.
   // inform the user, but continue.
   try
      {
        symbol.assign(values[vid], LOC);
      }
   catch (...)
      {
        if (symbol.get_name() != id_name(ID_QUAD_SYL))
           {
             CERR << "*** Could not assign value " << *values[vid]
                  << "    to variable " << symbol.get_name() << " ***" << endl;
           }
      }
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_Label(int d, Symbol & symbol)
{
const int value = find_int_attr("value", false, 10);
   if (d == 0)   symbol.pop(false);
   symbol.push_label(Function_Line(value));
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_Function(int d, Symbol & symbol)
{
const int native     = find_int_attr("native", true, 10);

   next_tag(LOC);
   expect_tag("UCS", LOC);
const UTF8 * uni = find_attr("uni", false);
   next_tag(LOC);
   expect_tag("/Function", LOC);

UCS_string text;
   while (*uni != '"')   read_chars(text, uni);

int err = 0;
   if (native == 1)
      {
        NativeFunction * nfun = NativeFunction::fix(text, symbol.get_name());
        if (nfun)   // fix succeeded
           {
             if (d == 0)   symbol.pop(false);
             symbol.push_function(nfun);
           }
        else        // fix failed
           {
             CERR << "   *** loading of native function " << text
                  << " failed" << endl << endl;;
             if (d == 0)   symbol.pop(false);
             symbol.push();
           }
      }
   else
      {
        UserFunction * ufun = new UserFunction(text, err, false, LOC);
        Assert(err == -1);

        if (d == 0)   symbol.pop(false);
        symbol.push_function(ufun);
      }
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_Shared_Variable(int d, Symbol & symbol)
{
const SV_key key = find_int_attr("key", false, 10);
   if (d != 0)   symbol.push();

   symbol.share_var(key);
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_SymbolTable()
{
const int size = find_int_attr("size", false, 10);

   loop(s, size)
      {
        next_tag(LOC);
        read_Symbol();
      }

   next_tag(LOC);
   expect_tag("/SymbolTable", LOC);
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_Symbol()
{
   expect_tag("Symbol", LOC);

UCS_string name_ucs;
   {
     const UTF8 * name = find_attr("name",  false);
     const UTF8 * name_end = name;
     while (*name_end != '"')   ++name_end;
     UTF8_string name_utf(name, name_end - name);
     name_ucs = UCS_string(name_utf);
   }

const int depth = find_int_attr("stack-size", false, 10);

   // lookup symbol, trying ⎕xx first
   //
Symbol * symbol = Workspace::lookup_existing_symbol(name_ucs);

   // we do NOT copy if:
   //
   //  )PCOPY and the symbol exists, or 
   //  there is an object list and this symbol is not contained in the list
   //
const bool no_copy = (symbol && protection) || ! is_allowed(name_ucs);

   if (reading_vids)
      {
        // we prepare vids for )COPY or )PCOPY, so we do not create a symbol
        // and care only for the top level
        //
        if (no_copy || (depth == 0))
           {
             //
             COUT << "NOT COPIED: " << name_ucs << endl;
             skip_to_tag("/Symbol");
             return;
           }

        // we have entries and copying is allowed
        //
        next_tag(LOC);
        if (is_tag("Variable"))
           {
             const unsigned int vid = find_int_attr("vid", false, 10);
             vids_COPY.push_back(vid);
           }
        skip_to_tag("/Symbol");
        return;
      }

   if (copying)
      {
        if (no_copy || (depth == 0))
           {
             skip_to_tag("/Symbol");
             return;
           }
      }

   if (symbol == 0)   symbol = Workspace::lookup_symbol(name_ucs);
   Assert(symbol);

   loop(d, depth)
      {
        next_tag(LOC);
        if      (is_tag("unused-name"))       read_unused_name(d, *symbol);
        else if (is_tag("Variable"))          read_Variable(d, *symbol);
        else if (is_tag("Function"))          read_Function(d, *symbol);
        else if (is_tag("Label"))             read_Label(d, *symbol);
        else if (is_tag("Shared-Variable"))   read_Shared_Variable(d, *symbol);
      }

   next_tag(LOC);
   expect_tag("/Symbol", LOC);
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_StateIndicator()
{
   if (copying)
      {
        skip_to_tag("/StateIndicator");
          return;
      }

const int levels = find_int_attr("levels", false, 10);

   loop(l, levels)
      {
        next_tag(LOC);
        expect_tag("SI-entry", LOC);

        read_SI_entry(l + 1);   // levels count from 1 ... levels

        // the parsers loop eats the terminating /SI-entry
      }

   next_tag(LOC);
   expect_tag("/StateIndicator", LOC);
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_SI_entry(int lev)
{
const int level     = find_int_attr("level",     false, 10);
const int pc        = find_int_attr("pc",        false, 10);
const int line      = find_int_attr("line",      false, 10);
const int vid_arg_A = find_int_attr("vid_arg_A", true, 10);
const int Id_arg_F  = find_int_attr("Id_arg_F",  true, 16);
const int vid_arg_B = find_int_attr("vid_arg_B", true, 10);

   next_tag(LOC);
const Executable * exec = 0;
   if      (is_tag("Execute"))        exec = read_Execute();
   else if (is_tag("Statements"))     exec = read_Statement();
   else if (is_tag("UserFunction"))   exec = read_UserFunction();
   else    Assert(0 && "Bad tag at " LOC); 

   Assert(lev == level);
   Assert(exec);

   Workspace::push_SI(exec, LOC);
StateIndicator * si = Workspace::SI_top();
   Assert(si);
   if (Id_arg_F != -1)
      {
        si->eval_arg_F = get_system_function(Id(Id_arg_F));
        if (vid_arg_A != -1)  si->eval_arg_A = values[vid_arg_A];
        if (vid_arg_B != -1)  si->eval_arg_B = values[vid_arg_B];
      }

   read_Parsers(*si);
}
//-----------------------------------------------------------------------------
const Executable *
XML_Loading_Archive::read_Execute()
{
   next_tag(LOC);
   expect_tag("UCS", LOC);
const UTF8 * uni = find_attr("uni", false);
UCS_string text;
   while (*uni != '"')   read_chars(text, uni);
   next_tag(LOC);
   expect_tag("/Execute", LOC);

const Executable * exec = new ExecuteList(text, LOC);
   return exec;
}
//-----------------------------------------------------------------------------
const Executable *
XML_Loading_Archive::read_Statement()
{
   next_tag(LOC);
   expect_tag("UCS", LOC);
const UTF8 * uni = find_attr("uni", false);
UCS_string text;
   while (*uni != '"')   read_chars(text, uni);

   next_tag(LOC);
   expect_tag("/Statements", LOC);

const Executable * exec = new StatementList(text, LOC);
   return exec;
}
//-----------------------------------------------------------------------------
const Executable *
XML_Loading_Archive::read_UserFunction()
{
const int level = find_int_attr("symbol-level", false, 10);
const UTF8 * name = find_attr("ufun-name", false);
const UTF8 * n  = name;
   while (*n != '"')   ++n;
UTF8_string name_utf(name, n - name);
UCS_string name_ucs(name_utf);

Symbol * symbol = Workspace::lookup_symbol(name_ucs);
   Assert(symbol);
   Assert(level >= 0);
   Assert(level < symbol->value_stack_size());
ValueStackItem & vsi = (*symbol)[level];
   Assert(vsi.name_class == NC_FUNCTION);
const Function * fun = vsi.sym_val.function;
   Assert(fun);
const UserFunction * ufun = fun->get_ufun1();
   Assert(fun == ufun);

   return ufun;
}
//-----------------------------------------------------------------------------
void
XML_Loading_Archive::read_Parsers(StateIndicator & si)
{
   next_tag(LOC);
   expect_tag("Parser", LOC);

const int ass_state = find_int_attr("assign-pending", false, 10);
const int lah_high = find_int_attr("lookahead-high", false, 10);

Prefix & parser = si.current_stack;

   parser.set_assign_state((Prefix::Assign_state)ass_state);
   parser.set_lookahead_high(Function_PC(lah_high));

   for (;;)
       {
         Token_loc tloc;
         const bool success = read_Token(tloc);
         if (!success)   break;

         parser.push(tloc);
       }

   next_tag(LOC);
}
//-----------------------------------------------------------------------------
bool
XML_Loading_Archive::read_Token(Token_loc & tloc)
{
   next_tag(LOC);
   if (is_tag("/Parser"))   return false;
   expect_tag("Token", LOC);

   tloc.pc  = Function_PC(find_int_attr("pc", false, 10));

const TokenTag tag = TokenTag(find_int_attr("tag", false, 16));

   switch(tag & TV_MASK)   // cannot call get_ValueType() yet
      {
        case TV_NONE: 
               new (&tloc.tok) Token(tag);
             break;

        case TV_CHAR:
             {
               const Unicode uni = Unicode(find_int_attr("char", false, 10));
               new (&tloc.tok) Token(tag, uni);
             }
             break;

        case TV_INT:   
             {
               const int64_t ival = find_int_attr("int", false, 10);
               new (&tloc.tok) Token(tag, ival);
             }
             break;

        case TV_FLT:   
             {
               const double fval = find_float_attr("float");
               new (&tloc.tok) Token(tag, fval);
             }
             break;

        case TV_CPX:
             {
               const double real = find_float_attr("real");
               const double imag = find_float_attr("imag");
               new (&tloc.tok) Token(tag, real, imag);
             }
             break;

        case TV_SYM:   
             {
               const UTF8 * sym_name = find_attr("sym", false);
               const UTF8 * end = sym_name;
               while (*end != '"')   ++end;
               UTF8_string name_utf(sym_name, end - sym_name);
               UCS_string name_ucs(name_utf);
               Symbol * symbol = Workspace::lookup_symbol(name_ucs);
               new (&tloc.tok) Token(tag, symbol);
             }
             break;

        case TV_LIN:   
             {
               const int ival = find_int_attr("line", false, 10);
               new (&tloc.tok) Token(tag, Function_Line(ival));
             }
             break;

        case TV_VAL:   
             {
               const unsigned int vid = find_int_attr("vid", false, 10);
               Assert(vid < values.size());
               new (&tloc.tok) Token(tag, values[vid]);
             }
             break;

        case TV_INDEX: 
             {
               const unsigned int rank = find_int_attr("rank", false, 10);
               char * vids = (char *)find_attr("index", false);
               IndexExpr & idx = *new IndexExpr(false, LOC);
               while (*vids != '"')
                  {
                    if (*vids == ',')   ++vids;
                    if (*vids == '-')   // elided index
                       {
                         idx.add(Value_P());
                       }
                    else                // value
                       {
                         Assert1(*vids == 'v');   ++vids;
                         Assert1(*vids == 'i');   ++vids;
                         Assert1(*vids == 'd');   ++vids;
                         Assert1(*vids == '_');   ++vids;
                         char * end;
                         const unsigned int vid = strtol(vids, &vids, 10);
                         Assert(vid < values.size());
                         idx.add(values[vid]);
                       }
                  }
               new (&tloc.tok) Token(tag, idx);
             }
             break;

        case TV_FUN:
             {
               const UTF8 * fun_name = find_attr("ufun-name", true);

               if (fun_name == 0)   // primitive or Quad function
                  {
                    const int fun_id = find_int_attr("fun-id", false, 16);
                    Function * sysfun = get_system_function(Id(fun_id));
                    new (&tloc.tok) Token(tag, sysfun);
                  }
               else
                  {
                    const int level = find_int_attr("symbol-level", false, 10);

                    const UTF8 * end = fun_name;
                    while (*end != '"')   ++end;
                    UTF8_string name_utf(fun_name, end - fun_name);
                    UCS_string ucs(name_utf);
                    const Symbol & symbol = *Workspace::lookup_symbol(ucs);
                    Assert(level >= 0);
                    Assert(level < symbol.value_stack_size());
                    const ValueStackItem & vsi = symbol[level];
                    Assert(vsi.name_class == NC_FUNCTION);
                    Function * fun = vsi.sym_val.function;
                    Assert(fun);
                    new (&tloc.tok) Token(tag, fun);
                  }
             }
             break;

        default: FIXME;
      }

   return true;
}
//=============================================================================

