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
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "CDR.hh"
#include "CharCell.hh"
#include "Common.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "LibPaths.hh"
#include "ProcessorID.hh"
#include "Quad_SVx.hh"
#include "Svar_DB.hh"
#include "Svar_signals.hh"
#include "Workspace.hh"

extern char **environ;

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

Quad_SVC Quad_SVC::fun;
Quad_SVO Quad_SVO::fun;
Quad_SVQ Quad_SVQ::fun;
Quad_SVR Quad_SVR::fun;
Quad_SVS Quad_SVS::fun;

APL_time Quad_SVE::timer_end = 0;

//=============================================================================
/**
    return true iff \b filename is an executable file
 **/
bool
Quad_SVx::is_executable(const char * file_and_args)
{
string filename(file_and_args);
const char * end = strchr(file_and_args, ' ');
   if (end)   filename.resize(end - file_and_args);

   return access(filename.c_str(), X_OK) == 0;
}
//-----------------------------------------------------------------------------
void
Quad_SVx::start_AP(AP_num proc, bool startup)
{
const char * dirs[] = { "", "/APs" };
   enum { dircount = sizeof(dirs) / sizeof(*dirs) };

const AP_num own_ID = ProcessorID::get_own_ID();
const AP_num par_ID = ProcessorID::get_parent_ID();

   // start_AP() is called from two places:
   // on start-up of the interpreter (and then startup == true), or
   // after making an offer to an AP that is not yet running
   //
   for (int d = 0; d < dircount; ++d)
       {
         char filename[PATH_MAX + 1];
         const char * verbose = "";
         Log(LOG_shared_variables)   verbose = " -v";
         if (startup)
            snprintf(filename, PATH_MAX,
                     "%s%s/APnnn --id %u --par %u%s --ppid %u",
                     LibPaths::get_APL_bin_path(), dirs[d],
                     own_ID, par_ID, verbose, getpid());
         else
            snprintf(filename, PATH_MAX,
                     "%s%s/AP%u --id %u --par %u --gra %u --auto%s --ppid %u",
                     LibPaths::get_APL_bin_path(), dirs[d], proc, proc,
                     own_ID, par_ID, verbose, getpid());

         if (!is_executable(filename))   continue;

         Log(LOG_shared_variables)
            CERR << "found executable: " << filename << endl;

         FILE * fp = popen(filename, "r");
         if (fp == 0)
            {
              CERR << "popen(" << filename << " failed: " << strerror(errno)
                   << endl;
              continue;
            }

         for (int cc; (cc = getc(fp)) != EOF;)   CERR << (char)cc;
         CERR << endl;

         pclose(fp);
         return;
       }

   CERR << "No binary found for AP " << proc << " (interpreter path = "
        << LibPaths::get_APL_bin_path() << ")" << endl;

   return;
}
//=============================================================================
Token
Quad_SVC::eval_AB(Value_P A, Value_P B)
{
   if (A->get_rank() > 2)   RANK_ERROR;
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
   if (A->get_rows() != var_count)               LENGTH_ERROR;
   if (A->get_rank() > 0 && A->get_cols() != 4)   LENGTH_ERROR;
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

const APL_Float qct = Workspace::get_CT();
const Cell * cA = &A->get_ravel(0);

const Shape shZ(var_count, 4);
Value_P Z(var_count > 1 ? new Value(shZ, LOC) : new Value(4, LOC));

   loop(z, var_count)
      {
        // construct control value
        //
        int ctl = 0;
        if (A->is_skalar())
           {
             const bool val = A->get_ravel(0).get_near_bool(qct);
              ctl = val ? ALL_SVAR_CONTROLS : NO_SVAR_CONTROL;
           }
        else if (A->is_vector())
           {
             if (A->get_ravel(0).get_near_bool(qct))   ctl |= SET_BY_1;
             if (A->get_ravel(1).get_near_bool(qct))   ctl |= SET_BY_2;
             if (A->get_ravel(2).get_near_bool(qct))   ctl |= USE_BY_1;
             if (A->get_ravel(3).get_near_bool(qct))   ctl |= USE_BY_2;
           }
        else // matrix
           {
             if (cA++->get_near_bool(qct))   ctl |= SET_BY_1;
             if (cA++->get_near_bool(qct))   ctl |= SET_BY_2;
             if (cA++->get_near_bool(qct))   ctl |= USE_BY_1;
             if (cA++->get_near_bool(qct))   ctl |= USE_BY_2;
           }

        // set control.
        //
        Symbol * sym = Workspace::lookup_existing_symbol(vars[z]);
        if (sym == 0)   throw_symbol_error(vars[z], LOC);

        const SV_key key = sym->get_SV_key();
        ctl = Svar_DB::set_control(key, Svar_Control(ctl));

        new (Z->next_ravel())   IntCell(ctl & SET_BY_1 ? 1 : 0);
        new (Z->next_ravel())   IntCell(ctl & SET_BY_2 ? 1 : 0);
        new (Z->next_ravel())   IntCell(ctl & USE_BY_1 ? 1 : 0);
        new (Z->next_ravel())   IntCell(ctl & USE_BY_2 ? 1 : 0);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Quad_SVC::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

const Shape shZ(var_count, 4);
Value_P Z(var_count > 1 ? new Value(shZ, LOC) : new Value(4, LOC));

   loop(z, var_count)
      {
        Symbol * sym = Workspace::lookup_existing_symbol(vars[z]);
        if (sym == 0)   throw_symbol_error(vars[z], LOC);

        const SV_key key = sym->get_SV_key();
        const Svar_Control control = Svar_DB::get_control(key);

        new (Z->next_ravel())   IntCell(control & SET_BY_1 ? 1 : 0);
        new (Z->next_ravel())   IntCell(control & SET_BY_2 ? 1 : 0);
        new (Z->next_ravel())   IntCell(control & USE_BY_1 ? 1 : 0);
        new (Z->next_ravel())   IntCell(control & USE_BY_2 ? 1 : 0);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Quad_SVE::Quad_SVE()
   : NL_SystemVariable(ID_QUAD_SVE)
{
   Symbol::assign(Value::Zero_P, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_SVE::assign(Value_P value, const char * loc)
{
   // ⎕SVE←X merely remembers the timer expiration time. The timer is only
   // started if ⎕SVE is referenced (in Quad_SVE::get_apl_value())
   //
   if (!value->is_skalar())   RANK_ERROR;

const APL_time duration = 1000000 * value->get_ravel(0).get_real_value();
   if (duration < 0)   DOMAIN_ERROR;

   if (duration == 0.0)
      {
        // ⎕SVE←0 causes get_apl_value() to return immediately. We achieve
        // that by setting the end time to the past.
        //
        timer_end = now() - 1;   // end time in the past
        return;
      }

   // set timer end
   //
   timer_end = now() + duration;
}
//-----------------------------------------------------------------------------
Value_P
Quad_SVE::get_apl_value() const
{
const APL_time current_time = now();
const APL_time wait = timer_end - current_time;

   // if timer has expired, return 0
   //
   if (wait <= 0)
      {
        Svar_DB::clear_all_events();
        return Value::Zero_P;
      }

   // At this point the timer is still running.
   // we use a new UDP port for event reporting and communicate it to APnnn
   //
bool got_event = false;
   {
     UdpClientSocket event_sock(LOC, ProcessorID::get_APnnn_port());
     Log(LOG_shared_variables)   event_sock.set_debug(CERR);

     {
       START_EVENT_REPORTING_c start(event_sock, event_sock.get_local_port());
     }

     uint8_t buf[Signal_base::get_class_size()];
     const Signal_base * resp = Signal_base::recv(event_sock, buf, wait / 1000);

     if (resp)   // got event
        {
          got_event = true;
          Log(LOG_shared_variables)
             {
               const uint64_t key = resp->get__GOT_EVENT__key();
               const offered_SVAR * svar = Svar_DB::find_var(SV_key(key));
               CERR << "⎕SVE got event '"
                    << event_name(Svar_event(resp->get__GOT_EVENT__event()))
                    << "' from shared variable ";
               if (svar)   svar->print_name(CERR);
               else        CERR << "unknown key " << key;
               CERR << endl;
             }
        }

     STOP_EVENT_REPORTING_c stop(event_sock);
     event_sock.udp_close();
   }

   // referencing ⎕SVE always clears all events
   //
   Svar_DB::clear_all_events();

   // if no event has occurred (timeout) then return 0
   //
  if (!got_event)   return Value::Zero_P;

   // we have got an event; return remaining time.
   //
const APL_Float remaining = timer_end - now();

   if (remaining < 0)   return Value::Zero_P;

Value_P Z(new Value(LOC));
   new (&Z->get_ravel(0))   FloatCell(0.000001 * remaining);
   return Z;
}
//=============================================================================
/**
 ** Offer variables in B to corresponding processors in A
 **/
Token
Quad_SVO::eval_AB(Value_P A, Value_P B)
{
   if (A->get_rank() > 1)   RANK_ERROR;
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> apl_vars(var_count);
   B->to_varnames(apl_vars,   false);

vector<UCS_string> surrogates(var_count);
   B->to_varnames(surrogates, true);

   if (A->get_rank() == 1 && A->element_count() != var_count)   LENGTH_ERROR;

Value_P Z(var_count > 1 ? new Value(var_count, LOC) : new Value(LOC));

   loop(z, var_count)
      {
        ShapeItem a = A->is_skalar() ? 0 : z;
        uint32_t vname[MAX_SVAR_NAMELEN + 1];
        const int vlen = surrogates[z].size();
        if (vlen > MAX_SVAR_NAMELEN)   DOMAIN_ERROR;
        bool bad_name = false;

        loop(v, vlen)
           {
             const Unicode uni = surrogates[z][v];
             if (!Avec::is_symbol_char(uni))   bad_name = true;
             vname[v] = uni;
           }
        vname[vlen] = 0;

        if (bad_name || vlen == 0)
           {
             new (Z->next_ravel()) IntCell(NO_COUPLING);
             continue;
           }

        // make sure current name class allows ⎕SVO
        //
        Symbol * sym = Workspace::lookup_symbol(apl_vars[z]);
        assert(sym);

        const AP_num proc = AP_num(A->get_ravel(a).get_int_value());

        ValueStackItem * vsp = sym->top_of_stack();
        Assert(vsp);

        switch(sym->get_nc())
           {
             case NC_UNUSED_USER_NAME:
             case NC_VARIABLE:
                  break;

             case NC_SHARED_VAR:
                  {
                    // CERR << "Shared variable " << apl_vars[z]
                    //      << " is already shared" << endl;

                    const SV_key key = sym->get_SV_key();
                    new (Z->next_ravel()) IntCell(Svar_DB::get_coupling(key));
                  }
                  continue;   // next z

             default: // function etc.
                  Q(sym->get_nc())   DOMAIN_ERROR;
           }

        SV_Coupling coupling = NO_COUPLING;
        const SV_key key = share_one_variable(proc, vname, coupling);
        if (coupling >= SV_OFFERED)   // offered or coupled
           {
             sym->share_var(key);
           }

        new (Z->next_ravel()) IntCell(coupling);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
SV_key
Quad_SVO::share_one_variable(AP_num to_ap, const uint32_t * vname,
                             SV_Coupling & coupling)
{
   // if to_ap is a (dependent) AP then start it if necessary...
   //
AP_num3 to_proc(to_ap, ProcessorID::get_own_ID(), ProcessorID::get_parent_ID());
bool auto_started = false;

   if (to_ap < AP_FIRST_USER)   // to_proc is an AP
      {
        if (!Svar_DB::is_registered(to_proc))
           {
             start_AP(to_ap, false);
             auto_started = true;
           }
      }
   else                         // to_proc is an independent processor
      {
        new (&to_proc)   AP_num3(to_ap, AP_NULL, AP_NULL);
      }

Svar_partner from;
   from.clear();
   from.id = ProcessorID::get_id();
   from.pid = getpid();
   from.port = ProcessorID::get_APnnn_port();

offered_SVAR * svar = Svar_DB::match_or_make(vname, to_proc, from, coupling);

   if (coupling == SV_COUPLED)
      {
        // if an AP was started automatically, then coupling could be
        // SV_COUPLED or SV_OFFERED, depending on how fast the AP did start
        // up. We pretend here that it started slowly so that auto-started
        // APs always return SV_OFFERED
        //
        if (auto_started)   coupling = SV_OFFERED;
        
        return svar->key;
      }

   // if something went wrong, complain.
   //
   if (coupling == NO_COUPLING)
      {
        Log(LOG_shared_variables)
           CERR << "Svar_DB::match_or_store() returned unexpected 0" << endl;
        return 0;
      }

   Assert(coupling == SV_OFFERED);

   // if the coupling partner was registered, then a signal exchange between
   // APnnn and the partner is taking place. Wait a little until the signal
   // exchange is finished, so that the next ⎕SVO will return SV_COUPLED.
   //
   for (int t = 0; t < 20; ++t)   // wait at most 20 milliseconds
       {
         if (!(svar->offering.flags & OSV_OFFER_SENT))   break;
         usleep(1000);   // wait 1 ms
       }

   return svar->key;
}
//-----------------------------------------------------------------------------
/**
 ** Return degree of coupling for variables in B
 **/
Token
Quad_SVO::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

Value_P Z(var_count > 1 ? new Value(var_count, LOC) : new Value(LOC));

   loop(z, var_count)
      {
        Symbol * sym = Workspace::lookup_existing_symbol(vars[z]);
        if (sym == 0)   // variable does not exist
           {
             new (Z->next_ravel()) IntCell(0);
             continue;
           }

        // the coupling of a non-shared variables has coupling 0
        //
        if (sym->get_nc() != NC_SHARED_VAR)
           {
             new (Z->next_ravel()) IntCell(0);
             continue;
           }

        const SV_key key = sym->get_SV_key();
        const SV_Coupling coupling = Svar_DB::get_coupling(key);
         new (Z->next_ravel()) IntCell(coupling);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_SVQ::eval_B(Value_P B)
{
   if (B->get_rank() > 1)        RANK_ERROR;
   if (B->element_count() > 1)   LENGTH_ERROR;

Value_P Z;

   // B is empty or has 1 element.
   //
   if (B->element_count()  == 0)    // B empty: return processors offering
      {
        Z = get_processors();
      }
   else                            // return variables offered by processor ↑B
      {
        const AP_num proc = AP_num(B->get_ravel(0).get_int_value());
        Z = get_variables(proc);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Quad_SVQ::get_processors()
{
   // shared variable processors are identified by one of the following:
   //
   // 1. auxiliary processors that can be started on demand
   //    (e.g. AP100, AP210). These APs are collocated with the apl binary,
   //    either in the same directory or in the 'APs' subdirectory,
   //
   // 2. running processors that have offered variables in Svar_DB.
   //
vector<int32_t> processors;

   // case 1...
   //
const char * dirs[] = { "", "/APs" };
   enum { dircount = sizeof(dirs) / sizeof(*dirs) };

   for (int d = 0; d < dircount; ++d)
       {
         char dirname[PATH_MAX + 1];
         snprintf(dirname, PATH_MAX, "%s%s", LibPaths::get_APL_bin_path(),
                  dirs[d]);
         dirname[PATH_MAX] = 0;
         DIR * dir = opendir(dirname);
         if (dir == 0)
            {
              CERR << "Could not open " << dirname << " : "
                   << strerror(errno) << endl;
              continue;
            }

         for (;;)   // scan files in directory
             {
                dirent * entry = readdir(dir);
                if (entry == 0)   break;   // directory done

                if (entry->d_type != DT_REG)   continue; // not a regular file

                char filename[PATH_MAX + 1];
                snprintf(filename, PATH_MAX, "%s/%s", dirname, entry->d_name);
                filename[PATH_MAX] = 0;

                if (!is_executable(filename))   continue;

                int apnum;
                if (sscanf(entry->d_name, "AP%u", &apnum) != 1)   continue;

                char expected[PATH_MAX + 1];
                snprintf(expected, sizeof(expected), "AP%u", apnum);
                if (strcmp(entry->d_name, expected))   continue;

                processors.push_back(apnum);
             }

         closedir(dir);
       }

   // case 2...
   //
   Svar_DB::get_processors(ProcessorID::get_own_ID(), processors);

   // sort and remove duplicates
   //
vector<int32_t> sorted;
   while (processors.size())
      {
        // find smallest
        //
        int smallest = processors[0];
        for (int s = 1; s < processors.size(); ++s)
            if (smallest > processors[s])   smallest = processors[s];

       // add smallest to sorted
       //
       sorted.push_back(smallest);

       // remove smallest from processors
       //
        for (int s = 0; s < processors.size();)
            {
              if (processors[s] != smallest)
                 {
                   ++s;
                 }
              else
                 {
                   processors[s] = processors.back();
                   processors.pop_back();
                 }
            }
      }

Value_P Z(new Value(sorted.size(), LOC));
   loop(z, sorted.size())   new (&Z->get_ravel(z)) IntCell(sorted[z]);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_SVQ::get_variables(AP_num proc)
{
vector<const uint32_t *> vars;
   Svar_DB::get_variables(ProcessorID::get_own_ID(), proc, vars);

const int count = vars.size();
vector<int> varlen;
ShapeItem max_len = 0;
   loop(z, count)
      {
        int len = 0;
        const uint32_t * vz = vars[z];
        for (; *vz; ++vz)   ++len;
        varlen.push_back(len);
         if (max_len < len)   max_len = len;
      }

const Shape shZ(count, max_len);
Value_P Z(new Value(shZ, LOC));

   loop(z, count)
   loop(c, max_len)
      {
        if (c < varlen[z])   new (Z->next_ravel()) CharCell(Unicode(vars[z][c]));
        else                 new (Z->next_ravel()) CharCell(UNI_ASCII_SPACE);
      }

   Z->set_default(*Value::Spc_P);   // prototype: character
   return Z;
}
//=============================================================================
Token
Quad_SVR::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

Value_P Z(var_count > 1 ? new Value(var_count, LOC) : new Value(LOC));

   loop(z, var_count)
      {
        Symbol * sym = Workspace::lookup_existing_symbol(vars[z]);
        if (sym == 0)   DOMAIN_ERROR;

        const SV_Coupling coupling = sym->unshare_var();
        new (Z->next_ravel()) IntCell(coupling);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_SVS::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

const Shape shZ(var_count, 4);
Value_P Z(var_count > 1 ? new Value(shZ, LOC) : new Value(4, LOC));

   loop(z, var_count)
      {
        Symbol * sym = Workspace::lookup_existing_symbol(vars[z]);
        if (sym == 0)   throw_symbol_error(vars[z], LOC);

        const SV_key key = sym->get_SV_key();
        const Svar_state state = Svar_DB::get_state(key);

        new (Z->next_ravel())   IntCell(state & SET_BY_1 ? 1 : 0);
        new (Z->next_ravel())   IntCell(state & SET_BY_2 ? 1 : 0);
        new (Z->next_ravel())   IntCell(state & USE_BY_1 ? 1 : 0);
        new (Z->next_ravel())   IntCell(state & USE_BY_2 ? 1 : 0);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
