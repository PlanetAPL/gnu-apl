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

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#include "CharCell.hh"
#include "ComplexCell.hh"
#include "Command.hh"
#include "Executable.hh"
#include "FloatCell.hh"
#include "IndexExpr.hh"
#include "IntCell.hh"
#include "Input.hh"
#include "LibPaths.hh"
#include "main.hh"
#include "Nabla.hh"
#include "Output.hh"
#include "Parser.hh"
#include "Prefix.hh"
#include "Quad_TF.hh"
#include "StateIndicator.hh"
#include "Svar_DB.hh"
#include "Symbol.hh"
#include "TestFiles.hh"
#include "Tokenizer.hh"
#include "UserFunction.hh"
#include "Value.hh"
#include "Workspace.hh"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

//-----------------------------------------------------------------------------
void
Command::process_line()
{
UCS_string line = Input::get_line();   // get_line() removes leading whitespace

   if (line.size() > 0)   process_line(line);
}
//-----------------------------------------------------------------------------
void
Command::process_line(UCS_string & line)
{

   switch(line[0])
      {
         case UNI_ASCII_R_PARENT:      // regular command, e.g. )SI
         case UNI_ASCII_R_BRACK:       // debug command, e.g. ]LOG
              {
                ostream & out = (line[0] == UNI_ASCII_R_PARENT) ? COUT : CERR;

                // split line into command and arguments
                //
                UCS_string cmd;   // the command without arguments
                int len = 0;
                line.copy_black(cmd, len);
                UCS_string arg(line, len, line.size() - len);
                vector<UCS_string> args;
                arg.copy_black_list(args);
                line.clear();
#define cmd_def(cmd_str, code, _arg) \
                if (cmd.starts_iwith(cmd_str)) { code } else
#include "Command.def"
                   {
                     out << _("BAD COMMAND") << endl;
                     return;
                   }

                if (line.size() == 0)   return;

                // if line is non-empty then is it ⎕LX of )LOAD
                //
              }
              break;   // continue below

         case UNI_NABLA:               // e.g. ∇FUN
              Nabla::edit_function(line);
              return;

         case UNI_ASCII_NUMBER_SIGN:   // e.g. # comment
         case UNI_COMMENT:             // e.g. ⍝ comment
              return;

        default: /* continue below */ ;
      }

   // at this point, line should be a statement list. Parse it...
   //
const Executable * statements = 0;
   try
      {
        statements = StatementList::fix(line, LOC);
      }
   catch (Error err)
      {
        COUT << _("SYNTAX ERROR") << endl;
        if (err.get_error_line_2().size())
           {
             COUT << "      " << err.get_error_line_2() << endl
                  << "      " << err.get_error_line_3() << endl;
           }

        err.print(CERR);
        return;
      }
   catch (...)
      {
        CERR << "*** Command::process_line() caught other exception ***"
             << endl;
        return;
      }

   if (statements == 0)
      {
        COUT << _("main: Parse error.") << endl;
        return;
      }

   // At this point, the user command was parsed correctly.
   // check for Escape (→)
   //
   {
     const Token_string & body = statements->get_body();
     if (body.size() == 3                &&
         body[0].get_tag() == TOK_ESCAPE &&
         body[1].get_Class() == TC_END   &&
         body[2].get_tag() == TOK_RETURN_STATS)
        {
          delete statements;

          // remove all SI entries up to (including) the next immediate
          // execution context
          //
          for (bool goon = true; goon;)
              {
                StateIndicator * si = Workspace::SI_top();
                if (si == 0)   break;   // SI empty

                const Executable * exec = si->get_executable();
                Assert(exec);
                goon = exec->get_parse_mode() != PM_STATEMENT_LIST;
                si->escape();   // pop local vars of user defined functions
                Workspace::pop_SI(LOC);
              }
          return;
        }
   }

// statements->print(CERR);

   // push a new context for the statements.
   //
   Workspace::push_SI(statements, LOC);

   for (;;)
       {
         //
         // NOTE that the entire SI may change while executing this loop.
         // We should therefore avoid references to SI entries.
         //
         Token token = Workspace::SI_top()
                                  ->get_executable()->execute_body();

// Q(token)

         // start over if execution has pushed a new context
         //
         if (token.get_tag() == TOK_SI_PUSHED)   continue;

         // maybe call EOC handler and repeat if true returned
         //
         if (Workspace::SI_top()->call_eoc_handler(token))
            continue;

         // the far most frequent cases are TC_VALUE and TOK_VOID
         // so we handle them first.
         //
         if (token.get_Class() == TC_VALUE || token.get_tag() == TOK_VOID )
            {
              if (Workspace::SI_top()->get_executable()
                             ->get_parse_mode() == PM_STATEMENT_LIST)
                 {
                   if (attention_raised)
                      {
                        attention_raised = false;
                        interrupt_raised = false;
                        ATTENTION;
                      }

                   break;   // will return to calling context
                 }

              Workspace::pop_SI(LOC);

              // we are back in the calling SI. There should be a TOK_SI_PUSHED
              // token at the top of stack. Replace it with the result from
              //  the called (just poped) SI.
              //
              {
                Prefix & prefix =
                         Workspace::SI_top()->get_prefix();
                Assert(prefix.at0().get_tag() == TOK_SI_PUSHED);

                copy_1(prefix.tos().tok, token, LOC);
              }
              if (attention_raised)
                 {
                   attention_raised = false;
                   interrupt_raised = false;
                   ATTENTION;
                 }

              continue;
            }

         if (token.get_tag() == TOK_BRANCH)
            {
              StateIndicator * si = Workspace::SI_top_fun();

              if (si == 0)
                 {
                    Workspace::more_error() = UCS_string(
                          _("empty branch (→⍳0) without suspended function"));
                    SYNTAX_ERROR;   // →N without function,
                 }

              // pop contexts above defined function
              //
              while (si != Workspace::SI_top())
                    Workspace::pop_SI(LOC);

              const Function_Line line = Function_Line(token.get_int_val());

              if (line == Function_Retry)   si->retry(LOC);
                else                        si->goon(line, LOC);
              continue;
            }

         if (token.get_tag() == TOK_ESCAPE)
            {
              // remove all SI entries up to (including) the next immediate
              // execution context
              //
              for (bool goon = true; goon;)
                  {
                    StateIndicator * si = Workspace::SI_top();
                    if (si == 0)   break;   // SI empty

                    const Executable * exec = si->get_executable();
                    Assert(exec);
                    goon = exec->get_parse_mode() != PM_STATEMENT_LIST;
                    si->escape();   // pop local vars of user defined functions
                    Workspace::pop_SI(LOC);
              }
              return;

              Assert(0 && "not reached");
            }

         if (token.get_tag() == TOK_ERROR)
            {
              // clear attention and interrupt flags
              //
              attention_raised = false;
              interrupt_raised = false;

              Workspace::get_error()->print(CERR);
              if (Workspace::SI_top()->get_level() == 0)
                 {
                   Value::erase_stale(LOC);
                   IndexExpr::erase_stale(LOC);
                 }
              return;
            }

         // we should not come here.
         //
         Q(token)  Q(token.get_Class())  Q(token.get_tag())  FIXME;
       }

   // pop the context for the statements
   //
   Workspace::pop_SI(LOC);
}
//-----------------------------------------------------------------------------
void 
Command::cmd_CHECK(ostream & out)
{
   // erase stale functions from failed ⎕EX
   //
   {
     bool erased = false;
     const int stale = Workspace::cleanup_expunged(CERR, erased);
     if (stale)
        {
          char cc[200];
          snprintf(cc, sizeof(cc), _("WARNING - %d stale functions (%serased)"),
                   stale, (erased ? "" : "not "));
          CERR << cc << endl;
        }
     else CERR << _("OK      - no stale functions") << endl;
   }

   {
     const int stale = Value::print_stale(CERR);
     if (stale)
        {
          char cc[200];
          snprintf(cc, sizeof(cc), _("ERROR   - %d stale values"), stale);
          out << cc << endl;
        }
     else out << _("OK      - no stale values") << endl;
   }
   {
     const int stale = IndexExpr::print_stale(CERR);
     if (stale)
        {
          char cc[200];
          snprintf(cc, sizeof(cc), _("ERROR   - %d stale indices"), stale);
          out << cc << endl;
        }
     else out << _("OK      - no stale indices") << endl;
   }
}
//-----------------------------------------------------------------------------
void 
Command::cmd_CONTINUE(ostream & out)
{
   Workspace::wsid(out, UCS_string("CONTINUE"));

vector<UCS_string> vcont;
   Workspace::save_WS(out, vcont);
   cmd_OFF(0);
}
//-----------------------------------------------------------------------------
void 
Command::cmd_DROP(ostream & out, const vector<UCS_string> & lib_ws)
{
   // )DROP wsname
   // )DROP libnum wsname


   // check number of arguments (1 or 2)
   //
   if (lib_ws.size() == 0)   // missing argument
      {
        out << _("BAD COMMAND") << endl;

        UCS_string & t4 = Workspace::more_error();
        t4 = UCS_string(_("missing workspace name in command )DROP"));
        return;
      }

   if (lib_ws.size() > 2)   // too many arguments
      {
        out << _("BAD COMMAND") << endl;
        UCS_string & t4 = Workspace::more_error();

        t4 = UCS_string(_("too many parameters in command )DROP"));
        return;
      }

   // at this point, lib_ws.size() is 1 or 2. If 2 then the first
   // is the lib number
   //
LibRef libref = LIB_NONE;
UCS_string wname = lib_ws.back();
   if (lib_ws.size() == 2)   libref = (LibRef)(lib_ws.front().atoi());
UTF8_string filename = LibPaths::get_lib_filename(libref, wname, true, "xml");

int result = unlink(filename.c_str());
   if (result)
      {
        out << wname << _(" NOT DROPPED: ") << strerror(errno) << endl;
        UCS_string & t4 = Workspace::more_error();
        t4.clear();
        t4.append_ascii(_("could not unlink file "));
        t4.append_utf8(filename.c_str());
      }
}
//-----------------------------------------------------------------------------
void
Command::cmd_ERASE(ostream & out, vector<UCS_string> & args)
{
   Workspace::erase_symbols(CERR, args);
}
//-----------------------------------------------------------------------------
void 
Command::cmd_KEYB()
{
   CERR << _("US Keyboard Layout:") <<      "\n"
                                            "\n"
"╔════╦════╦════╦════╦════╦════╦════╦════╦════╦════╦════╦════╦════╦═════════╗\n"
"║ ~⍨ ║ !¡ ║ @€ ║ #£ ║ $⍧ ║ %  ║ ^  ║ &  ║ *⍂ ║ (⍱ ║ )⍲ ║ _≡ ║ +⌹ ║         ║\n"
"║ `◊ ║ 1¨ ║ 2¯ ║ 3< ║ 4≤ ║ 5= ║ 6≥ ║ 7> ║ 8≠ ║ 9∨ ║ 0∧ ║ -× ║ =÷ ║ BACKSP  ║\n"
"╠════╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦══════╣\n"
"║       ║ Q¿ ║ W⌽ ║ E⍷ ║ R  ║ T⍉ ║ Y¥ ║ U  ║ I⍸ ║ O⍥ ║ P⍟ ║ {  ║ }⍬ ║  |⍀  ║\n"
"║  TAB  ║ q? ║ w⍵ ║ e∈ ║ r⍴ ║ t∼ ║ y↑ ║ u↓ ║ i⍳ ║ o○ ║ p⋆ ║ [← ║ ]→ ║  \\⍝  ║\n"
"╠═══════╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩══════╣\n"
"║ (CAPS   ║ A⊖ ║ S  ║ D  ║ F⍫ ║ G⍒ ║ H⍋ ║ J⍤ ║ K⌺ ║ L⍞ ║ :  ║ \"  ║         ║\n"
"║  LOCK)  ║ a⍺ ║ s⌈ ║ d⌊ ║ f_ ║ g∇ ║ h∆ ║ j∘ ║ k' ║ l⎕ ║ ;⊢ ║ '⊣ ║ RETURN  ║\n"
"╠═════════╩═══╦╩═══╦╩═══╦╩═══╦╩═══╦╩═══╦╩═══╦╩═══╦╩═══╦╩═══╦╩═══╦╩═════════╣\n"
"║             ║ Z  ║ X  ║ C⍝ ║ V  ║ B⍎ ║ N⍕ ║ M⌶ ║ <⍪ ║ >⍙ ║ ?⌿ ║          ║\n"
"║  SHIFT      ║ z⊂ ║ x⊃ ║ c∩ ║ v∪ ║ b⊥ ║ n⊤ ║ m| ║ ,⌷ ║ .⍎ ║ /⍕ ║  SHIFT   ║\n"
"╚═════════════╩════╩════╩════╩════╩════╩════╩════╩════╩════╩════╩══════════╝\n"
   << endl;
}
//-----------------------------------------------------------------------------
void 
Command::cmd_HELP(ostream & out)
{
   out << _("Commands are:") << endl;
#define cmd_def(cmd_str, _cod, arg) \
   CERR << "       " cmd_str " " #arg << endl;
#include "Command.def"
   out << endl;
}
//-----------------------------------------------------------------------------
void
Command::cmd_HOST(ostream & out, const UCS_string & arg)
{
UTF8_string host_cmd(arg);
FILE * pipe = popen(host_cmd.c_str(), "r");
   if (pipe == 0)   // popen failed
      {
        out << _(")HOST command failed: ") << strerror(errno) << endl;
        return;
      }

   for (;;)
       {
         const int cc = fgetc(pipe);
         if (cc == EOF)   break;
         out << (char)cc;
       }

int result = pclose(pipe);
   out << endl << IntCell(result) << endl;
}
//-----------------------------------------------------------------------------
void
Command::cmd_IN(ostream & out, vector<UCS_string> & args, bool protection)
{
   if (args.size() == 0)
      {
        out << _("BAD COMMAND") << endl;
        Workspace::more_error() =
                   UCS_string(_("missing filename in command )IN"));
        return;
      }

UCS_string fname = args.front();
   args.erase(args.begin());

UTF8_string filename = LibPaths::get_lib_filename(LIB_NONE, fname, true, "atf");

FILE * in = fopen(filename.c_str(), "r");
   if (in == 0)   // open failed: try filename.atf unless already .atf
      {
        UTF8_string fname_utf8(fname);
        CERR << ")IN " << fname_utf8.c_str()
             << _(" failed: ") << strerror(errno) << endl;

        char cc[200];
        snprintf(cc, sizeof(cc),
                 _("command )IN: could not open file %s for reading: %s"),
                 fname_utf8.c_str(), strerror(errno));
        Workspace::more_error() = UCS_string(cc);
        return;
      }

UTF8 buffer[80];
int idx = 0;

transfer_context tctx(protection);

bool new_record = true;

   for (;;)
      {
        const int cc = fgetc(in);
        if (cc == EOF)   break;
        if (idx == 0 && cc == 0x0A)   // optional LF
           {
             // CERR << "CRLF" << endl;
             continue;
           }

        if (idx < 80)
           {
              if (idx < 72)   buffer[idx++] = cc;
              else            buffer[idx++] = 0;
             continue;
           }

        if (cc == 0x0D || cc == 0x15)   // ASCII or EBCDIC
           {
             tctx.is_ebcdic = (cc == 0x15);
             tctx.process_record(buffer, args);

             idx = 0;
             ++tctx.recnum;
             continue;
           }

        CERR << _("BAD record charset (neither ASCII nor EBCDIC)") << endl;
        break;
      }

   fclose(in);
}
//-----------------------------------------------------------------------------
void 
Command::cmd_LIBS(ostream & out, const vector<UCS_string> & lib_ref)
{
   // Command is:
   //
   // )LIB path           (set root to path)
   // )LIB                (display root and path states)
   //
   if (lib_ref.size() > 0)   // set path
      {
        UTF8_string utf(lib_ref[0]);
        LibPaths::set_APL_lib_root(utf.c_str());
        out << "LIBRARY ROOT SET TO " << lib_ref[0] << endl;
        return;
      }

   out << "Library root: " << LibPaths::get_APL_lib_root() << 
"\n"
"\n"
"Library reference number mapping:\n"
"\n"
"---------------------------------------------------------------------------\n"
"Ref Conf  Path                                                State   Err\n"
"---------------------------------------------------------------------------\n";
         

   loop(d, 10)
       {
          UTF8_string path = LibPaths::get_lib_dir((LibRef)d);
          out << " " << d << " ";
          switch(LibPaths::get_cfg_src((LibRef)d))
             {
                case LibPaths::LibDir::CS_NONE:      out << "NONE" << endl;
                                                     continue;
                case LibPaths::LibDir::CS_ENV:       out << "ENV   ";   break;
                case LibPaths::LibDir::CS_ARGV0:     out << "BIN   ";   break;
                case LibPaths::LibDir::CS_PREF_SYS:  out << "PSYS  ";   break;
                case LibPaths::LibDir::CS_PREF_HOME: out << "PUSER ";   break;
             }

        out << left << setw(52) << path.c_str();
        DIR * dir = opendir(path.c_str());
        if (dir)   { out << " present" << endl;   closedir(dir); }
        else       { out << " missing (" << errno << ")" << endl; }
      }

   out <<
"===========================================================================\n" << endl;
}
//-----------------------------------------------------------------------------
void 
Command::cmd_LIB(ostream & out, const UCS_string & arg)
{
UTF8_string path = LibPaths::get_lib_dir((LibRef)(arg.atoi()));

DIR * dir = opendir(path.c_str());
   if (dir == 0)
      {
        out << _("IMPROPER LIBRARY REFERENCE") << ": " << path << "/ : "
            << strerror(errno) << endl;

        char cc[200];
          snprintf(cc, sizeof(cc),
                   _("path %s: could not be openend as directory: %s"),
                   path.c_str(), strerror(errno));
        Workspace::more_error() = UCS_string(cc);
        return;
      }

   const int print_width = Workspace::get_PW();
   for (int col = 0;;)   // scan files in directory
       {
         dirent * entry = readdir(dir);
         if (entry == 0)   break;   // directory done

         if (entry->d_type != DT_REG)   continue; // not a regular file

         const int dlen = strlen(entry->d_name);
         int next_col = ((col + 9)/9)*9;
         if (col && (next_col + dlen) > print_width)
            {
              out << endl;
              col = 0;
            }

         if (col)
            {
              do  out << " ";   while (++col % 9);
            }
          out << entry->d_name;
          col += dlen;
       }

   out << endl;
   closedir(dir);
}
//-----------------------------------------------------------------------------
void 
Command::cmd_LOG(ostream & out, const UCS_string & arg)
{
#ifdef DYNAMIC_LOG_WANTED

   log_control(arg);

#else

   out << "\n"
<< _("Command ]LOG is not available, since dynamic logging was not\n"
"configured for this APL interpreter. To enable dynamic logging (which\n"
"will decrease performance), recompile the interpreter as follows:")

<< "\n\n"
"   ./configure DYNAMIC_LOG_WANTED=yes (... "
<< _("other configure options")
<< ")\n"
"   make\n"
"   make install (or try: src/apl)\n"
"\n"

<< _("above the src directory.")
<< "\n";

#endif
}
//-----------------------------------------------------------------------------
void 
Command::cmd_MORE(ostream & out)
{
   if (Workspace::more_error().size() == 0)
      {
        out << _("NO MORE ERROR INFO") << endl;
        return;
      }

   out << Workspace::more_error() << endl;
   return;
}
//-----------------------------------------------------------------------------
void 
Command::cmd_OFF(int exit_val)
{
   cleanup();
   COUT << endl;
   if (!silent)   COUT << _("Goodbye.") << endl;;
   exit(exit_val);
}
//-----------------------------------------------------------------------------
void
Command::cmd_OUT(ostream & out, vector<UCS_string> & args)
{
   if (args.size() == 0)
      {
        out << _("BAD COMMAND") << endl;
        Workspace::more_error() =
                   UCS_string(_("missing filename in command )OUT"));
        return;
      }

UCS_string fname = args.front();
   args.erase(args.begin());

UTF8_string filename = LibPaths::get_lib_filename(LIB_NONE, fname, false,"atf");
   
FILE * atf = fopen(filename.c_str(), "w");
   if (atf == 0)
      {
        UTF8_string fname_utf8(fname);
        out << ")OUT " << fname << _(" failed: ") << strerror(errno) << endl;
        char cc[200];
        snprintf(cc, sizeof(cc),
                 _("command )OUT: could not open file %s for writing: %s"),
                 fname_utf8.c_str(), strerror(errno));
        Workspace::more_error() = UCS_string(cc);
        return;
      }

uint64_t seq = 1;   // sequence number for records written
   Workspace::write_OUT(atf, seq, args);

   fclose(atf);
}
//-----------------------------------------------------------------------------
#ifdef DYNAMIC_LOG_WANTED
void
Command::log_control(const UCS_string & arg)
{
   if (arg.size() == 0 || arg[0] == UNI_ASCII_QUESTION)  // no arg or '?'
      {
        for (LogId l = LID_MIN; l < LID_MAX; l = LogId(l + 1))
            {
              const char * info = Log_info(l);
              Assert(info);

              const bool val = Log_status(l);
              CERR << "    " << setw(2) << right << l << ": " 
                   << (val ? "(ON)  " : "(OFF) ") << left << info << endl;
            }

        return;
      }

LogId val = LogId(0);
bool skip_leading_ws = true;

   loop(a, arg.size())
      {
        if ((arg[a] <= ' ') && skip_leading_ws)   continue;
        skip_leading_ws = false;
        if (arg[a] < UNI_ASCII_0)   break;
        if (arg[a] > UNI_ASCII_9)   break;
        val = LogId(10*val + arg[a] - UNI_ASCII_0);
      }

   if (val >= LID_MIN && val <= LID_MAX)
      {
        const char * info = Log_info(val);
        Assert(info);
        const bool new_status = !Log_status(val);
        Log_control(val, new_status);
        CERR << "    Log facility '" << info << "' is now "
             << (new_status ? "ON " : "OFF") << endl;
      }
}
#endif
//-----------------------------------------------------------------------------
void
Command::transfer_context::process_record(const UTF8 * record, const
                                          vector<UCS_string> & objects)
{
const char rec_type = record[0];   // '*', ' ', or 'X'
const char sub_type = record[1];

   if (rec_type == '*')   // comment or similar
      {
        Log(LOG_command_IN)
           {
             const char * stype = _(" *** bad sub-record of *");
             switch(sub_type)
                {
                  case ' ': stype = _(" comment");     break;
                  case '(': {
                              stype = _(" timestamp");
                              YMDhmsu t(now());   // fallback if sscanf() != 7
                              if (7 == sscanf((const char *)(record + 1),
                                              "(%d %d %d %d %d %d %d)",
                                              &t.year, &t.month, &t.day,
                                              &t.hour, &t.minute, &t.second,
                                              &t.micro))
                                  {
                                    timestamp = t.get();
                                  }
                            }
                            break;
                  case 'I': stype = _(" imbed");       break;
                }

             CERR << _("record #") << setw(3) << recnum << ": '" << rec_type
                  << "'" << stype << endl;
           }
      }
   else if (rec_type == ' ' || rec_type == 'X')   // object
      {
        if (new_record)
           {
             Log(LOG_command_IN)
                {
                  const char * stype = _(" *** bad sub-record of X");

//                          " -------------------------------------";
                  switch(sub_type)
                     {
                       case 'A': stype = " 2 ⎕TF array ";           break;
                       case 'C': stype = " 1 ⎕TF char array ";      break;
                       case 'F': stype = " 2 ⎕TF function ";        break;
                       case 'N': stype = " 1 ⎕TF numeric array ";   break;
                     }

                  CERR << "record #" << setw(3) << recnum
                       << ": " << stype << endl;
                }

             item_type = sub_type;
           }

        add(record + 1, 71);

        new_record = (rec_type == 'X');   // 'X' marks the final record
        if (new_record)
           {
             if      (item_type == 'A')   array_2TF(objects);
             else if (item_type == 'C')   chars_1TF(objects);
             else if (item_type == 'N')   numeric_1TF(objects);
             else if (item_type == 'F')   function_2TF(objects);
             else                         CERR << "????: " << data << endl;
             data.clear();
           }
      }
   else
      {
        CERR << "record #" << setw(3) << recnum << ": '" << rec_type << "'"
             << "*** bad record type '" << rec_type << endl;
      }
}
//-----------------------------------------------------------------------------
uint32_t
Command::transfer_context::get_nrs(UCS_string & name, Shape & shape) const
{
int idx = 1;

   // data + 1 is: NAME RK SHAPE RAVEL...
   //
   while (idx < data.size() && data[idx] != UNI_ASCII_SPACE)
         name.append(data[idx++]);
   ++idx;   // skip space after the name

int rank = 0;
   while (idx < data.size() &&
          data[idx] >= UNI_ASCII_0 &&
          data[idx] <= UNI_ASCII_9)
      {
        rank *= 10;
        rank += data[idx++] - UNI_ASCII_0;
      }
   ++idx;   // skip space after the rank

   loop (r, rank)
      {
        ShapeItem s = 0;
        while (idx < data.size() &&
               data[idx] >= UNI_ASCII_0 &&
               data[idx] <= UNI_ASCII_9)
           {
             s *= 10;
             s += data[idx++] - UNI_ASCII_0;
           }
        shape.add_shape_item(s);
        ++idx;   // skip space after shape[r]
      }
  
   return idx;
}
//-----------------------------------------------------------------------------
void
Command::transfer_context::numeric_1TF(const vector<UCS_string> & objects) const
{
UCS_string var_name;
Shape shape;
int idx = get_nrs(var_name, shape);

   if (objects.size() && !var_name.contained_in(objects))   return;

Symbol * sym = 0;
   if (Avec::is_quad(var_name[0]))   // system variable.
      {
        int len = 0;
        const Token t = Workspace::get_quad(var_name, len);
        if (t.get_ValueType() == TV_SYM)   sym = t.get_sym_ptr();
        else                               Assert(0 && "Bad system variable");
      }
   else                            // user defined variable
      {
        sym = Workspace::lookup_symbol(var_name);
        Assert(sym);
      }
   
   Log(LOG_command_IN)
      {
        CERR << endl << var_name << " rank " << shape.get_rank() << " IS '";
        loop(j, data.size() - idx)   CERR << data[idx + j];
        CERR << "'" << endl;
      }

Token_string tos;
   {
     UCS_string data1(data, idx, data.size() - idx);
     Tokenizer tokenizer(PM_EXECUTE, LOC);
     if (tokenizer.tokenize(data1, tos) != E_NO_ERROR)   return;
   }
 
   if (tos.size() != shape.get_volume())   return;

Value_P val(new Value(shape, LOC));
   new (&val->get_ravel(0)) IntCell(0);   // prototype

const ShapeItem ec = val->element_count();
   loop(e, ec)
      {
        const TokenTag tag = tos[e].get_tag();
        Cell * C = &val->get_ravel(e);
        if      (tag == TOK_INTEGER)  new (C) IntCell(tos[e].get_int_val());
        else if (tag == TOK_REAL)     new (C) FloatCell(tos[e].get_flt_val());
        else if (tag == TOK_COMPLEX)  new (C)
                                          ComplexCell(tos[e].get_cpx_real(),
                                                      tos[e].get_cpx_imag());
        else FIXME;
      }

   Assert(sym);
   sym->assign(val, LOC);
}
//-----------------------------------------------------------------------------
void
Command::transfer_context::chars_1TF(const vector<UCS_string> & objects) const
{
UCS_string var_name;
Shape shape;
int idx = get_nrs(var_name, shape);

   if (objects.size() && !var_name.contained_in(objects))   return;

Symbol * sym = 0;
   if (Avec::is_quad(var_name[0]))   // system variable.
      {
        int len = 0;
        const Token t = Workspace::get_quad(var_name, len);
        if (t.get_ValueType() == TV_SYM)   sym = t.get_sym_ptr();
        else                               Assert(0 && "Bad system variable");
      }
   else                            // user defined variable
      {
        sym = Workspace::lookup_symbol(var_name);
        Assert(sym);
      }
   
   Log(LOG_command_IN)
      {
        CERR << endl << var_name << " rank " << shape.get_rank() << " IS '";
        loop(j, data.size() - idx)   CERR << data[idx + j];
        CERR << "'" << endl;
      }

Value_P val(new Value(shape, LOC));
   new (&val->get_ravel(0)) CharCell(UNI_ASCII_SPACE);   // prototype

const ShapeItem ec = val->element_count();
   loop(e, ec)   new (&val->get_ravel(e)) CharCell(data[idx + e]);

   Assert(sym);
   sym->assign(val, LOC);
}
//-----------------------------------------------------------------------------
void
Command::transfer_context::array_2TF(const vector<UCS_string> &
                                        objects) const
{
   // an Array in 2 ⎕TF format
   //
UCS_string data1(data.get_items() + 1, data.size() - 1);
UCS_string var_or_fun;

   // data1 is: VARNAME←data...
   //
   if (objects.size())
      {
        UCS_string var_name;
        loop(d, data1.size())
           {
             const Unicode uni = data1[d];
             if (uni == UNI_LEFT_ARROW)   break;
             var_name.append(uni);
           }

        if (!var_name.contained_in(objects))   return;
      }

   Quad_TF::tf2_parse(data1, var_or_fun);

   Assert(var_or_fun.size());
}
//-----------------------------------------------------------------------------
void
Command::transfer_context::function_2TF(const vector<UCS_string>
                                           & objects) const
{
int idx = 1;
UCS_string fun_name;

   /// chars 1...' ' are the function name
   while ((idx < data.size()) && (data[idx] != UNI_ASCII_SPACE))
        fun_name.append(data[idx++]);
   ++idx;

   if (objects.size() && !fun_name.contained_in(objects))   return;

UCS_string statement;
   while (idx < data.size())   statement.append(data[idx++]);
   statement.append(UNI_ASCII_LF);

UCS_string fun_name1;
   Quad_TF::tf2_parse(statement, fun_name1);

Symbol * sym1 = Workspace::lookup_existing_symbol(fun_name1);
   Assert(sym1);
Function * fun1 = sym1->get_function();
   Assert(fun1);
   fun1->set_creation_time(timestamp);

   Log(LOG_command_IN)
      {
       const YMDhmsu ymdhmsu(timestamp);
       CERR << "FUNCTION '" << fun_name1 <<  "'" << endl
            << "   created: " << ymdhmsu.day << "." << ymdhmsu.month
            << "." << ymdhmsu.year << "  " << ymdhmsu.hour
            << ":" << ymdhmsu.minute << ":" << ymdhmsu.second
            << "." << ymdhmsu.micro << " (" << timestamp << ")" << endl;
      }
}
//-----------------------------------------------------------------------------
/// map characters 128...255 of the IBM character set for workstations
/// (lrm fig. 68) to Unicode characters,
const int cp_to_uni_map[128] = // see lrm fig 68.
{
  0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
  0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
  0x2395, 0x235E, 0x2339, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
  0x22A4, 0x00D6, 0x00DC, 0x00F8, 0x00A3, 0x22A5, 0x2376, 0x2336,
  0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
  0x00BF, 0x2308, 0x00AC, 0x00BD, 0x222A, 0x00A1, 0x2355, 0x234E,
  0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x235F, 0x2206, 0x2207,
  0x2192, 0x2563, 0x2551, 0x2557, 0x255D, 0x2190, 0x230A, 0x2510,
  0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x2191, 0x2193,
  0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2261,
  0x2378, 0x22F8, 0x2235, 0x2337, 0x2342, 0x233B, 0x22A2, 0x22A3,
  0x25CA, 0x2518, 0x250C, 0x2588, 0x2584, 0x00A6, 0x00CC, 0x2580,
  0x237A, 0x2379, 0x2282, 0x2283, 0x235D, 0x2372, 0x2374, 0x2371,
  0x233D, 0x2296, 0x25CB, 0x2228, 0x2373, 0x2349, 0x2208, 0x2229,
  0x233F, 0x2340, 0x2265, 0x2264, 0x2260, 0x00D7, 0x00F7, 0x2359,
  0x2218, 0x2375, 0x236B, 0x234B, 0x2352, 0x00AF, 0x00A8, 0x00A0
};

/// the inverse mapping of \b cp_to_uni_map
const struct _uni_to_cp_map
{
  int uni;   ///< the Unicode
  int cp;    ///< the IBM char for uni
} uni_to_cp_map[128] = ///< the mapping
{
  { 0x00A0, 255 }, { 0x00A1, 173 }, { 0x00A3, 156 }, { 0x00A6, 221 },
  { 0x00A8, 254 }, { 0x00AA, 166 }, { 0x00AC, 170 }, { 0x00AF, 253 },
  { 0x00BA, 167 }, { 0x00BD, 171 }, { 0x00BF, 168 }, { 0x00C4, 142 },
  { 0x00C5, 143 }, { 0x00C7, 128 }, { 0x00CC, 222 }, { 0x00D1, 165 },
  { 0x00D6, 153 }, { 0x00D7, 245 }, { 0x00DC, 154 }, { 0x00E0, 133 },
  { 0x00E1, 160 }, { 0x00E2, 131 }, { 0x00E4, 132 }, { 0x00E5, 134 },
  { 0x00E7, 135 }, { 0x00E8, 138 }, { 0x00E9, 130 }, { 0x00EA, 136 },
  { 0x00EB, 137 }, { 0x00EC, 141 }, { 0x00ED, 161 }, { 0x00EE, 140 },
  { 0x00EF, 139 }, { 0x00F1, 164 }, { 0x00F2, 149 }, { 0x00F3, 162 },
  { 0x00F4, 147 }, { 0x00F6, 148 }, { 0x00F7, 246 }, { 0x00F8, 155 },
  { 0x00F9, 151 }, { 0x00FA, 163 }, { 0x00FB, 150 }, { 0x00FC, 129 },
  { 0x2190, 189 }, { 0x2191, 198 }, { 0x2192, 184 }, { 0x2193, 199 },
  { 0x2206, 182 }, { 0x2207, 183 }, { 0x2208, 238 }, { 0x2218, 248 },
  { 0x2228, 235 }, { 0x2229, 239 }, { 0x222A, 172 }, { 0x2235, 210 },
  { 0x2260, 244 }, { 0x2261, 207 }, { 0x2264, 243 }, { 0x2265, 242 },
  { 0x2282, 226 }, { 0x2283, 227 }, { 0x2296, 233 }, { 0x22A2, 214 },
  { 0x22A3, 215 }, { 0x22A4, 152 }, { 0x22A5, 157 }, { 0x22F8, 209 },
  { 0x2308, 169 }, { 0x230A, 190 }, { 0x2336, 159 }, { 0x2337, 211 },
  { 0x2339, 146 }, { 0x233B, 213 }, { 0x233D, 232 }, { 0x233F, 240 },
  { 0x2340, 241 }, { 0x2342, 212 }, { 0x2349, 237 }, { 0x234B, 251 },
  { 0x234E, 175 }, { 0x2352, 252 }, { 0x2355, 174 }, { 0x2359, 247 },
  { 0x235D, 228 }, { 0x235E, 145 }, { 0x235F, 181 }, { 0x236B, 250 },
  { 0x2371, 231 }, { 0x2372, 229 }, { 0x2373, 236 }, { 0x2374, 230 },
  { 0x2375, 249 }, { 0x2376, 158 }, { 0x2378, 208 }, { 0x2379, 225 },
  { 0x237A, 224 }, { 0x2395, 144 }, { 0x2500, 196 }, { 0x2502, 179 },
  { 0x250C, 218 }, { 0x2510, 191 }, { 0x2514, 192 }, { 0x2518, 217 },
  { 0x251C, 195 }, { 0x2524, 180 }, { 0x252C, 194 }, { 0x2534, 193 },
  { 0x253C, 197 }, { 0x2550, 205 }, { 0x2551, 186 }, { 0x2554, 201 },
  { 0x2557, 187 }, { 0x255A, 200 }, { 0x255D, 188 }, { 0x2560, 204 },
  { 0x2563, 185 }, { 0x2566, 203 }, { 0x2569, 202 }, { 0x256C, 206 },
  { 0x2580, 223 }, { 0x2584, 220 }, { 0x2588, 219 }, { 0x2591, 176 },
  { 0x2592, 177 }, { 0x2593, 178 }, { 0x25CA, 216 }, { 0x25CB, 234 }
};

void
Command::transfer_context::add(const UTF8 * str, int len)
{

#if 0
   // helper to print uni_to_cp_map using cp_to_uni_map. The print-out can be
   // triggered by )IN file.atf and then sorted with GNU/Linux command 'sort'.
   //
   loop(q, 128)
       fprintf(stderr, "  { 0x%4.4X, %u },\n", cp_to_uni_map[q], q + 128);
   exit(0);
#endif

   loop(l, len)
      {
        const UTF8 utf = str[l];
        Unicode uni;
        switch(utf)
           {
             case 0         ... ('*' - 1):   // < '*'
             case ('*' + 1) ... ('^' - 1):   // < '^'
             case ('^' + 1) ... ('~' - 1):   // < '~'
             case ('~' + 1):                 // i.e. ASCII except * ^ ~
                  data.append(Unicode(utf));         break;

             case '^': data.append(UNI_AND);              break;   // ~ → ∼
             case '*': data.append(UNI_STAR_OPERATOR);    break;   // * → ⋆
             case '~': data.append(UNI_TILDE_OPERATOR);   break;   // ~ → ∼
             
             default:  data.append(Unicode(cp_to_uni_map[utf - 128]));
           }
      }
}
//-----------------------------------------------------------------------------
static int compare_uni(const void * u1, const void * u2)
{
   return ((const _uni_to_cp_map *)u1)->uni - ((const _uni_to_cp_map *)u2)->uni;
}

unsigned char
Command::unicode_to_cp(Unicode uni)
{
   if (uni <= 0x80)                return uni;
   if (uni == UNI_STAR_OPERATOR)   return '*';   // ⋆ → *
   if (uni == UNI_AND)             return '^';   // ∧ → ^
   if (uni == UNI_TILDE_OPERATOR)  return 126;   // ∼ → ~

const _uni_to_cp_map __uni = { uni, 0 };

   // search in uni_to_cp_map table
   //
const void * where = bsearch(&__uni, uni_to_cp_map, 128, sizeof(_uni_to_cp_map),
                             compare_uni);

   if (where == 0)
      {
        // the workspace being )OUT'ed can contain characters that are not
        // in IBM's APL character set. We replace such characters by 0xB0
        //
        return 0xB0;
      }

   Assert(where);
   return ((const _uni_to_cp_map *)where)->cp;
}
//-----------------------------------------------------------------------------
bool
Command::parse_from_to(UCS_string & from, UCS_string & to,
                       const UCS_string & user_arg)
{
   from.clear();
   to.clear();

int s = 0;
bool got_minus = false;
     while (s < user_arg.size() && user_arg[s] <= ' ') ++s;
     if (s == user_arg.size())   return false;   // empty argument: OK

     while (s < user_arg.size()   &&
                user_arg[s] > ' ' &&
                user_arg[s] != '-')  from.append(user_arg[s++]);
     while (s < user_arg.size() && user_arg[s] <= ' ') ++s;

     if (s < user_arg.size() && user_arg[s] == '-') { ++s;   got_minus = true; }

     while (s < user_arg.size() && user_arg[s] <= ' ') ++s;
     while (s < user_arg.size() && user_arg[s] > ' ')  to.append(user_arg[s++]);
     while (s < user_arg.size() && user_arg[s] <= ' ') ++s;

const bool got_rest = (s < user_arg.size());
   return  got_rest || !got_minus;
}
//-----------------------------------------------------------------------------
bool
Command::is_lib_ref(const UCS_string & lib)
{
   if (lib.size() == 1)   // single char: lib number
      {
        if (Avec::is_digit(lib[0]))   return true;
      }

   if (lib[0] == UNI_ASCII_FULLSTOP)   return true;

   loop(l, lib.size())
      {
        const Unicode uni = lib[l];
        if (uni == UNI_ASCII_SLASH)       return true;
        if (uni == UNI_ASCII_BACKSLASH)   return true;
      }

   return false;
}
//-----------------------------------------------------------------------------
