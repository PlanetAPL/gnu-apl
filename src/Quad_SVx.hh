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

#ifndef __SHARED_VARIABLES_HH_DEFINED__
#define __SHARED_VARIABLES_HH_DEFINED__

#include "QuadFunction.hh"
#include "SystemVariable.hh"
#include "UdpSocket.hh"

//-----------------------------------------------------------------------------
/// some helper functions to start auxiliary processors
class Quad_SVx
{
public:
   /// start the multiplexer process for processor \b proc
   static void start_AP(AP_num proc, bool startup);

protected:
   /// return true iff \b filename is executable by everybody
   static bool is_executable(const char * filename);

   /// disconnect from auxiliary processor proc if connected.
   static void disconnect(AP_num proc);

   /// an auxiliary processor and its handler port
   struct AP_handler
      {
        AP_num          ap;         ///< AP number
        UdpClientSocket handler;    ///< handler port for the AP
        int             varcount;   ///< reference count for offered variables
      };

   /// a list of APs connected to this APL interpreter
   static vector<AP_handler> connected_APs;
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕SVC (Shared Variable Control).
 */
class Quad_SVC : public QuadFunction, Quad_SVx
{
public:
   /// Constructor.
   Quad_SVC() : QuadFunction(TOK_QUAD_SVC) {}

   static Quad_SVC fun;         ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_AB().
   virtual Token eval_B(Value_P B);
};
//-----------------------------------------------------------------------------
/**
   The system variable ⎕SVE (Shared Variable Event).
 */
class Quad_SVE : public NL_SystemVariable, Quad_SVx
{
public:
   /// Constructor.
   Quad_SVE();

protected:
   /// overloaded Symbol::assign()
   virtual void assign(Value_P value, const char * loc);

   /// Overloaded Symbol::get_apl_value().
   virtual Value_P get_apl_value() const;

   /// when the current ⎕SVE timer expires (as float)
   static APL_time timer_end;
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-SVO (Shared Variable Offer).
 */
class Quad_SVO : public QuadFunction, Quad_SVx
{
public:
   /// Constructor.
   Quad_SVO() : QuadFunction(TOK_QUAD_SVO) {}

   static Quad_SVO fun; ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_AB().
   virtual Token eval_B(Value_P B);

   /// share one variable
   SV_key share_one_variable(AP_num proc, const uint32_t * vname,
                             SV_Coupling & coupling);
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-SVQ (Shared Variable Query).
 */
class Quad_SVQ : public QuadFunction, Quad_SVx
{
public:
   /// Constructor.
   Quad_SVQ() : QuadFunction(TOK_QUAD_SVQ) {}

   static Quad_SVQ          fun;         ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_B(Value_P B);

   /// return processors with matching offers
   Value_P get_processors();

   /// return variables offered by processor proc
   Value_P get_variables(AP_num proc);
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕SVR (shared Variable Retraction).
 */
class Quad_SVR : public QuadFunction, Quad_SVx
{
public:
   /// Constructor.
   Quad_SVR() : QuadFunction(TOK_QUAD_SVR) {}

   static Quad_SVR fun;         ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_B(Value_P B);
};
//=============================================================================
/**
   The system function ⎕SVS (Shared Variable State).
 */
class Quad_SVS : public QuadFunction, Quad_SVx
{
public:
   /// Constructor.
   Quad_SVS() : QuadFunction(TOK_QUAD_SVS) {}

   static Quad_SVS          fun;         ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_B(Value_P B);
};
//-----------------------------------------------------------------------------

#endif // __SHARED_VARIABLES_HH_DEFINED__
