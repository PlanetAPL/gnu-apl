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
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <iomanip>
#include <iostream>
#include <vector>

#include "CDR_string.hh"
#include "APmain.hh"
#include "Svar_signals.hh"

using namespace std;

struct SVAR_context
{
   SVAR_context(Coupled_var & var)
   : var_shared(var)
  {}

   /// the (shared) variable
   Coupled_var & var_shared;
};
//-----------------------------------------------------------------------------
bool
is_valid_varname(const uint32_t * varname)
{
   // APnnn supports all variable names
   //
   return *varname;   // varname is not empty
}
//-----------------------------------------------------------------------------
bool
initialize(Coupled_var & var)
{
   Svar_DB::set_state(var.key, true, LOC);
   return false;   // OK
}
//-----------------------------------------------------------------------------
bool
make_counter_offer(SV_key key)
{
   // APnnn makes no counter offers.
   // Instead it creates an offer mismatch event
   //
   got_event(SVE_OFFER_MISMATCH, key);

   return false;   // no counter offer
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
   error_loc = "no_error";
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
        ctx->var_shared.context = 0;
        delete ctx;
      }
}
//-----------------------------------------------------------------------------
