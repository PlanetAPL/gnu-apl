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

#include <vector>

#include "../../config.h"
#include "../Svar_DB.hh"

extern ostream CERR;

using namespace std;

class UdpSocket;
class Signal_base;
class CDR_string;

extern bool verbose;
extern int event_port;
extern char pref[];   ///< a prefix for debug printouts

struct SVAR_context;

/// a coupled shared variable
struct Coupled_var
{
   SV_key key;                ///< the key for this shared variable in Svar_DB
   const CDR_string * data;   ///< the data held by this shared variable
   SVAR_context * context;    ///< a context for this shared variable
};

extern void print_vars(ostream & out);

extern vector<Coupled_var> coupled_vars;

//-----------------------------------------------------------------------------
enum APL_error_code
{
#define err_def(c, t, maj, min) E_ ## c = (maj) << 16 | (min),
#include "../Error.def"
};

/// send GOT_EVENT to the event_port (if non-zero)
extern void got_event(uint32_t event, SV_key key);

// API to AP specific functions...

/// initialize new variable \b var in this AP (due to ⎕SVO);
/// return true on error
extern bool initialize(Coupled_var & var);

/// return true if this AP automatically makes counter offers to incoming offers
extern bool make_counter_offer(SV_key key);

/// retract variable \b var in this AP (due to ⎕SVR)
extern void retract(Coupled_var & var);

/// return true if \b varname is supported by this AP
extern bool is_valid_varname(const uint32_t * varname);

/// assign a new value
extern APL_error_code assign_value(Coupled_var & var, const string & data);

/// get the current value
extern APL_error_code get_value(Coupled_var & var, string & data);

/// the place where an error was detected
extern string error_loc;

/// Stringify x.
#define STR(x) #x

/// The current location in the source file.
#define LOC Loc(__FILE__, __LINE__)

/// The location line l in file f.
#define Loc(f, l) f ":" STR(l)

//-----------------------------------------------------------------------------

