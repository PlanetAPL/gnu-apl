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


#include "Avec.hh"
#include "Input.hh"
#include "Logging.hh"
#include "Nabla.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "Symbol.hh"
#include "UserFunction.hh"
#include "Workspace.hh"

#define NABLA_ERROR throw_edit_error(LOC)

//-----------------------------------------------------------------------------
void
Nabla::edit_function(const UCS_string & cmd)
{
Nabla nabla(cmd);
   nabla.edit();
}
//-----------------------------------------------------------------------------
Nabla::Nabla(const UCS_string & cmd)
   : fun_symbol(0),
     ecmd(ECMD_NOP),
     edit_from(-1),
     edit_to(-1),
     function_existed(true),
     do_close(false),
     locked(false),
     first_command(cmd),
     current_line(1)
{
}
//-----------------------------------------------------------------------------
void
Nabla::throw_edit_error(const char * loc)
{
   COUT << "DEFN ERROR" << endl
        << "      " << first_command << endl
        << "      " << UCS_string(first_command.size() - 1, UNI_ASCII_SPACE)
        << "^" << endl;

   throw_define_error(fun_name, first_command, loc);
}
//-----------------------------------------------------------------------------
void
Nabla::edit()
{
   if (const char * loc = start())
      {
        CERR << "bad editor command '" << first_command
             << "' : problem '" << loc << "'" << endl;
       NABLA_ERROR;
      }

   // editor loop
   //
   Log(LOG_nabla)   CERR << "Nabla(" << fun_name << ")..." << endl;
   while (!do_close)
       {
         const UCS_string prompt = current_line.print_prompt();
         const char * line = Input::get_user_line_nabla(&prompt);
         const UTF8_string utf(line);
         const UCS_string ucs(utf);
         if (const char * loc = parse_oper(ucs, false))
            {
              CERR << "???" << endl;
              continue;
            }

         if (const char * loc = execute_oper())
            CERR << "execute_oper() failed at " << loc << endl;;
       }

   Log(LOG_nabla)
      CERR << "done '" << fun_name << "'" << endl;

UCS_string fun_text;
   loop(l, lines.size())
      {
        fun_text.append(lines[l].text);
        fun_text.append(UNI_ASCII_LF);
      }

int error_line = 0;
UserFunction * ufun = UserFunction::fix(fun_text, error_line, false, LOC);
   if (ufun == 0)   NABLA_ERROR;

   if (locked)
      {
         const int exec_properties[4] = { 1, 1, 1, 1 };
         ufun->set_exec_properties(exec_properties);
      }
}
//-----------------------------------------------------------------------------
const char *
Nabla::start()
{
   // cmd should be something like:
   //
   // ∇FUN
   // ∇FUN[⎕]
   // ∇FUN[⎕]∇
   // etc.
   //
UCS_string::iterator c(first_command.begin());

   // skip leading nabla.
   //
   if (c.next() != UNI_NABLA)   return LOC;

   // skip leading spaces
   //
   while (c.more() && c.get() == UNI_ASCII_SPACE)   c.next();

   // function name or header.
   //
   while (c.more() && c.get() != UNI_ASCII_L_BRACK)   fun_name.append(c.next());

   // at this point there could be an axis specification [X] that
   // could be confused with an operation like [⎕]. We take the first char
   // after the [ to decide if there is an axis or an operation.
   //
   if (c.get() == UNI_ASCII_L_BRACK)   // [
      {
        for (int off = 1; ; ++off)
            {
              if (c.get(off) <= UNI_ASCII_SPACE)   continue; 
              if (Avec::is_first_symbol_char(c.get(off)))   // axis
                 {
                   fun_name.append(c.next());   //  copy the [
                   while (c.more() && c.get() != UNI_ASCII_L_BRACK)
                         fun_name.append(c.next());
                 }
              break;
            }
      }

   if (fun_name.size() == 0)   return LOC;   // no function name

   // optional operation
   //
   if (c.get() == UNI_ASCII_L_BRACK)
      {
        UCS_string oper;
        Unicode cc;
        do oper.append(cc = c.next());   while (cc != UNI_ASCII_R_BRACK);
        if (const char * loc = parse_oper(oper, true))   return loc;   // error
      }

   if (const char * loc = open_function())   return loc;

   // immediate close (only show command is allowed here),
   // e.g. ∇fun[⎕]∇
   //
   if (c.get() == UNI_NABLA)
      {
        if (ecmd != ECMD_SHOW)   return LOC;
        if (const char * loc = execute_oper())
           CERR << "execute_oper() failed at " << loc << endl;
        do_close = true;
        return 0;
      }

   if (const char * loc = execute_oper())
      CERR << "execute_oper() failed at " << loc << endl;

   return 0;   // no error
}
//-----------------------------------------------------------------------------
const char *
Nabla::parse_oper(const UCS_string & oper, bool initial)
{
   Log(LOG_nabla)
      CERR << "parsing oper '" << oper << "'" << endl;

   current_text.clear();
   ecmd = ECMD_NOP;

UCS_string::iterator c(oper.begin());
Unicode cc = c.next();
   while (cc == ' ')   cc = c.next();

   // we expect one of the following:
   //
   // [⎕] [n⎕] [⎕m] [n⎕m] [⎕n-m]                    (show)
   //     [n∆] [∆m] [n∆m] [∆n-m] [∆n1 n2 ...]       (delete)
   // [→]                                           (escape)
   // [n]                                           (goto)
   // text                                          (override text)

   if (cc == UNI_NABLA)       // initial ∇
      {
        do_close = true;
        return 0;
      }

   if (cc == UNI_DEL_TILDE)   // initial ⍫
      {
        locked = true;
        do_close = true;
        return 0;
      }

   if (cc != UNI_ASCII_L_BRACK)
      {
        ecmd = ECMD_EDIT;
        edit_from = current_line;
        for (; cc != Invalid_Unicode; cc = c.next())   current_text.append(cc);
        return 0;
      }

   // a loop over multiple commands, like
   // [2⎕4] [∆5]
   //
   // only the last command is executed; the previous commands are discarded/
   //
command_loop:

   // at this point, [ was seen and skipped

   ecmd = ECMD_NOP;
   edit_from.clear();
   edit_to.clear();
   got_minus = false;

   // optional edit_from
   //
   if (Avec::is_digit(c.get()))   edit_from = parse_lineno(c);

   // operation, which is one of:
   //
   // [⎕   show
   // []   edit
   // [∆   delete
   // [→   abandon
   switch (c.get())
      {
        case UNI_QUAD_QUAD:
        case UNI_QUAD_QUAD1:    ecmd = ECMD_SHOW;     c.next();   break;
        case UNI_ASCII_R_BRACK: ecmd = ECMD_EDIT;                 break;
        case UNI_DELTA:         ecmd = ECMD_DELETE;   c.next();   break;
        case UNI_RIGHT_ARROW:   ecmd = ECMD_ESCAPE;   c.next();   break;
        case Invalid_Unicode:   return LOC;

        default: CERR << "Bad edit op '" << c.get() << "'" << endl;
                 return LOC;
      }

   // don't allow delete or escape in the first command
   if (initial)
      {
        if (ecmd == ECMD_DELETE)   return LOC;
        if (ecmd == ECMD_ESCAPE)   return LOC;
      }

again:
   // optional edit_to
   //
   if (Avec::is_digit(c.get()))   edit_to = parse_lineno(c);

   if (c.get() == UNI_ASCII_MINUS)   // range
      {
        if (got_minus)   return "error: second -  at " LOC;
        got_minus = true;
        edit_from = edit_to;
        c.next();   // eat the -
        goto again;
      }

   if (c.next() != UNI_ASCII_R_BRACK)   return LOC;

   // at this point we have parsed an editor command, like:
   //
   // [from ⎕ to]
   // [from ∆ to]
   // [from]

   while (c.get() == UNI_ASCII_SPACE)   c.next();

   if (c.get() == UNI_ASCII_L_BRACK)   // another command: ignore previous
      {
         c.next();   // eat the [
         goto command_loop;
      }

   // copy the rest to current_text. Set do_close if ∇ or ⍫ is seen
   // unless inside strings.
   //
   for (;;)
      {
        switch(cc = c.next())
           {
             case Invalid_Unicode:     // regular end of input
                  return 0;

             case UNI_NABLA:           // ∇
                  do_close = true;
                  return 0;

             case UNI_DEL_TILDE:       // ⍫
                  locked = true;
                  do_close = true;
                  return 0;

             case UNI_ASCII_DOUBLE_QUOTE:  // "
                  current_text.append(cc);
                  for (;;)
                      {
                        cc = c.next();
                        if (cc == Invalid_Unicode)   // premature end of input
                           {
                             current_text.append(UNI_ASCII_DOUBLE_QUOTE);
                             return 0;
                           }

                        current_text.append(cc);
                        if (cc == UNI_ASCII_DOUBLE_QUOTE)   break; // string end
                        if (cc == UNI_ASCII_BACKSLASH)      // \x
                           {
                             cc = c.next();
                             if (cc == Invalid_Unicode)   // premature input end
                                {
                                  current_text.append(UNI_ASCII_BACKSLASH);
                                  current_text.append(UNI_ASCII_DOUBLE_QUOTE);
                                  return 0;
                                }
                             current_text.append(cc);
                           }
                      }
                  break;

             case UNI_SINGLE_QUOTE:    // '
                  current_text.append(cc);
                  for (;;)
                      {
                        // no need to care for ''. Since we only copy, we can
                        // handle ' ... '' ... ' like two adjacent strings
                        // instead of a string containing a (doubled) quote.
                        //
                        cc = c.next();
                        if (cc == Invalid_Unicode)   // premature end of input
                           {
                             current_text.append(UNI_SINGLE_QUOTE);
                             return 0;
                           }

                        current_text.append(cc);
                        if (cc == UNI_SINGLE_QUOTE)      break;   // string end
                      }
                  break;

             default:
                current_text.append(cc);
           }
      }

   return 0;
}
//-----------------------------------------------------------------------------
LineLabel
Nabla::parse_lineno(UCS_string::iterator & c)
{
LineLabel ret(0);

   while (Avec::is_digit(c.get()))
      {
        ret.ln_major *= 10;
        ret.ln_major += c.next() - UNI_ASCII_0;
      }

   if (c.get() == UNI_ASCII_FULLSTOP)
      {
        c.next();   // eat the .
        while (Avec::is_digit(c.get()))   ret.ln_minor.append(c.next());
      }

   return ret;
}
//-----------------------------------------------------------------------------
const char *
Nabla::open_function()
{
   // open an existing function according to fun_name, or create a new function.

   // there are two cases when a new function is created:
   //
   // 1. the header line contains at least one non-symbol char, or
   // 2. the header consists of only symbol chars and a function with
   //    that name exists.
   //
   // Examples:
   //
   // 1.   R←A ANY_FUNCTION B
   // 2.   EXISTING_FUNCTION
   //
   loop(f, fun_name.size())
      {
        if (!Avec::is_symbol_char(fun_name[f]))
           {
             return open_new_function();
           }
      }

   // all chars are symbol chars.
   // They may or may not relate to an existing symbol.
   //
Symbol * fsym = Workspace::lookup_existing_symbol(fun_name);
   if (fsym == 0)   return open_new_function();

   // existing symbol: check that function is not on the SI stack.
   //
   if (Workspace::is_called(fun_name))
      {
        UCS_string & t4 = Workspace::more_error();
        t4.clear();
        t4.append_utf8("function ");
        t4.append(fun_name);
        t4.append_utf8(" could not be edited, since "
                       "it is used on the )SI stack.");
        return LOC;
      }

   if (fsym->get_nc() == NC_UNUSED_USER_NAME)   return open_new_function();

   return open_existing_function(fsym);
}
//-----------------------------------------------------------------------------
const char *
Nabla::open_existing_function(Symbol * fsym)
{
   Log(LOG_nabla)
      CERR << "opening existing function '" << fun_name << "'" << endl;

const NameClass nc = fsym->get_nc();
   if (nc != NC_FUNCTION && nc != NC_OPERATOR)
      {
        Log(LOG_nabla)
           CERR << "symbol '" << fun_name << "' has name class " << nc
                << " (not function or operator)" << endl;
        return LOC;
      }

Function * function = fsym->get_function();
   Assert(function);

   if (function->get_exec_properties()[0])   NABLA_ERROR;   // locked

const UCS_string ftxt = function->canonical(false);
   Log(LOG_nabla)
      CERR << "function is:\n" << ftxt << endl;

vector<UCS_string> tlines;
   ftxt.to_vector(tlines);

   loop(t, tlines.size())   lines.push_back(FunLine(t, tlines[t]));

   current_line = LineLabel(tlines.size());
   return 0;
}
//-----------------------------------------------------------------------------
const char *
Nabla::open_new_function()
{
   Log(LOG_nabla)
      CERR << "creating new function '" << fun_name << "'" << endl;

int error_line = 0;
UserFunction * ufun = UserFunction::fix(fun_name, error_line, true, LOC);

   if (ufun == 0)   return "Bad function header at " LOC;
   fun_name = ufun->get_name();

   Log(LOG_nabla)
      CERR << "created new function '" << fun_name << "'" << endl;
   function_existed = false;

   return open_function();
}
//-----------------------------------------------------------------------------
const char *
Nabla::execute_oper()
{
const bool have_from = edit_from.ln_major != -1;
const bool have_to = edit_to.ln_major != -1;

   if (!have_from)   edit_from = lines[0].label;
   if (!have_to)     edit_to = lines[lines.size() - 1].label;
   if (edit_to.ln_major > lines.size() - 1)
      { edit_to.ln_major = lines.size() - 1;   edit_to.ln_minor.clear(); };

   if (ecmd == ECMD_NOP)
      {
        Log(LOG_nabla)
           CERR << "Nabla::execute_oper(NOP)" << endl;
        return 0;
      }

   if (ecmd == ECMD_SHOW)     return execute_show();
   if (ecmd == ECMD_DELETE)   return execute_delete();
   if (ecmd == ECMD_EDIT)     return execute_edit();
   if (ecmd == ECMD_ESCAPE)   return execute_escape();

   CERR << "edit command " << ecmd
        << " from " << edit_from << " to " << edit_to << endl;
   FIXME;

   return LOC;
}
//-----------------------------------------------------------------------------
const char *
Nabla::execute_show()
{
   Log(LOG_nabla)
      CERR << "Nabla::execute_oper(SHOW) from "
           << edit_from << " to " << edit_to << " line-count " << lines.size() << endl;
int idx_from = find_line(edit_from);
int idx_to   = find_line(edit_to);

   if (idx_from == -1)   edit_from = lines[idx_from = 0].label;
   if (idx_to == -1)     edit_to   = lines[idx_to = lines.size() - 1].label;

   Log(LOG_nabla)
      CERR << "Nabla::execute_oper(SHOW) from "
           << edit_from << " to " << edit_to << endl;

   for (int e = idx_from; e <= idx_to; ++e)   lines[e].print(COUT);
   current_line = lines.back().label;
   current_line.next();
   return 0;
}
//-----------------------------------------------------------------------------
const char *
Nabla::execute_delete()
{
   // for delete we want exact numbers.
   //
const int idx_to = find_line(LineLabel(edit_to));
   if (idx_to == -1)   return "No/bad edit_to at " LOC;

   if (edit_from == -1)   // delete single line
      {
        lines.erase(lines.begin() + idx_to);
        return 0;
      }

   // delete multiple lines
   //
const int idx_from = find_line(LineLabel(edit_from));
   if (idx_from == -1)       return "No/bad edit_from at " LOC;
   if (idx_from >= idx_to)   return "idx_from >= idx_to at " LOC;

   lines.erase(lines.begin() + idx_from, lines.begin() + idx_to);
   current_line = lines.back().label;
   return 0;
}
//-----------------------------------------------------------------------------
const char *
Nabla::execute_edit()
{
   // if the user has not specified a line then edit at current_line
   //
   if (edit_from.ln_major == -1)   edit_from = current_line;

   current_line = edit_from;

const int idx_from = find_line(edit_from);

   Assert(lines.size() > 0);
   if (idx_from == -1)   // new line
      {
        if (!current_text.size())   return 0;   // no text: done

        // find the largest label before edit_from (if any)
        //
        int before_idx = -1;
        for (int i = 0; i < lines.size(); ++i)
            {
              if (lines[i].label < edit_from)   before_idx = i;
              else                                 break;
            }

        FunLine fl(edit_from, current_text);
        lines.insert(lines.begin() + before_idx + 1, fl);
        current_line.next();
        return 0;
      }


   // replace line
   //
   if (!current_text.size())   return 0;   // no text: done

   lines[idx_from].text = current_text;
   current_line.next();
   return 0;
}
//-----------------------------------------------------------------------------
const char *
Nabla::execute_escape()
{
   lines.clear();
   open_function();
}
//-----------------------------------------------------------------------------
int
Nabla::find_line(const LineLabel & lab)
{
   if (lab.ln_major == -1)   return -1;

   loop(l, lines.size())   if (lab == lines[l].label)   return l;

   return -1;   // not found.
}
//-----------------------------------------------------------------------------
void
Nabla::FunLine::print(ostream & out) const
{
   label.print(out);
   out << " " << text << endl;
}
//-----------------------------------------------------------------------------
void
LineLabel::print(ostream & out) const
{
   out << "[" << ln_major;
   if (ln_minor.size())   out << "." << ln_minor;
   out << "]";
}
//-----------------------------------------------------------------------------
UCS_string
LineLabel::print_prompt() const
{
UCS_string ret("[");
   ret.append_number(ln_major);

   if (ln_minor.size())
      {
        ret.append(Unicode('.'));
        loop(s, ln_minor.size())   ret.append(Unicode(char(ln_minor[s])));
      }

   ret.append(Unicode(']'));
   ret.append(Unicode(' '));
   return ret;
}
//-----------------------------------------------------------------------------
void
LineLabel::next()
{
   if (ln_minor.size() == 0)   // full number: add 1
      {
        ++ln_major;
        return;
      }

   // fract number: increment last fract digit
   //
const Unicode cc = ln_minor[ln_minor.size() - 1];
   if (cc != UNI_ASCII_9)   ln_minor[ln_minor.size() - 1] = Unicode(cc + 1);
   else                     ln_minor.append(UNI_ASCII_1);
}
//-----------------------------------------------------------------------------
bool
LineLabel::operator ==(const LineLabel & other) const
{
   return (ln_major == other.ln_major) && (ln_minor == other.ln_minor);
}
//-----------------------------------------------------------------------------
bool
LineLabel::operator <(const LineLabel & other) const
{
   if (ln_major != other.ln_major)   return ln_major < other.ln_major;
 return  ln_minor.compare(other.ln_minor) < 0;
}
//-----------------------------------------------------------------------------
ostream &
operator <<(ostream & out, const LineLabel & lab)
{
   lab.print(out);
   return out;
}
//-----------------------------------------------------------------------------

