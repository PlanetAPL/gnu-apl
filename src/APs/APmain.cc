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
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>

#include <cassert>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>

#include "APmain.hh"
#include "Svar_DB.hh"
#include "Svar_signals.hh"

#ifdef DYNAMIC_LOG_WANTED
extern bool LOG_shared_variables;
bool LOG_shared_variables = false;
#else
#endif

//-----------------------------------------------------------------------------
class CERR_Out : public filebuf
{
public:
   CERR_Out() : print_sema_held(false) {}

   virtual int overflow(int c);

   filebuf * use()   { used = true;   return this; }

   bool print_sema_held;

   static bool used;   // set when CERR is constructed

} CERR_filebuf;
bool CERR_Out::used = false;

//-----------------------------------------------------------------------------
int
CERR_Out::overflow(int c)
{
   if (!print_sema_held)
      {
        Svar_DB::start_print(LOC);
        print_sema_held = true;
      }

   cerr << (char)c;

   if (c == '\n')
      {
        Svar_DB::end_print(LOC);
        print_sema_held = false;
      }

   return 0;
}

ostream CERR(CERR_filebuf.use());

//-----------------------------------------------------------------------------

bool verbose = false;
int event_port = 0;

const char * prog = 0;
char pref[FILENAME_MAX + 20];

pid_t parent_pid = 0;

/// coupled variables
vector<Coupled_var> coupled_vars;

/// the place where the AP specific part has detected an error
string error_loc = "?";

/// the name of this AP (aplXXX where XXX is the processor number)
char AP_NAME[40] = "apl" STR(AP_NUM);

AP_num3 ProcessorID::id(NO_AP, AP_NULL, AP_NULL);

ostream & get_CERR()
{
   return CERR_Out::used ? CERR : cerr;
}
//-----------------------------------------------------------------------------
uint64_t
now_ms()
{
timeval current;
   gettimeofday(&current, 0);

uint64_t ret = current.tv_sec;
   ret *= 1000;
   ret += current.tv_usec/1000;
   return ret;
}
//-----------------------------------------------------------------------------
/// return true if key already exists; otherwise add variable and return false
bool
add_var(SV_key key)
{
   for (int c = 0; c < coupled_vars.size(); ++c)
       {
         Coupled_var & cv = coupled_vars[c];
         if (key == cv.key)   return true;   // key already exists
       }

   // key is new; add it to coupled_vars
   //
Coupled_var cv = { key, 0, 0 };
   initialize(cv);
   coupled_vars.push_back(cv);

   return false;
}
//-----------------------------------------------------------------------------
void
print_vars(ostream & out)
{
   for (int c = 0; c < coupled_vars.size(); ++c)
       {
         Coupled_var & cv = coupled_vars[c];
         CERR << "   key: 0x" << hex << cv.key << dec << " ";
         offered_SVAR * svar = Svar_DB::find_var(cv.key);
         if (svar)   svar->print_name(CERR) << endl;
         else     CERR << "(unknown var)" << endl;
       }
}
//-----------------------------------------------------------------------------
void
do_Assert(const char * cond, const char * fun, const char * file, int line)
{
   CERR << "Assertion '" << cond << "' failed at "
        << file << ":" << line << endl;

   assert(0);
}
//-----------------------------------------------------------------------------
void
control_C(int)
{
   if (verbose)   CERR << pref << " unregistering processor "
                       << ProcessorID::get_id().proc << endl;

   {
     Svar_partner this_proc;
     this_proc.clear();
     this_proc.id = ProcessorID::get_id();
     this_proc.pid = getpid();
     Svar_DB::unregister_processor(this_proc);
   }

   if (verbose)   CERR << pref << " done (got ^C)" << endl;

   exit(0);
}

static struct sigaction old_ctl_C_action;
static struct sigaction new_ctl_C_action;

//-----------------------------------------------------------------------------
int usage()
{
   CERR
<< "Usage:"                            << endl
<< prog << " [options]"                << endl
<<                                        endl
<< "options:"                          << endl
<< "    --auto        automatically started (exit after last retract)"  << endl
<< "    --id num      run as processor num"  << endl
<< "    --par num     run as child of num"  << endl
<< "    --ppid num    terminate if process num has died" << endl
<< "    --gra num     run as grandchild of num"  << endl
<< "    -h, --help    print this help" << endl
<< "    -v            verbose"         << endl
<<                                        endl;

   return 1;
}
//-----------------------------------------------------------------------------
void
got_event(uint32_t event, SV_key key)
{
   if (event_port)   // event timer is running
      {
        UdpClientSocket sock(LOC, event_port);
        GOT_EVENT_c signal(sock, key, event);
        event_port = 0;
      }
}
//-----------------------------------------------------------------------------
int
main(int argc, char * argv[])
{
bool need_help = false;
bool auto_started = false;

   prog = argv[0];

   if (strrchr(prog, '/'))   prog = strrchr(prog, '/') + 1;
   snprintf(pref, sizeof(pref) - 1, "%s(%u) ", prog, getpid());

   // set default processor ID
   ProcessorID::set_own_ID(AP_num(AP_NUM));

   for (int a = 1; a < argc; )
       {
         const char * opt = argv[a++];
         const char * val = (a < argc) ? argv[a] : 0;

         if      (!strcmp(opt, "-h"))             need_help = true;
         else if (!strcmp(opt, "--help"))         need_help = true;
         else if (!strcmp(opt, "--auto"))         auto_started = true;
         else if (!strcmp(opt, "-v"))             verbose = true;
         else if (!strcmp(opt, "--id") && val)
                          { ProcessorID::set_own_ID(AP_num(atoi(val))); ++a; }
         else if (!strcmp(opt, "--par") && val)
                         { ProcessorID::set_parent_ID(AP_num(atoi(val))); ++a; }
         else if (!strcmp(opt, "--ppid") && val)
                        { parent_pid = atoi(val); ++a; }
         else if (!strcmp(opt, "--gra") && val)
                         { ProcessorID::set_grand_ID(AP_num(atoi(val))); ++a; }
         else
            {
              CERR << pref << ": Bad command line option "
                   << argv[a] << endl;
              need_help = true;
            }
       }

   if (need_help)   return usage();

   snprintf(AP_NAME, sizeof(AP_NAME), "apl%u", ProcessorID::get_id().proc);

   // serious attempt to run: run in the background
   //
   if (fork())   return 0;            // parent returns (daemonize)

   // child code...
   // register as e.g. AP210-port. We do this BEFORE closing stdout so that
   // caller of our parent waits until we have closed stdout
   //
UdpServerSocket sock(LOC, 0);
   if (verbose)  sock.set_debug(CERR);

   memset(&new_ctl_C_action, 0, sizeof(struct sigaction));
   new_ctl_C_action.sa_handler = &control_C;
   sigaction(SIGTERM, &new_ctl_C_action, &old_ctl_C_action);

Svar_partner this_proc;
   this_proc.clear();
   this_proc.id = ProcessorID::get_id();
   this_proc.pid = getpid();
   this_proc.port = sock.get_local_port();

   Svar_DB::register_processor(this_proc);

   fclose(stdout);   // cause getc() of caller to return EOF !

   for (bool goon = true; goon;)
       {
         uint8_t buffer[Signal_base::get_class_size()];
         const Signal_base * signal = Signal_base::recv(sock, buffer, 10000);
         if (signal == 0)   // no signal for 10 seconds
            {
              // if our parent has died, then we are done
              //
              if (parent_pid && !Svar_partner::pid_alive(parent_pid))
                 {
                   goon = false;
                   if (verbose)   CERR << AP_NAME
                                       << " done (parent died)" << endl;
                 }
              continue;
            }

         switch(signal->get_sigID())
            {
            case sid_DISCONNECT:   // master (local APL interpreter) disconnect
                 if (verbose)   CERR << AP_NAME << " got DISCONNECT" << endl;
                 goon = false;
                 continue;

            case sid_NEW_VARIABLE:     // a new (not yet matched) offer
                 if (verbose)   CERR << AP_NAME << " got NEW_VARIABLE" << endl;
                 {
                   const SV_key key = signal->get__NEW_VARIABLE__key();
                   offered_SVAR * svar = Svar_DB::find_var(key);
                  if (svar == 0)
                     {
                       CERR << "Could not find svar for key "
                            << key << " at " << LOC << endl;
                       continue;
                     }

                   if (! is_valid_varname(svar->varname))
                      {
                       CERR << "Bad varname ";
                       svar->print_name(CERR) << " at " << LOC << endl;
                       continue;
                      }

                   add_var(key);
                 }
                 continue;

            case sid_MAKE_OFFER:        // a new offer from a peer
                 if (verbose)   CERR << AP_NAME << " got MAKE_OFFER" << endl;
                 {
                   const SV_key key = signal->get__MAKE_OFFER__key();
                   offered_SVAR * svar = Svar_DB::find_var(key);
                  if (svar == 0)
                     {
                       CERR << "Could not find svar for key "
                            << key << " at " << LOC << endl;
                       continue;
                     }

                   if (! is_valid_varname(svar->varname))
                      {
                       CERR << "Bad varname ";
                       svar->print_name(CERR) << " at " << LOC << endl;
                       continue;
                      }

                   // make a counter offer (APs 100 and 210) or not (APnnn)
                   //
                   if (!make_counter_offer(key))   continue;   // APnnn

                   SV_Coupling coupling = NO_COUPLING;
                   Svar_DB::match_or_make(svar->varname, svar->offering.id, 
                                          this_proc, coupling);

                   add_var(key);

                   svar->offering.flags &= ~OSV_OFFER_SENT;
                   svar->set_state(true, LOC);
                 }
                 continue;

            case sid_OFFER_MATCHED:     // our offer was matched
                 if (verbose)   CERR << AP_NAME << " got OFFER_MATCHED" << endl;
                 {
                   const SV_key key = signal->get__OFFER_MATCHED__key();

                   add_var(key);
                   Svar_DB::add_event(SVE_OFFER_MATCHED,
                                      ProcessorID::get_id(), key);
                 }
                 continue;

            case sid_RETRACT_OFFER:     // ⎕SVR varname
                 if (verbose)   CERR << AP_NAME << " got RETRACT_OFFER" << endl;
                 {
                   const SV_key key = signal->get__RETRACT_OFFER__key();
                   for (int c = 0; c < coupled_vars.size(); ++c)
                       {
                         Coupled_var & cv = coupled_vars[c];
                         if (key == cv.key)
                            {
                              retract(cv);
                              coupled_vars.erase(coupled_vars.begin() + c);

                              if (coupled_vars.size() == 0 && auto_started)
                                 {
                                   if (verbose)      CERR << AP_NAME << " done"
                                      " (last variable retracted)" << endl;
                                   goon  = false;
                                   break;
                                 }
                           }
                       }
                   Svar_DB::add_event(SVE_OFFER_RETRACT,
                                      ProcessorID::get_id(), key);
                 }
                 continue;

            case sid_GET_VALUE:
                 if (verbose)   CERR << AP_NAME << " got GET_VALUE" << endl;
                 {
                   const SV_key key = signal->get__GET_VALUE__key();
                   APL_error_code error = E_VALUE_ERROR;
                   bool found = false;
                   string data;
                   for (int c = 0; c < coupled_vars.size(); ++c)
                       {
                         Coupled_var & cv = coupled_vars[c];
                         if (key == cv.key)
                            {
                              found = true;
                              error_loc = LOC;   error = get_value(cv, data);
                              break;
                           }
                       }

                   if (!found)
                      {
                        CERR << "Key 0x" << hex << key << dec
                             << " not found. Variables are:"
                             << endl;
                        print_vars(CERR);
                        error_loc = LOC;   error = E_VALUE_ERROR;
                      }

                   VALUE_IS_c response(sock, key, error, error_loc, data);
                 }
                 continue;

            case sid_ASSIGN_VALUE:
                 if (verbose)   CERR << AP_NAME << " got ASSIGN_VALUE" << endl;
                 {
                   const SV_key key = signal->get__ASSIGN_VALUE__key();
                   APL_error_code error = E_VALUE_ERROR;
                   bool found = false;
                   for (int c = 0; c < coupled_vars.size(); ++c)
                       {
                         Coupled_var & cv = coupled_vars[c];
                         if (key == cv.key)
                            {
                              found = true;
                              error_loc = LOC;   error = assign_value(cv,
                                           signal->get__ASSIGN_VALUE__value());
                              break;
                           }
                       }

                   if (!found)
                      {
                        CERR << "Key 0x" << hex << key << dec
                             << " not found. Variables are:"
                             << endl;
                        print_vars(CERR);
                        error_loc = LOC;   error = E_VALUE_ERROR;
                      }

                   ASSIGNED_c response(sock, key, error, error_loc);
                 }
                 continue;

            case sid_START_EVENT_REPORTING:
                 if (verbose)
                    CERR << AP_NAME << " got START_EVENT_REPORTING" << endl;

                 event_port = signal->get__START_EVENT_REPORTING__event_port();
                 {
                   Svar_event events = SVE_NO_EVENTS;
                   SV_key key = Svar_DB::get_events(events,
                                                    ProcessorID::get_id());
                   if (events || key)   got_event(events, key);
                 }
                 continue;

            case sid_STOP_EVENT_REPORTING:
                 if (verbose)
                    CERR << AP_NAME << " got STOP_EVENT_REPORTING" << endl;
                 event_port = 0;
                 continue;

            case sid_NEW_EVENT:
                 if (verbose)   CERR << AP_NAME << " got NEW_EVENT "
                    << event_name(Svar_event(signal->get__NEW_EVENT__event()))
                    << endl;

                 // someone has created a new event with Svar_DB::add_event()
                 // We forward it to our interpreter (if ⎕SVE is running)
                 //
                 got_event(signal->get__NEW_EVENT__event(),
                           signal->get__NEW_EVENT__key());
                 continue;

            default: CERR << pref << ": bad signal ID "
                          << signal->get_sigID() << endl;
          }
       }

   Svar_DB::unregister_processor(this_proc);
   return 0;
}
//-----------------------------------------------------------------------------
