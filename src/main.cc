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
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "buildtag.hh"
#include "Command.hh"
#include "Common.hh"
#include "Input.hh"
#include "LibPaths.hh"
#include "Logging.hh"
#include "main.hh"
#include "Output.hh"
#include "Prefix.hh"
#include "ProcessorID.hh"
#include "Quad_SVx.hh"
#include "TestFiles.hh"
#include "UserFunction.hh"
#include "ValueHistory.hh"
#include "Workspace.hh"
#include "Value.hh"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

   // global flags
   //
bool silent = false;
bool emacs_mode = false;
bool do_not_echo = false;

//-----------------------------------------------------------------------------
/// initialize subsystems
void
init(const char * argv0)
{
rlimit rl;
   getrlimit(RLIMIT_AS, &rl);
   total_memory = rl.rlim_cur;

   Output::init();
   Avec::init();
   LibPaths::init(argv0);
   Value::init();
   VH_entry::init();
}
//-----------------------------------------------------------------------------
/// the opposite of init()
void
cleanup()
{
   ProcessorID::disconnect();

   Input::exit_readline();   // write history
   Output::reset_colors();
}
//-----------------------------------------------------------------------------

APL_time interrupt_when = 0;
bool interrupt_raised = false;
bool attention_raised = false;

static void
control_C(int)
{
APL_time when = now();
   attention_raised = true;
   if ((when - interrupt_when) < 500000)   // second ^C within 500 ms
      {
        interrupt_raised = true;
      }

   interrupt_when = when;
}

static struct sigaction old_control_C_action;
static struct sigaction new_control_C_action;

//-----------------------------------------------------------------------------
static void
seg_fault(int)
{
   CERR << "\n\n====================================================\n"
           "SEGMENTATION FAULT" << endl;

   Backtrace::show(__FILE__, __LINE__);

   CERR << "====================================================\n";

   // count errors
   TestFiles::assert_error();

   Command::cmd_OFF(3);
}

static struct sigaction old_segfault_action;
static struct sigaction new_segfault_action;

//-----------------------------------------------------------------------------
static void
signal_USR1_handler(int)
{
   CERR << "Got signal USR1" << endl;
}

static struct sigaction old_USR1_action;
static struct sigaction new_USR1_action;

//-----------------------------------------------------------------------------
/**
    When APL is started as a script. then several options might be grouped
    into a single option. expand_argv() expands such grouped options into
    individual options
 **/
const char **
expand_argv(int & argc, const char ** argv)
{
bool need_expand = false;
int end_of_arg = -1;   // position of --

   // check if any argument contains spaces and remember the '--' option
   //
   loop(a, argc)
      {
        if (end_of_arg != -1)   // '--' seen
           {
             scriptname = argv[a];
             end_of_arg = -1;
           }

        if (strchr(argv[a], ' '))     need_expand = true;
        if (!strcmp(argv[a], "--"))   end_of_arg = a;
      }

   // if no argument had a space then return the original argv 
   //
   if (!need_expand)   return argv;

vector<const char *>argvec;

   end_of_arg = -1;
   loop(a, argc)
      {
        if (end_of_arg != -1)   // '--' seen
           {
             scriptname = argv[a];
             end_of_arg = -1;
           }
        end_of_arg = -1;

        if (strchr(argv[a], ' ') == 0)   // no space in this arg
           {
             argvec.push_back(argv[a]);
             if (!strcmp(argv[a], "--"))   end_of_arg = a;
             continue;
           }

        // arg has spaces, i.e. it may have multiple options
        //
        const char * arg = argv[a];

        for (;;)
            {
              while (*arg == ' ')   ++arg;   // skip leading spaces
              if (*arg == 0)        break;   // nothing left
              if (*arg == '#')      break;   // comment

              const char * sp = strchr(arg, ' ');
              if (sp == 0)   sp = arg + strlen(arg);
              const int new_len = sp - arg;   // excluding terminating 0
              if (!strncmp(arg, "--", new_len))   end_of_arg = a;

              char * new_arg = new char[new_len + 1];
              memcpy(new_arg, arg, new_len);
              new_arg[new_len] = 0;
              argvec.push_back(new_arg);
              arg = sp;
            }
      }

   argc = argvec.size();

   // the user may not be aware of how 'execve' calls an interpreter script
   // and may not have given -f as the last option in the script file.
   // In that situation argv is something like:
   //
   // "-f" "-other-options..." "script-name"
   //
   // instead of the expected:
   //
   // "-other-options..." "-f" "script-name"
   //
   // We fix this common mistake here. The downside is, or course, that option
   // names cannnot be script names. We ignore options -h and --help since
   // they exit immediately.
   //
   for (int a = 1; a < (argc - 1); ++a)
       {
         const char * opt = argvec[a];
         const char * next = argvec[a + 1];
         if (!strcmp(opt, "-f") && ( !strcmp(next, "-d")        ||
                                     !strcmp(next, "--id")      ||
                                     !strcmp(next, "-l")        ||
                                     !strcmp(next, "--noCIN")   ||
                                     !strcmp(next, "--noCONT")  ||
                                     !strcmp(next, "--Color")   ||
                                     !strcmp(next, "--noColor") ||
                                     !strcmp(next, "--SV")      ||
                                     !strcmp(next, "--noSV")    ||
                                     !strcmp(next, "--par")     ||
                                     !strcmp(next, "-s")        ||
                                     !strcmp(next, "--script")  ||
                                     !strcmp(next, "--silent")  ||
                                     !strcmp(next, "-T")        ||
                                     !strcmp(next, "--TM")      ||
                                     !strcmp(next, "-w")))
            {
              // there is another known option after -f
              //
              for (int aa = a; aa < (argc - 2); ++aa)
                 argvec[aa] = argvec[aa + 1];
              argvec[argc - 2] = opt;   // put the -f before last
              break;   // for a...
            }
       }

const char ** ret = new const char *[argc + 1];
   loop(a, argc)   ret[a] = argvec[a];
   ret[argc] = 0;
   return ret;
}
//-----------------------------------------------------------------------------
void
show_argv(int argc, const char ** argv)
{
   CERR << "argc: " << argc << endl;
   loop(a, argc)   CERR << "  argv[" << a << "]: '" << argv[a] << "'" << endl;
}
//-----------------------------------------------------------------------------
static void
show_version(ostream & out)
{
const char * build_tag[] = { BUILDTAG, 0 };
   out << "BUILDTAG:" << endl
       << "---------" << endl;
   for (const char ** bt = build_tag; *bt; ++bt)
       {
         switch(bt - build_tag)
            {
              case 0:  out << "  Project:        ";   break;
              case 1:  out << "  Version / SVN:  ";   break;
              case 2:  out << "  Build Date:     ";   break;
              case 3:  out << "  Build OS:       ";   break;
              case 4:  out << "  config.status:  ";   break;
              default: out << "  [" << bt - build_tag << "] ";
            }
         out << *bt << endl;
       }

   out << "  Readline:       " << HEX4(Input::readline_version()) << endl
       << endl;

   Output::set_color_mode(Output::COLM_OUTPUT);
}
//-----------------------------------------------------------------------------
/// print how this interpreter was configured (via ./configure) and exit
const char *
is_default(bool yes)
{
   return yes ? _(" (default)") : "";
}

static void
show_configure_options()
{
   enum { value_size = sizeof(Value),
          cell_size  = sizeof(Cell),
          header_size = value_size - cell_size * SHORT_VALUE_LENGTH_WANTED,
        };

   CERR << endl << _("configurable options:") << endl <<
                   "---------------------" << endl <<

   "    ASSERT_LEVEL_WANTED=" << ASSERT_LEVEL_WANTED
        << is_default(ASSERT_LEVEL_WANTED == 1) << endl <<

#ifdef DYNAMIC_LOG_WANTED
   "    DYNAMIC_LOG_WANTED=yes"
#else
   "    DYNAMIC_LOG_WANTED=no (default)"
#endif
   << endl <<

#ifdef ENABLE_NLS
   "    NLS enabled"
#else
   "    NLS disabled"
#endif
   << endl <<

#ifdef HAVE_LIBREADLINE
   "    libreadline is used (default)"
#else
   "    libreadline is not used (disabled or not present)"
#endif
   << endl <<

   "    MAX_RANK_WANTED="     << MAX_RANK_WANTED
        << is_default(MAX_RANK_WANTED == 8)
   << endl <<

   "    SHORT_VALUE_LENGTH_WANTED=" << SHORT_VALUE_LENGTH_WANTED
        << is_default(SHORT_VALUE_LENGTH_WANTED == 1)
        << _(", therefore:") << endl <<
   "        sizeof(Value)       : "  << value_size  << " bytes" << endl <<
   "        sizeof(Cell)        :  " << cell_size   << " bytes" << endl <<
   "        sizeof(Value header): "  << header_size << " bytes" << endl <<
   endl <<

#ifdef VALUE_CHECK_WANTED
   "    VALUE_CHECK_WANTED=yes"
#else
   "    VALUE_CHECK_WANTED=no (default)"
#endif
   << endl <<

#ifdef VALUE_HISTORY_WANTED
   "    VALUE_HISTORY_WANTED=yes"
#else
   "    VALUE_HISTORY_WANTED=no (default)"
#endif
   << endl <<

#ifdef VF_TRACING_WANTED
   "    VF_TRACING_WANTED=yes"
#else
   "    VF_TRACING_WANTED=no (default)"
#endif
   << endl
   << endl;


   show_version(CERR);
}
//-----------------------------------------------------------------------------
/// print possible command line options and exit
void
usage(const char * prog)
{
const char * prog1 = strrchr(prog, '/');
   if (prog1)   ++prog1;
   else         prog1 = prog;

char cc[4000];
   snprintf(cc, sizeof(cc), 

_("usage: %s [options]\n"
"    options: \n"
"    -h, --help           print this help\n"
"    -d                   run in the background (i.e. as daemon)\n"
"    -f file              read input from file (after all -T files)\n"
"    --id proc            use processor ID proc (default: first unused > 1000)\n"), prog);
   CERR << cc;

#ifdef DYNAMIC_LOG_WANTED
   snprintf(cc, sizeof(cc), 
_("    -l num               turn log facility num (1-%d) ON\n"), LID_MAX-1);
   CERR << cc;

#endif

   snprintf(cc, sizeof(cc), _(
"    --par proc           use processor parent ID proc (default: no parent)\n"
"    -w milli             wait milli milliseconds at startup\n"
"    --noCIN              do not echo input(for scripting)\n"
"    --rawCIN             do not use the readline lib for input\n"
"    --[no]Color          start with ]XTERM ON [OFF])\n"
"    --noCONT             do not load CONTINUE workspace on startup)\n"
"    --emacs              run in emacs mode\n"
"    --[no]SV             [do not] start APnnn (a shared variable server)\n"
"    --cfg                show ./configure options used and exit\n"
"    --gpl                show license (GPL) and exit\n"
"    --silent             do not show the welcome message\n"
"    -s, --script         same as --silent --noCIN --noCONT --noColor -f -\n"
"    -v, --version        show version information and exit\n"
"    -T testcases ...     run testcases\n"
"    --TM mode            test mode (for -T files):\n"
"                         0:   (default) exit after last testcase\n"
"                         1:   exit after last testcase if no error\n"
"                         2:   continue (don't exit) after last testcase\n"
"                         3:   stop testcases after first error (don't exit)\n"
"                         4:   exit after first error\n"
"    --TR                 randomize order of testfiles\n"
"    --TS                 append to (rather than override) summary.log\n"
"    --                   end of options for %s\n"), prog1);
   CERR << cc << endl;
}
//-----------------------------------------------------------------------------
/// print the GPL
static void
show_GPL(ostream & out)
{
   out <<
"\n"
"---------------------------------------------------------------------------\n"
"\n"
"    This program is GNU APL, a free implementation of the\n"
"    ISO/IEC Standard 13751, \"Programming Language APL, Extended\"\n"
"\n"
"    Copyright (C) 2008-2013  Dr. Jürgen Sauermann\n"
"\n"
"    This program is free software: you can redistribute it and/or modify\n"
"    it under the terms of the GNU General Public License as published by\n"
"    the Free Software Foundation, either version 3 of the License, or\n"
"    (at your option) any later version.\n"
"\n"
"    This program is distributed in the hope that it will be useful,\n"
"    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"    GNU General Public License for more details.\n"
"\n"
"    You should have received a copy of the GNU General Public License\n"
"    along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
"\n"
"---------------------------------------------------------------------------\n"
"\n";
}
//-----------------------------------------------------------------------------
/// print a welcome message (copyright notice)
static void
show_welcome(ostream & out, const char * argv0)
{
char c1[200];
char c2[200];
   snprintf(c1, sizeof(c1), _("Welcome to GNU APL version %s"),PACKAGE_VERSION);
   snprintf(c2, sizeof(c2), _("for details run: %s --gpl."), argv0);

const char * lines[] =
{
  "",
  "   ______ _   __ __  __    ___     ____   __ ",
  "  / ____// | / // / / /   /   |   / __ \\ / / ",
  " / / __ /  |/ // / / /   / /| |  / /_/ // /  ",
  "/ /_/ // /|  // /_/ /   / ___ | / ____// /___",
  "\\____//_/ |_/ \\____/   /_/  |_|/_/    /_____/",



  "",
  c1,
  "",
  "Copyright (C) 2008-2013  Dr. Jürgen Sauermann",
  "Banner by FIGlet: www.figlet.org",
  "",
_("This program comes with ABSOLUTELY NO WARRANTY;"),
  c2,
  "",
_("This program is free software, and you are welcome to redistribute it"),
_("according to the GNU Public License (GPL) version 3 or later."),
  "",
  0
};

   // compute max. length
   //
int len = 0;
   for (const char ** l = lines; *l; ++l)
       {
         const char * cl = *l;
         const int clen = strlen(cl);
         if (len < clen)   len = clen;
       }
 
const int left_pad = (80 - len)/2;

   for (const char ** l = lines; *l; ++l)
       {
         const char * cl = *l;
         const int clen = strlen(cl);
         const int pad = left_pad + (len - clen)/2;
         loop(p, pad)   out << " ";
         out << cl << endl;
       }
}
//-----------------------------------------------------------------------------
#ifdef ENABLE_NLS
extern char * bindtextdomain (const char * domainname, const char * dirname);
extern char * textdomain (const char * domainname);


static void
init_NLS()
{
   Log(LOG_startup)   CERR << "NLS is enabled." << endl;

const char * locale = setlocale(LC_ALL, "");
   if (locale == 0)
      {
        CERR <<
"*** function locale() has returned 0 which indicates a problem\n"
"with your locale settings. This will most likely prevent APL characters\n"
"from being entered or displayed properly\n"
"\n"
"Please fix your locales or ./configure GNU APL with --disable-nls\n"
              << endl;
        return;
      }

   Log(LOG_startup)   CERR << "locale is: " << locale << "." << endl;

const char * dir = bindtextdomain(PACKAGE, LOCALEDIR);

   if (dir)
      {
        Log(LOG_startup)   CERR << "locale directory is: "
                                << dir << "." << endl;
      }
   else
      {
        CERR << "*** bindtextdomain(" << PACKAGE << ", " << LOCALEDIR
             << " failed: " << strerror(errno) << endl;
        return;
      }

const char * dom = textdomain(PACKAGE);
   if (dom)
      {
        Log(LOG_startup)   CERR << "locale domain is: " << dom << "." << endl;
      }
   else
      {
        CERR << "*** textdomain(" << PACKAGE << ", " << LOCALEDIR
             << " failed: " << strerror(errno) << endl;
        return;
      }
}
#endif
//-----------------------------------------------------------------------------

/// a structure that contains user preferences from different sources
/// (command line arguments, config files, environment variables ...)
struct user_preferences
{
   user_preferences()
   : do_CONT(true),
     do_Color(true),
     requested_id(0),
     requested_par(0),
     do_sv(true),
     daemon(false),
     append_summary(false),
     wait_ms(0),
     randomize_testfiles(false)
   {}

   /// load workspace CONTINUE on start-up
   bool do_CONT;

   /// output coloring enabled
   bool do_Color;

   /// desired --id (⎕AI[1] and shared variable functions)
   int requested_id;

   /// desired --par (⎕AI[1] and shared variable functions)
   int requested_par;

   /// enable shared variables
   bool do_sv;

   /// run as deamon
   bool daemon;

   /// append test results to summary.log rather than overriding it
   bool append_summary;

   /// wait at start-up
   int wait_ms;

   /// randomize the order of testfiles
   bool randomize_testfiles;
};
//-----------------------------------------------------------------------------
// read user preference file(s) if present
void
read_config_file(bool sys, user_preferences & up)
{
char filename[PATH_MAX + 1] = "/etc/gnu-apl.d/preferences";   // fallback filename


   if (sys)   // try /etc/gnu-apl/preferences
      {
#ifdef SYSCONFDIR
        if (strlen(SYSCONFDIR))
           snprintf(filename, PATH_MAX, "%s/gnu-apl.d/preferences", SYSCONFDIR);
#endif
      }
   else       // try $HOME/.gnu_apl
      {
        const char * HOME = getenv("HOME");
        if (HOME == 0)
           {
             Log(LOG_startup)
                CERR << "environment variable 'HOME' is not defined!" << endl;
             return;
           }
        snprintf(filename, PATH_MAX, "%s/.gnu-apl/preferences", HOME);
      }

   filename[PATH_MAX] = 0;

FILE * f = fopen(filename, "r");
   if (f == 0)
      {
         Log(LOG_startup)
            CERR << "config file " << filename
                 << " is not present/readable" << endl;
         return;
      }

   Log(LOG_startup)
      CERR << "Reading config file " << filename << " ..." << endl;

int line = 0;
   for (;;)
       {
         enum { BUFSIZE = 200 };
         char buffer[BUFSIZE + 1];
         const char * s = fgets(buffer, BUFSIZE, f);
         if (s == 0)   break;   // end of file

         buffer[BUFSIZE] = 0;
         ++line;

         // skip leading spaces
         //
         while (*s && *s <= ' ')   ++s;
         if (*s == 0)     continue;   // empty line
         if (*s == '#')   continue;   // comment line
         char opt[BUFSIZE] = { 0 };
         char arg[BUFSIZE] = { 0 };
         int d[20];
         const int count = sscanf(s, "%s"
                   " %s %X %X %X %X %X %X %X %X %X"
                   " %X %X %X %X %X %X %X %X %X %X",
              opt, arg,  d+ 1, d+ 2, d+ 3, d+ 4, d+ 5, d+ 6, d+ 7, d+ 8, d+ 9,
                   d+10, d+11, d+12, d+13, d+14, d+15, d+16, d+17, d+18, d+19);

         if (count < 2)
            {
              CERR << "bad tag or value at line " << line
                   << " of config file " << filename << " (ignored)" << endl;
              continue;
            }
         d[0] = strtol(arg, 0, 16);
         const bool yes = !strcasecmp(arg, "YES"     )
                       || !strcasecmp(arg, "ENABLED" )
                       || !strcasecmp(arg, "ON"      );

         const bool no  = !strcasecmp(arg, "NO"      )
                       || !strcasecmp(arg, "DISABLED")
                       || !strcasecmp(arg, "OFF"     ) ;

         const bool yes_no  = yes || no;

         if (!strcasecmp(opt, "Color"))
            {
              if (yes_no)   // user said "Yes" or "No"
                 {
                   // if the user said "No" then use_curses is not touched.
                   // if the user said "Yes" (which is an obsolete setting)
                   // then use_curses is cleared because at the time when
                   // "Yes" was valid, curses was not yet implemented.
                   //
                   up.do_Color = yes;
                   if (yes)   Output::use_curses = false;
                 }
              if (!strcasecmp(arg, "ANSI"))
                 {
                   up.do_Color = true;
                   Output::use_curses = false;
                 }
              if (!strcasecmp(arg, "CURSES"))
                 {
                   up.do_Color = true;
                   Output::use_curses = true;
                 }
            }
         else if (yes_no && !strcasecmp(opt, "Welcome"))
            {
              silent = no;
            }
         else if (yes_no && !strcasecmp(opt, "SharedVars"))
            {
              up.do_sv = yes;
            }
         else if (!strcasecmp(opt, "Logging"))
            {
              Log_control(LogId(d[0]), true);
            }
         else if (!strcasecmp(opt, "CIN-SEQUENCE"))
            {
              loop(p, count - 1)   Output::color_CIN[p] = (char)(d[p] & 0xFF);
              Output::color_CIN[count - 1] = 0;
            }
         else if (!strcasecmp(opt, "COUT-SEQUENCE"))
            {
              loop(p, count - 1)   Output::color_COUT[p] = (char)(d[p] & 0xFF);
              Output::color_COUT[count - 1] = 0;
            }
         else if (!strcasecmp(opt, "CERR-SEQUENCE"))
            {
              loop(p, count - 1)   Output::color_CERR[p] = (char)(d[p] & 0xFF);
              Output::color_CERR[count - 1] = 0;
            }
         else if (!strcasecmp(opt, "UERR-SEQUENCE"))
            {
              loop(p, count - 1)   Output::color_UERR[p] = (char)(d[p] & 0xFF);
              Output::color_UERR[count - 1] = 0;
            }
         else if (!strcasecmp(opt, "RESET-SEQUENCE"))
            {
              loop(p, count - 1)   Output::color_RESET[p] = (char)(d[p] & 0xFF);
              Output::color_RESET[count - 1] = 0;
            }
         else if (!strcasecmp(opt, "CLEAR-EOL-SEQUENCE"))
            {
              loop(p, count - 1)   Output::clear_EOL[p] = (char)(d[p] & 0xFF);
              Output::clear_EOL[count - 1] = 0;
            }
         else if (!strcasecmp(opt, "CIN-FOREGROUND"))
            {
              Output::color_CIN_foreground = atoi(arg);
            }
         else if (!strcasecmp(opt, "CIN-BACKGROUND"))
            {
              Output::color_CIN_background = atoi(arg);
            }
         else if (!strcasecmp(opt, "COUT-FOREGROUND"))
            {
              Output::color_COUT_foreground = atoi(arg);
            }
         else if (!strcasecmp(opt, "COUT-BACKGROUND"))
            {
              Output::color_COUT_background = atoi(arg);
            }
         else if (!strcasecmp(opt, "CERR-FOREGROUND"))
            {
              Output::color_CERR_foreground = atoi(arg);
            }
         else if (!strcasecmp(opt, "CERR-BACKGROUND"))
            {
              Output::color_CERR_background = atoi(arg);
            }
         else if (!strcasecmp(opt, "UERR-FOREGROUND"))
            {
              Output::color_UERR_foreground = atoi(arg);
            }
         else if (!strcasecmp(opt, "UERR-BACKGROUND"))
            {
              Output::color_UERR_background = atoi(arg);
            }
         else if (!strncasecmp(opt, "LIBREF-", 7))
            {
              const int lib_ref = atoi(opt + 7);
              if (lib_ref < 0 || lib_ref > 9)
                 {
                   CERR << "bad library reference number " << lib_ref
                        << " at line " << line << " of config file "
                        << filename << " (ignored)" << endl;
                   continue;
                 }

              LibPaths::set_lib_dir((LibRef)lib_ref, arg,
                                    sys ? LibPaths::LibDir::CS_PREF_SYS
                                        : LibPaths::LibDir::CS_PREF_HOME);
            }
       }

   fclose(f);
}
//-----------------------------------------------------------------------------
int
main(int argc, const char * _argv[])
{
const char ** argv = expand_argv(argc, _argv);
   Quad_ARG::argc = argc;   // remember argc for ⎕ARG
   Quad_ARG::argv = argv;   // remember argv for ⎕ARG

const char * argv0 = argv[0];
   init(argv0);

user_preferences up;
   read_config_file(true,  up);   // /etc/gnu-apl.d/preferences
   read_config_file(false, up);   // $HOME/.gnu_apl

   // struct sigaction differs between GNU/Linux and other systems, which
   // causes direct bracket assignment to not compile on some machines.
   //
   // We therefore memset everything to 0 and then set the handler (which
   // should be compatible on GNU/Linux and other systems.
   //
   memset(&new_control_C_action, 0, sizeof(struct sigaction));
   memset(&new_USR1_action,      0, sizeof(struct sigaction));
   memset(&new_segfault_action,  0, sizeof(struct sigaction));

   new_control_C_action.sa_handler = &control_C;
   new_USR1_action     .sa_handler = &signal_USR1_handler;
   new_segfault_action .sa_handler = &seg_fault;

   sigaction(SIGINT,  &new_control_C_action, &old_control_C_action);
   sigaction(SIGUSR1, &new_USR1_action,      &old_USR1_action);
   sigaction(SIGSEGV, &new_segfault_action,  &old_segfault_action);

   // init NLS so that usage() will be translated
   //
#ifdef ENABLE_NLS
   init_NLS();
#endif

   for (int a = 1; a < argc; )
       {
         const char * opt = argv[a++];
         const char * val = (a < argc) ? argv[a] : 0;

         if (!strcmp(opt, "--"))   // end of arguments
            {
              a = argc;
              break;
            }
         else if (!strcmp(opt, "--cfg"))
            {
              show_configure_options();
              return 0;
            }
         else if (!strcmp(opt, "--Color"))
            {
              up.do_Color = true;
            }
         else if (!strcmp(opt, "--noColor"))
            {
              up.do_Color = false;
            }
         else if (!strcmp(opt, "-d"))
            {
              up.daemon = true;
            }
         else if (!strcmp(opt, "-f"))
            {
              ++a;
              if (val)   Input::input_file_name = val;
              else
                 {
                   CERR << _("-f without filename") << endl;
                   return 1;
                 }
            }
         else if (!strcmp(opt, "--gpl"))
            {
              show_GPL(cout);
              return 0;
            }
         else if (!strcmp(opt, "-h") || !strcmp(opt, "--help"))
            {
              usage(argv[0]);
              return 0;
            }
         else if (!strcmp(opt, "--id"))
            {
              ++a;
              if (!val)
                 {
                   CERR << "--id without processor number" << endl;
                   return 2;
                 }

              up.requested_id = atoi(val);
              continue;
            }
#ifdef DYNAMIC_LOG_WANTED
         else if (!strcmp(opt, "-l"))
            {
              ++a;
              if (val)   Log_control(LogId(atoi(val)), true);
              else
                 {
                   CERR << _("-l without log facility") << endl;
                   return 3;
                 }
            }
#endif // DYNAMIC_LOG_WANTED
         else if (!strcmp(opt, "--noCIN"))
            {
              do_not_echo = true;
            }
         else if (!strcmp(opt, "--rawCIN"))
            {
              Input::use_readline = false;
            }
         else if (!strcmp(opt, "--noCONT"))
            {
              up.do_CONT = false;
            }
         else if (!strcmp(opt, "--emacs"))
            {
              emacs_mode = true;
            }
         else if (!strcmp(opt, "--SV"))
            {
              up.do_sv = true;
            }
         else if (!strcmp(opt, "--noSV"))
            {
              up.do_sv = false;
            }
         else if (!strcmp(opt, "--par"))
            {
              ++a;
              if (!val)
                 {
                   CERR << "--par without processor number" << endl;
                   return 4;
                 }
              up.requested_par = atoi(val);
              continue;
            }
         else if (!strcmp(opt, "-s") || !strcmp(opt, "--script"))
            {
              up.do_CONT = false;             // --noCONT
              up.do_Color = false;            // --noColor
              do_not_echo = true;             // -noCIN
              silent = true;                  // --silent
              Input::input_file_name = "-";   // -f -
            }
         else if (!strcmp(opt, "--silent"))
            {
              silent = true;
            }
         else if (!strcmp(opt, "-T"))
            {
              up.do_CONT = false;

              // TestFiles::open_next_testfile() will exit if it sees "-"
              //
              TestFiles::need_total = true;
              for (; a < argc; ++a)
                  {
                    if (!strcmp(argv[a], "--"))   // end of arguments
                       {
                         a = argc;
                         break;
                         
                       }
                    TestFiles::test_file_names.push_back(argv[a]);
                  }

              // 
              if (TestFiles::is_validating())
                 {
                   // truncate summary.log, unless append_summary is desired
                   //
                   if (!up.append_summary)
                      {
                        ofstream summary("testcases/summary.log",
                                         ios_base::trunc);
                      }
                   TestFiles::open_next_testfile();
                 }
            }
         else if (!strcmp(opt, "--TM"))
            {
              ++a;
              if (val)
                 {
                   const int mode = atoi(val);
                   TestFiles::test_mode = TestFiles::TestMode(mode);
                 }
              else
                 {
                   CERR << _("--TM without test mode") << endl;
                   return 5;
                 }
            }
         else if (!strcmp(opt, "--TR"))
            {
              up.randomize_testfiles = true;
            }
         else if (!strcmp(opt, "--TS"))
            {
              up.append_summary = true;
            }
         else if (!strcmp(opt, "-v") || !strcmp(opt, "--version"))
            {
              show_version(cout);
              return 0;
            }
         else if (!strcmp(opt, "-w"))
            {
              ++a;
              if (val)   up.wait_ms = atoi(val);
              else
                 {
                   CERR << _("-w without milli(seconds)") << endl;
                   return 6;
                 }
            }
         else
            {
              CERR << _("unknown option '") << opt << "'" << endl;
              usage(argv[0]);
              return 7;
            }
       }

   if (emacs_mode)
      {
        // CIN = U+F00C0 = UTF8 F3 B0 83 80 ...
        Output::color_CIN[0] = 0xF3;
        Output::color_CIN[1] = 0xB0;
        Output::color_CIN[2] = 0x83;
        Output::color_CIN[3] = 0x80;
        Output::color_CIN[4] = 0;

        // COUT = U+F00C1 = UTF8 F3 B0 83 81 ...
        Output::color_COUT[0] = 0xF3;
        Output::color_COUT[1] = 0xB0;
        Output::color_COUT[2] = 0x83;
        Output::color_COUT[3] = 0x81;
        Output::color_COUT[4] = 0;

        // CERR = U+F00C2 = UTF8 F3 B0 83 82 ...
        Output::color_CERR[0] = 0xF3;
        Output::color_CERR[1] = 0xB0;
        Output::color_CERR[2] = 0x83;
        Output::color_CERR[3] = 0x82;
        Output::color_CERR[4] = 0;

        // UERR = U+F00C3 = UTF8 F3 B0 83 83 ...
        Output::color_UERR[0] = 0xF3;
        Output::color_UERR[1] = 0xB0;
        Output::color_UERR[2] = 0x83;
        Output::color_UERR[3] = 0x83;
        Output::color_UERR[4] = 0;

        // no clear_EOL
        Output::clear_EOL[0] = 0;
      }

   // init input after reading the command line options, so that the user
   // has a chance to disable readline
   //
   Input::init();

   if (up.daemon)
      {
        if (fork())   return 0;   // parent returns
        Log(LOG_startup)   CERR << "process forked" << endl;
      }

   if (up.wait_ms)   usleep(1000*up.wait_ms);

   if (!silent)   show_welcome(cout, argv0);

   Log(LOG_startup)   CERR << "PID is " << getpid() << endl;
   Log(LOG_argc_argv)   show_argv(argc, argv);

   TestFiles::testcase_count = TestFiles::test_file_names.size();
   if (up.randomize_testfiles)   TestFiles::randomize_files();

   if (ProcessorID::init(up.do_sv, up.requested_id, up.requested_par))
      {
        // error message printed in ProcessorID::init()
        return 8;
      }

   if (up.do_Color)   Output::toggle_color("ON");

   if (up.do_CONT)
      {
         UCS_string cont("CONTINUE.xml");
         UTF8_string filename =
            LibPaths::get_lib_filename(LIB0, "CONTINUE", true, "xml");

         if (access(filename.c_str(), F_OK) == 0)
            {
              UCS_string load_cmd(")LOAD CONTINUE");
              Command::process_line(load_cmd);
            }
      }

   for (;;)
       {
         Token t = Workspace::immediate_execution(
                       TestFiles::test_mode == TestFiles::TM_EXIT_AFTER_ERROR);
         if (t.get_tag() == TOK_OFF)   Command::cmd_OFF(0);
       }

   /* not reached */
}
//-----------------------------------------------------------------------------
