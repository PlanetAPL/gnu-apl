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

#include <cassert>
#include <iostream>

#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "APmain.hh"
#include "CDR_string.hh"
#include "Svar_DB.hh"
#include "Svar_signals.hh"

using namespace std;

//-----------------------------------------------------------------------------
struct SVAR_context
{
   SVAR_context()
  {}
};
//-----------------------------------------------------------------------------
int
set_ACK(Coupled_var & var, int exco)
{
const uint8_t _cdr[] =
   {
     0x00, 0x00, 0x20, 0x20, // ptr
     0x00, 0x00, 0x00, 0x20, // nb: total len
     0x00, 0x00, 0x00, 0x01, // nelm: element count
     0x01, 0x00, 0x00, 0x00, // type, rank, pad, pad
     uint8_t(exco), uint8_t(exco>>8), uint8_t(exco>>16), uint8_t(exco>>24),
     0x00, 0x00, 0x00, 0x00, // pad
     0x00, 0x00, 0x00, 0x00, // pad
     0x00, 0x00, 0x00, 0x00, // pad
   };

const CDR_string * cdr = new CDR_string(_cdr, sizeof(_cdr));
   delete var.data;
   var.data = cdr;
   Svar_DB::set_state(var.key, true, LOC);

   return exco;
}
//-----------------------------------------------------------------------------
void
handle_var(Coupled_var & var)
{
FILE * fp = 0;
const char * cmd = 0;

const CDR_string & cdr = *var.data;
   if (cdr.size() < 20)   // less than min. size of CDR header
      {
        CERR << "CDR record too short (" << cdr.size()
             << " bytes)" << endl;
        set_ACK(var, 444);
        return;
      }


   if (cdr[13] != 1)   // not a vector
      {
        CERR << "Bad CDR rank (" << int(cdr[13]) << endl;
        set_ACK(var, 445);
        return;
      }

         if (cdr[12] != 4)   // not a char vector
            {
              CERR << "Bad CDR record type (" << int(cdr[12]) << endl;
              set_ACK(var, 446);
              return;
            }

         cmd = string((const char *)cdr.get_items() + 20,
                      cdr.size() - 20).c_str();

         if (verbose)   CERR << pref << " got command " << cmd << endl;

         fp = popen(cmd, "r");
         if (fp == 0)   // bad command
            {
              CERR << pref << " popen() failed" << endl;
              set_ACK(var, 1);  // 1 := INVALID COMMAND
              return;
            }

         for (;;)
             {
               const int cc = fgetc(fp);
               if (cc == EOF)   break;
               CERR << char(cc);
             }

         CERR << flush;
         const int result = pclose(fp);
         if (verbose)   CERR << pref << " finished command with exit code "
                             << result << endl;

         set_ACK(var, WEXITSTATUS(result));
}
//-----------------------------------------------------------------------------
bool
is_valid_varname(const uint32_t * varname)
{
   // AP100 supports all variable names
   //
   return *varname;   // varname is not empty
}
//-----------------------------------------------------------------------------
bool
initialize(Coupled_var & var)
{
   Svar_DB::set_state(var.key, true, LOC);
   Svar_DB::set_control(var.key, USE_BY_1);
   return false;   // OK
}
//-----------------------------------------------------------------------------
bool
make_counter_offer(SV_key key)
{
   return true;   // make counter offer
}
//-----------------------------------------------------------------------------

APL_error_code
assign_value(Coupled_var & var, const string & data)
{
   // check CDR size, type, and shape of data. We expect a char string
   // for both the CTL and the DAT variable.
   //
   if (data.size() < sizeof(CDR_header))   // too short
      { error_loc = LOC;   return E_LENGTH_ERROR; }

   delete var.data;
   var.data = new CDR_string((const uint8_t *)data.c_str(), data.size());
   handle_var(var);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
APL_error_code
get_value(Coupled_var & var, string & data)
{
   if (var.data == 0)   return E_VALUE_ERROR;

   data = string((const char *)(var.data->get_items()), var.data->size());
   error_loc = "no_error";   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
void
retract(Coupled_var & var)
{
SVAR_context * ctx = var.context;
   if (ctx)
      {
        delete ctx;
      }
}

//-----------------------------------------------------------------------------
