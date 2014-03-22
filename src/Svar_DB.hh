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

#ifndef __SVAR_DB_HH_DEFINED__
#define __SVAR_DB_HH_DEFINED__

#include "ProcessorID.hh"
#include "Common.hh"

#include <semaphore.h>
#include <stdint.h>

#include <vector>

#include <iostream>

using namespace std;

class UdpSocket;

/// The coupling level of a shared variable
enum SV_Coupling
{
   NO_COUPLING = 0,   ///< not offered or coupled
   SV_OFFERED  = 1,   ///< offered but not coupled,
   SV_COUPLED =  2    ///< coupled
};

/** shared variables control (what is NOT allowed). Terminology:
    _OFF: the parner that has first offered the variable,
    _ACC: the parner that has accepted the  offer
    _1: the partner reading ot changing the control vector
    _2: the other partner of _1 (if any)

   Normally the APL interpreter is _1 == _OFF and the
   partner is _2 == _ACC. If not (eg. when the partner offers first) then
   the functions set_control() and get_control() map the _1 and _2 arg (of
   set_control()) and result (of get_control()) to the proper _OFF and _ACC
   int in in the flags of the corresponding partner.

 **/
enum Svar_Control
{
                                //   SET USE
                                //   O A O A
   SET_BY_OFF         = 0x08,   ///< 1 0 0 0: offering proc  can't set (again)
   SET_BY_ACC         = 0x04,   ///< 0 1 0 0: accepting proc  can't set (again)
   USE_BY_OFF         = 0x02,   ///< 0 0 1 0: offering proc  can't (yet) use
   USE_BY_ACC         = 0x01,   ///< 0 0 0 1: accepting proc  can't (yet) use
   NO_SVAR_CONTROL    = 0,      ///< 0 0 0 0: everything allowed
   ALL_SVAR_CONTROLS  = SET_BY_OFF | SET_BY_ACC | USE_BY_OFF | USE_BY_ACC,
   SET_BY_1           = SET_BY_OFF,   ///< if 1 is the offering partner
   SET_BY_2           = SET_BY_ACC,   ///< if 1 is the offering partner
   USE_BY_1           = USE_BY_OFF,   ///< if 1 is the offering partner
   USE_BY_2           = USE_BY_ACC,   ///< if 1 is the offering partner
};

/// the state of the shared variable
enum Svar_state
{
                                                 //    SET USE
                                                 //    O A O A
   SVS_NOT_SHARED   = NO_SVAR_CONTROL,           ///<  0 0 0 0
   SVS_IDLE         = USE_BY_OFF | USE_BY_ACC,   ///<  0 0 1 1
   SVS_OFF_HAS_SET  = SET_BY_OFF | USE_BY_OFF,   ///<  1 0 1 0
   SVS_ACC_HAS_SET  = SET_BY_ACC | USE_BY_ACC    ///<  0 1 0 1
};

/// events for a shared variable
enum Svar_event
{
   SVE_NO_EVENTS          = 0,        /// no event pending
   SVE_OFFER_MISMATCH     = 0x0001,   ///< not matching ⎕SVO
   SVE_OFFER_MATCHED      = 0x0002,   ///< matching ⎕SVO
   SVE_OFFER_RETRACT      = 0x0004,   ///< ⎕SVR
   SVE_ACCESS_CONTROL     = 0x0008,   ///< ⎕SVC by partner
   SVE_USE_BY_OFF_SUCCESS = 0x0010,   ///< CTL is 1 x x x and offerer USEs var
   SVE_USE_BY_OFF_FAILED  = 0x0020,   ///< CTL is x 1 x x and offerer USE failed
   SVE_SET_BY_OFF_SUCCESS = 0x0040,   ///< CTL is x x 1 x and offerer SETs var
   SVE_SET_BY_OFF_FAILED  = 0x0080,   ///< CTL is x x x 1 and offerer SET failed
   SVE_USE_BY_ACC_SUCCESS = 0x0100,   ///< CTL is 1 x x x and acceptor USEs var
   SVE_USE_BY_ACC_FAILED  = 0x0200,   ///< CTL is x 1 x x and acc. USE failed
   SVE_SET_BY_ACC_SUCCESS = 0x0400,   ///< CTL is x x 1 x and acceptor SETs var
   SVE_SET_BY_ACC_FAILED  = 0x0800,   ///< CTL is x x x 1 and acc. SET failed
};

/// return the name for event \b ev
extern const char * event_name(Svar_event ev);

/// SV_key uniquely identifies a shared variable. It is created when the
/// variable is first offered and passed to the share partner when the offer
/// is matched. The partner (offering or accepting) is identified by the AP_num
/// of the partner (since pid and port may change when processes are forked).
typedef uint64_t SV_key;

/// flags of a shared variable
enum OSV_flags
{
   OSV_NONE              = 0,                    ///< nothing

   // the lower 4 bits are the Svar_Control bits
   //
   OSV_CTL_MASK          = ALL_SVAR_CONTROLS,   ///< mask for control bits
   OSV_OFFER_SENT        = 0x10,                ///< an offer (⎕SVO) was sent
   OSV_EVENT             = 0x20,                ///< an event has occurred
};
//-----------------------------------------------------------------------------
/// one of the two partners of a shared vaiable
struct Svar_partner
{
   /// whether the partner is still participating. This is int rather than bool
   /// so that we can add the alive()s of both partners in order to get the
   /// coupling. We use port rather than proc for this purpose, since proc
   /// can be 0 (general offers).
   int alive() const   { return port ? 1 : 0; }

   /// return true if the pid of this partner is alive (as per
   /// /proc entry for the pid)
   bool pid_alive() const   { return pid_alive(pid); }

   /// return true if the process with pid p is alive (as per /proc entry p)
   static bool pid_alive(pid_t p);

   /// clear this partner
   void clear() { memset(this, 0, sizeof(*this)); }

   /// set the control bits of this partner (as seen by the offering partner)
   void set_control(Svar_Control ctl)
      { flags = (flags &!OSV_CTL_MASK) | ctl; }

   /// return the control bits of this partner (as seen by the offering partner)
   Svar_Control get_control() const
      { return Svar_Control(flags & OSV_CTL_MASK); }

   /// print this partner
   ostream & print(ostream & out) const;

   /// the processor
   AP_num3 id;

   /// the PID of \b id
   pid_t pid;

   /// the UDP port for contacting \b id
   uint16_t port;

   /// flags controlled by this partner. The numbers 1 and 2 in the SET_BY_1,
   /// SET_BY_2 bits refer to this partner as 1 and the other partner as 2.
   uint16_t flags;
};
//-----------------------------------------------------------------------------
/// one Svar_partner and its events
struct Svar_partner_events
{
   /// the partner
   Svar_partner partner;

   /// the events for the partner
   Svar_event   events;

   /// clear everything
   void clear() { partner.clear();   events = SVE_NO_EVENTS; }
};
//-----------------------------------------------------------------------------

/// one  shared variable.
struct offered_SVAR
{
   /// a key that uniquely identifies this variable
   SV_key key;

   /// true if this is a valid entry.
   bool valid() const   { return get_coupling() != NO_COUPLING; }

   /// invalidate this entry
   void clear()   { memset(this, 0, sizeof(*this)); }

   /// remove accepting partner
   void remove_accepting();

   /// remove offering partner
   void remove_offering();

   /// remove an outdated variable, inform partners that are still alive.
   /// increment count iff removed
   void remove_stale(int & count);

   /// retract the offer made by the calling ID.
   SV_Coupling retract();

   /// return the partner that stores the data (the partner with the
   /// smallest ID)
   uint16_t data_owner_port() const;

   /// complain about \b proc
   void bad_proc(const char * function, const AP_num3 & ap) const;

   /// return true if UCS_other matches varname (could be a wildcard match)
   bool match_name(const uint32_t * UCS_other) const;

   /// return the coupling of the variable
   SV_Coupling get_coupling() const
       { return SV_Coupling(offering.alive() + accepting.alive()); }

   /// return the control bits of this variable
   Svar_Control get_control() const;

   /// set the control bits of this variable
   void set_control(Svar_Control ctl);

   /// return the state of this variable
   Svar_state get_state() const;

   /// update the state when using or setting this variable, and clear events
   void set_state(bool used, const char * loc);

   /// return true iff the calling partner may use the current value
   bool may_use(int attempt) const;

   /// return true iff the calling partner may set the current value
   bool may_set(int attempt) const;

   /// return the share partner of this variable
   Svar_partner get_peer() const;

   /// print the variable
   void print(ostream & out) const;

   /// print a name
   static ostream & print_name(ostream & out, const uint32_t * name,
                               int len = MAX_SVAR_NAMELEN);

   /// print the name of this variable
   ostream & print_name(ostream & out) const
      { return print_name(out, varname, 0); }

   /// name of the offered variable (UCS), 0-terminated
   uint32_t varname[MAX_SVAR_NAMELEN + 1];
 
   /// the partner that has offered the variable first
   Svar_partner offering;

   /// the partner that has accepted the offer from \b offering
   Svar_partner accepting;

   /// state of this variable
   Svar_state state;
};
//-----------------------------------------------------------------------------
/// the memory being shared
struct Svar_DB_memory
{
   /// see Svar_DB::match_or_make(), sema is acquired
   offered_SVAR * match_or_make(const uint32_t * UCS_varname,
                                const AP_num3 & to,
                                const Svar_partner & from,
                                SV_Coupling & coupling);

   /// register partner \b p in this database
   void register_processor(const Svar_partner & p);

   /// return the registered processor \b proc
   Svar_partner_events get_registered(const AP_num3 & id) const;

   /// return true if \b id is not used by any registered proc, parent, or grand
   bool is_unused_id(AP_num id) const;

   /// return an id >= AP_FIRST_USER that is not mentioned in any registered
   /// processor (i,e, not proc, parent. or grand)
   AP_num get_unused_id() const;

   /// return the UDP port number for processor \b proc with parent \b parent
   uint16_t get_udp_port(AP_num proc, AP_num parent) const;

   /// remove \b p from this database
   void unregister_processor(const Svar_partner & p);

   /// remove stale variables using \b proc
   void remove_stale();

   /// see Svar_DB::retract_var(), sema is acquired
   SV_Coupling retract_var(SV_key key);

   /// see Svar_DB::retract_all(), sema is acquired
   void retract_all(const AP_num3 & id);

   /// see Svar_DB::get_coupling(), sema is acquired
   SV_Coupling get_coupling(SV_key key);

   /// see Svar_DB::get_control(), sema is acquired
   Svar_Control get_control(SV_key key) const;

   /// see Svar_DB::set_control(), sema is acquired
   Svar_Control set_control(SV_key key, Svar_Control control);

   /// see Svar_DB::get_state(), sema is acquired
   Svar_state get_state(SV_key key) const;

   /// see Svar_DB::set_state(), sema is acquired
   void set_state(SV_key key, bool used, const char * loc);

   /// see Svar_DB::get_processors(), sema is acquired
   void get_processors(int to_proc, vector<int32_t> & processors);

   /// see Svar_DB::get_variables(), sema is acquired
   void get_variables(int to_proc, int from_proc,
                      vector<const uint32_t *> & vars);

   /// see Svar_DB::add_event(), sema is acquired
   void add_event(Svar_event event, AP_num3 proc, SV_key key);

   /// see Svar_DB::clear_all_events(), sema is acquired
   Svar_event clear_all_events();

   /// see Svar_DB::clear_event(), sema is acquired
   void clear_event(SV_key key);

   /// see Svar_DB::get_events(), sema is acquired
   SV_key get_events(Svar_event & events, AP_num3 proc) const;

   /// see Svar_DB::pairing_key(), sema is acquired
   SV_key pairing_key(SV_key key, bool (*compare)
                             (const uint32_t * v1, const uint32_t * v2)) const;

   /// see Svar_DB::get_peer(), sema is acquired
   Svar_partner get_peer(SV_key key);

   /// try to match an existing offer, return a pointer to it if found and
   /// initialize the accepting side of the offer
   offered_SVAR * match_pending_offer(const uint32_t * UCS_varname,
                                      const AP_num3 & to,
                                      const Svar_partner & from);

   /// create a new offer in the DB, return a pointer to it, or 0 if the
   /// Svar_DB is full
   offered_SVAR * create_offer(const uint32_t * UCS_varname,
                               const AP_num3 & to, const Svar_partner & from);

   /// find the variable named \b varname and offered or accepted by \b proc
   offered_SVAR * find_var(SV_key key) const;

   /// semaphore for locking this memory
   sem_t sema;

   /// a sequence number helping to create keys
   uint32_t seq;

   /// the offered variables
   offered_SVAR offered_vars[MAX_SVARS_OFFERED];

   /// processors registered in this database
   Svar_partner_events active_processors[MAX_ACTIVE_PROCS];

   /// a semaphore to coordinate printouts of different processes
   sem_t print_sema;

   /// when print_sema was last acquired
   int64_t print_sema_when;
};
//-----------------------------------------------------------------------------

#define SV_LOCKED(open_act, closed_act) \
   if (the_Svar_DB.is_open()) { open_act } else { closed_act }

/// a database in shared memory that contains all shared variables on
/// this machine
class Svar_DB
{
public:
   /// open (and possibly initialize) the shared variable database
   Svar_DB();

   /// remove all offers from this process
   ~Svar_DB();

   /// match a new offer against the DB. Return: 0 on error, 1 if the
   /// new offer was inserted into the DB (sicne no match was found), or
   /// 2 if a pending offer was found. In the latter case, the pending offer
   /// was removed and its relevant data returned through the d_... args.
   static offered_SVAR * match_or_make(const uint32_t * UCS_varname,
                                       const AP_num3 & to,
                                       const Svar_partner & from,
                                       SV_Coupling & coupling)
      {
        offered_SVAR * svar;
        SV_LOCKED(svar = the_Svar_DB.DB_memory->match_or_make(UCS_varname, to,
                  from, coupling); , svar = 0; )
        return svar;
      }

   /// register processor in the database
   static void register_processor(const Svar_partner & svp)
      {
        SV_LOCKED(the_Svar_DB.DB_memory->register_processor(svp); , )
      }

   /// return the partner with id \b id
   static Svar_partner_events get_registered(const AP_num3 & id)
      {
        Svar_partner_events svp;
        SV_LOCKED(svp = the_Svar_DB.DB_memory->get_registered(id); ,
                  svp.clear();)
        return svp;
      }

   /// return true if \b id is registered
   static bool is_registered(const AP_num3 & id)
      {
        const Svar_partner_events svp = get_registered(id);
        return svp.partner.id.proc > AP_NULL;   // NO_AP or AP_NULL
      }

   /// return true if \b id is not used by any registered proc, parent, or grand
   static bool is_unused_id(AP_num id)
      {
        bool unused;
        SV_LOCKED(unused = the_Svar_DB.DB_memory->is_unused_id(id); ,
                  unused = false; )
        return unused;
      }

   /// return an id >= AP_FIRST_USER that is not mentioned in any registered
   /// processor (i,e, not proc, parent. or grand)
   static AP_num get_unused_id()
      {
        AP_num unused;
        SV_LOCKED(unused = the_Svar_DB.DB_memory->get_unused_id(); ,
           unused = NO_AP; )
        return unused;
      }

   /// return the UDP port for communication with AP \b proc
   static uint16_t get_udp_port(AP_num proc, AP_num parent)
      {
        uint16_t port = 0;
        SV_LOCKED(port = the_Svar_DB.DB_memory->get_udp_port(proc, parent); , )
        return port;
      }

   /// unregister processor in the database
   static void unregister_processor(const Svar_partner & p)
      {
        SV_LOCKED(the_Svar_DB.DB_memory->unregister_processor(p); , )
      }

   /// retract an offer, return previous coupling
   static SV_Coupling retract_var(SV_key key)
      {
        SV_Coupling ret;
        SV_LOCKED(ret = the_Svar_DB.DB_memory->retract_var(key); ,
                  ret = NO_COUPLING; )
        return ret;
      }

   /// return coupling of \b entry with \b key.
   static SV_Coupling get_coupling(SV_key key)
      {
        SV_Coupling coupling;
        SV_LOCKED(coupling = the_Svar_DB.DB_memory->get_coupling(key); ,
                  coupling = NO_COUPLING; )
        return coupling;
      }

   /// return the current control vector of this variable
   static Svar_Control get_control(SV_key key)
      {
        SV_LOCKED(return the_Svar_DB.DB_memory->get_control(key); ,
                  return NO_SVAR_CONTROL; )
      }

   /// find variable with key \b key
   static offered_SVAR * find_var(SV_key key)
      {
        SV_LOCKED(return the_Svar_DB.DB_memory->find_var(key); ,
                  return 0; )
      }

   /// set the current control vector of this variable
   static Svar_Control set_control(SV_key key, Svar_Control ctl)
      {
        SV_LOCKED(return the_Svar_DB.DB_memory->set_control(key, ctl); ,
                  return NO_SVAR_CONTROL; )
      }

   /// return the current state of this variable
   static Svar_state get_state(SV_key key)
      {
        SV_LOCKED(return the_Svar_DB.DB_memory->get_state(key); ,
                  return SVS_NOT_SHARED; )
      }

   /// set the current state of this variable
   static void set_state(SV_key key, bool used, const char * loc)
      {
        SV_LOCKED(the_Svar_DB.DB_memory->set_state(key, used, loc); , )
      }

   /// add processors with pending offers to \b processors. Duplicates
   /// are OK and will be removed later
   static void get_processors(int to_proc, vector<int32_t> & processors)
      {
        SV_LOCKED(the_Svar_DB.DB_memory->get_processors(to_proc, processors); ,)
      }

   /// return all variables shared between \b to_proc and \b from_proc
   static void get_variables(int to_proc, int from_proc,
                             vector<const uint32_t *> & vars)
      {
        SV_LOCKED(the_Svar_DB.DB_memory->get_variables(to_proc,
                                                       from_proc, vars); , )
      }

   /// return the share partner
   static Svar_partner get_peer(SV_key key)
      {
        Svar_partner peer;
        SV_LOCKED(peer = the_Svar_DB.DB_memory->get_peer(key); , peer.clear(); )
        return peer;
      }

   /// some shared variables belong to a pair, like CTL and DAT in AP210.
   /// return the key of the other variable (as determined by the compare()
   /// function procided), of 0 if the key or the other variable don't exist
   static SV_key pairing_key(SV_key key,
          bool (*compare)(const uint32_t * v1, const uint32_t * v2))
      {
        SV_LOCKED(return the_Svar_DB.DB_memory->pairing_key(key, compare); ,
                  return 0; )
      }

   /// clear all events, return the current event bitmap.
   static Svar_event clear_all_events()
      {
        SV_LOCKED(return the_Svar_DB.DB_memory->clear_all_events(); , )
        return SVE_NO_EVENTS;   // not reached; suppress -Wreturn-type warnings
      }

   /// clear the events of one variable
   static void clear_event(SV_key key)
      {
        SV_LOCKED(the_Svar_DB.DB_memory->clear_event(key); , )
      }

   /// return events for processor \b proc
   static SV_key get_events(Svar_event & events, AP_num3 proc)
      {
        SV_LOCKED(return the_Svar_DB.DB_memory->get_events(events, proc); ,
                  return 0;)
      }

   /// set an event for proc (and maybe also for key)
   static void add_event(Svar_event event, AP_num3 proc, SV_key key)
      {
        SV_LOCKED(the_Svar_DB.DB_memory->add_event(event, proc, key); , )
      }

   /// print the database
   static void print(ostream & out);

#ifdef PRINT_SEMA_WANTED
   /// acquire print_sema and hold it until end_print() is called
   static void start_print(const char * loc);

   /// release print_sema
   static void end_print(const char * loc);
#else
   /// acquire print_sema and hold it until end_print() is called
   static void start_print(const char * loc) {}

   /// release print_sema
   static void end_print(const char * loc) {}
#endif

protected:
   /// return if the shared memory was opened successfully
   bool is_open() const   { return DB_memory; }

   /// the shared memory
   Svar_DB_memory * DB_memory;

   /// the database in the shared memory
   static Svar_DB the_Svar_DB;

   /// true after start_print(), false after end_print()
   static bool print_sema_acquired;
};

#endif // __SVAR_DB_HH_DEFINED__
