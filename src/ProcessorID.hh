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

#ifndef __PROCESSOR_ID_HH_DEFINED__
#define __PROCESSOR_ID_HH_DEFINED__

#include "UdpSocket.hh"

using namespace std;

/// Auxiliary processor numbers
enum AP_num
{
  NO_AP         = -1,     ///< invalid AP
  AP_NULL       = 0,      ///< invalid AP for structs using memset(0)
  AP_GENERAL    = 0,      ///< AP for generic offers
  AP_FIRST_USER = 1001,   ///< the first AP for APL interpreters
};

/// three AP numbers that uniquely identify a processor
struct AP_num3
{
   /// constructor: invalid AP_num3
   AP_num3() {}

   /// constructor: processor, parent, and grand-parent
   AP_num3(AP_num pr, AP_num pa, AP_num gp)
   : proc(pr),
     parent(pa),
     grand(gp)
   {}

   /// ture if \b this AP_num3 is equal to \b other
   bool operator==(const AP_num3 & other) const
      { return proc   == other.proc   &&
               parent == other.parent &&
               grand  == other.grand; }

   AP_num proc;     ///< the processor
   AP_num parent;   ///< the parent of the processor
   AP_num grand;    ///< the parent of the parent
};

#ifdef AP_NUM

/// a simple ProcessorID to be used by APs
class ProcessorID
{
public:
   /// return the current ID (proc, parent, and grandparent)
   static const AP_num3 & get_id()        { return id; }

   /// set the current ID
   static void set_own_ID(AP_num ap)      { id.proc = ap; }

   /// return the parent's ID
   static AP_num get_parent_ID()          { return id.parent; }

   /// set the parent's ID
   static void set_parent_ID(AP_num ap)   { id.parent = ap; }

   /// return the grandparent's ID
   static AP_num get_grand_ID()           { return id.grand; }

   /// set the grandparent's ID
   static void set_grand_ID(AP_num ap)    { id.grand = ap; }

protected:
   /// the current ID (proc, parent, and grandparent)
   static    AP_num3 id;
};

#else

// the normal ProcessorID to be used by the APL interpreter

#include <vector>

/// a mapping between the left argument of ⎕SVO or ⎕SVQ and AP numbers
struct SvoPid
{
   SvoPid() : id(NO_AP, AP_NULL, AP_NULL) {}

   int svopid;           ///< left argument of ⎕SVO and ⎕SVQ
   int ip_addr;          ///< remote IP address
   char user[32];        ///< user account (login name)
   AP_num3 id;           ///< processor, parent and grandparent
};

/// A processor authentication
struct ProcAuth
{
   /// constructor: no ID
   ProcAuth() : id(NO_AP, AP_NULL, AP_NULL) {}

   /// the ID
   AP_num3 id;

   /// the allowed remote processors
   vector<int> rsvopid;        ///< left argument(s) of remote ⎕SVO and ⎕SVQ
};

/// A network profile
struct Network_Profile
{
   vector<SvoPid>     svo_pids;     ///< ///< left argument of ⎕SVO and ⎕SVQ
   vector<ProcAuth>   proc_auths;   ///< processor authentications

   /// clear everything
   void clear()
      {
        svo_pids.clear();
        proc_auths.clear();
      }
};

/// One processor. APL interpreters have numbers > 1000 while auxiliary
/// processors have numbers < 1000
class ProcessorID
{
public:
   /// initialize our own process ID, return non-0 on error.
   /// proc_id == 0 uses the next free ID > 1000; otherwise proc_id is used.
   /// \b do_sv defines if an APnnn process for incoming ⎕SVO offers
   /// shall be forked.
   static bool init(bool do_sv, int proc_id, int par_id);

   /// return the own id, parent, and grand-parent
   static const AP_num3 & get_id()        { return id; }

   /// return the processor ID of this apl interpreter
   static AP_num get_own_ID()   { return id.proc; }

   /// return the processor ID of the parent of this apl interpreter
   static AP_num get_parent_ID()   { return id.parent; }

   /// return the port number for communication with this AP
   static uint16_t get_APnnn_port()   { return APnnn_port; }

   /// return the local socket communication with this AP
   static UdpSocket & get_own_socket()   { return APnnn_socket; }

   /// read the network profile file from its default location
   static int read_network_profile();

   /// disconnect from APnnn process
   static void disconnect();

protected:
   /// read the network profile file from file \b file
   static int read_network_profile(const char * filename);

   /// read one SvoPid entry from \b file
   static const char * read_svopid(FILE * file, SvoPid & svopid, int & line);

   /// read one ProcAuth entry from \b file
   static const char * read_procauth(FILE * file, ProcAuth & procauth,
                                     int & line);

   /// the processor, parent, and grandparent of this apl interpreter
   static AP_num3 id;

   /// the UDP port of APnnn
   static uint16_t  APnnn_port;

   /// the network profile currently used
   static Network_Profile network_profile;

   /// socket towards APnnn process
   static UdpSocket APnnn_socket;

   /// true if shared variables are enabled
   static bool doing_SV;
};

#endif

#endif // __PROCESSOR_ID_HH_DEFINED__
