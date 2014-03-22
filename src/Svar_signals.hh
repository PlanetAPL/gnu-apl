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

/********

Usage:

    m4 -D Svar_signals=def_file < udp_signal.m4 > $@

This reads a Svar_signals specification from file def_file.def and prints
a header file on stdout that defines (the serialization of) signals according
to def_file.def.

In a Makefile, you would use it, for example, like this:

    my_signal.hh:  udp_signal.m4 my_signal.def
            m4 -D Svar_signals=udp_signal $< > $@

to produce my_signal.hh from my_signal.def. After that, my_signal.hh
can be used together with UdpSocket.cc and UdpSocket.hh in order to send
the signals defined in udp_signal.def from one process to another process.

That is:

                      my_signal.def
                            |
                            |
                            | udp_signal.m4
                            |
                            V
                      my_signal.hh  UdpSocket.hh
                            |            |
                            |            |
                            |  #include  |
                            |            |
                            V            V
                         my_client_program.cc
                         my_server_program.cc  UdpSocket.cc
                                   |               |
                                   |               |
                                   |    compile    |
                                   |    & link     |
                                   V               V
                                   my_client_program
                                   my_server_program


and then:

    my_client_program  ---  signals defined in ---> my_server_program
    my_server_program  ---  in my_signal.def   ---> my_client_program

********/

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <string>
#include <iostream>
#include <iomanip>

#include "UdpSocket.hh"

using namespace std;

//-----------------------------------------------------------------------------
/// an integer signal item of size \b bytes
template<typename T, int bytes>
class Sig_item_int
{
public:
   /// construct an item with value \b v
   Sig_item_int(T v) : value(v & get_mask()) {}

   /// construct (deserialize) this item from a (received) buffer
   Sig_item_int(const uint8_t * & buffer)
      {
        value = 0;
        for (int b = 0; b < bytes; ++b)
            value = value << 8 | *buffer++;

        if (bytes == 6 && (value & 0x000080000000000ULL))
           value |= 0xFFFF000000000000ULL;
      }

   /// return the value of the item
   T get_value() const   { return value; }

   /// store (aka. serialize) this item into a string
   void store(string & buffer) const
      {
        for (int b = bytes; b > 0;)   buffer += char(value >> (8*--b));
      }

   /// print the item
   ostream & print(ostream & out) const
      {
        return out << value;
      }

   /// return the mask of the item, e.g. 0x00FFFFFF for a 3-byte integer
   static uint64_t get_mask()
      { uint64_t mask = 0;
        for (int b = 0; b < bytes; ++b)   mask = mask << 8 | 0x00FF;
        return mask;
      }

   /// the value of the item
   T value;
};
//-----------------------------------------------------------------------------
/// a hexadecimal integer signal item of size \b bytes
template<typename T, int bytes>
class Sig_item_xint : public Sig_item_int<T, bytes>
{
public:
   /// construct an item with value \b v
   Sig_item_xint(T v) : Sig_item_int<T, bytes>(v) {}

   /// construct (deserialize) this item from a (received) buffer
   Sig_item_xint(const uint8_t * & buffer) : Sig_item_int<T, bytes>(buffer) {}

   /// print the item
   ostream & print(ostream & out) const
      {
        return out << "0x" << hex << setfill('0') << setw(bytes)
                   << Sig_item_int<T, bytes>::value
                   << setfill(' ') << dec;
      }
};
//-----------------------------------------------------------------------------
typedef Sig_item_int < int16_t, 1> Sig_item_i8;   ///<   8-bit signed integer
typedef Sig_item_int <uint16_t, 1> Sig_item_u8;   ///<   8-bit unsigned integer
typedef Sig_item_xint<uint16_t, 1> Sig_item_x8;   ///<   8-bit hex integer
typedef Sig_item_int < int16_t, 2> Sig_item_i16;   ///< 16-bit signed integer
typedef Sig_item_int <uint16_t, 2> Sig_item_u16;   ///< 16-bit unsigned integer
typedef Sig_item_xint<uint16_t, 2> Sig_item_x16;   ///< 16-bit hex integer
typedef Sig_item_int < int32_t, 3> Sig_item_i24;   ///< 24-bit signed integer
typedef Sig_item_int <uint32_t, 3> Sig_item_u24;   ///< 24-bit unsigned integer
typedef Sig_item_xint<uint32_t, 3> Sig_item_x24;   ///< 24-bit hex integer
typedef Sig_item_int < int32_t, 4> Sig_item_i32;   ///< 32-bit signed integer
typedef Sig_item_int <uint32_t, 4> Sig_item_u32;   ///< 32-bit unsigned integer
typedef Sig_item_xint<uint32_t, 4> Sig_item_x32;   ///< 32-bit hex integer
typedef Sig_item_int < int64_t, 6> Sig_item_i48;   ///< 48-bit signed integer
typedef Sig_item_int <uint64_t, 6> Sig_item_u48;   ///< 48-bit unsigned integer
typedef Sig_item_xint<uint64_t, 6> Sig_item_x48;   ///< 48-bit hex integer
typedef Sig_item_int < int64_t, 8> Sig_item_i64;   ///< 64-bit signed integer
typedef Sig_item_int <uint64_t, 8> Sig_item_u64;   ///< 64-bit unsigned integer
typedef Sig_item_xint<uint64_t, 8> Sig_item_x64;   ///< 64-bit hex integer

/// a string signal item of size \b bytes
class Sig_item_string
{
public:
   /// construct an item with value \b v
   Sig_item_string(const string & str)
   : value(str)
   {}

   /// construct (deserialize) this item from a (received) buffer
   Sig_item_string(const uint8_t * & buffer)
      {
        Sig_item_u16 len (buffer);
        value.reserve(len.get_value() + 2);
        for (int b = 0; b < len.get_value(); ++b)   value += *buffer++;
      }

   /// return the value of the item
   const string get_value() const   { return value; }

   /// store (aka. serialize) this item into a buffer
   void store(string & buffer) const
      {
        const Sig_item_u16 len (value.size());
        len.store(buffer);
        for (int b = 0; b < value.size(); ++b)   buffer += value[b];
      }

   /// print the item
   ostream & print(ostream & out) const
      {
        return out << "\"" << value << "\"";
      }

protected:
   /// the value of the item
   string value;
};
//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/// a number identifying the signal
enum Signal_id
{
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


/// )OFF
   sid_DISCONNECT,

/// ⎕SVO, ⎕SVR
   sid_NEW_VARIABLE,
   sid_MAKE_OFFER,
   sid_OFFER_MATCHED,
   sid_RETRACT_OFFER,

/// SVAR←X and X←SVAR
   sid_GET_VALUE,
   sid_VALUE_IS,
   sid_ASSIGN_VALUE,
   sid_ASSIGNED,

/// ⎕SVE
   sid_START_EVENT_REPORTING,
   sid_STOP_EVENT_REPORTING,
   sid_GOT_EVENT,
   sid_NEW_EVENT,

   sid_MAX,
};
//----------------------------------------------------------------------------
/// the base class for all signal classes
class Signal_base
{
public:
   /// store (encode) the signal into buffer
   virtual void store(string & buffer) const = 0;

   /// print the signal
   virtual ostream & print(ostream & out) const = 0;

   /// return the ID of the signal
   virtual Signal_id get_sigID() const = 0;

   /// return the name of the ID of the signal
   virtual const char * get_sigName() const = 0;

   /// return the max. size of a signal
   inline static size_t get_class_size();

   /// get function for an item that is not defined for the signal
   void bad_get(const char * signal, const char * member) const
      {
        cerr << endl << "*** called function get_" << signal << "__" << member
             << "() with wrong signal " << get_sigName() << endl;
        assert(0 && "bad_get()");
      }

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


/// )OFF
   /// access functions for signal DISCONNECT...


/// ⎕SVO, ⎕SVR
   /// access functions for signal NEW_VARIABLE...
   virtual uint64_t get__NEW_VARIABLE__key() const   ///< dito
      { bad_get("NEW_VARIABLE", "key");   return 0; }

   /// access functions for signal MAKE_OFFER...
   virtual uint64_t get__MAKE_OFFER__key() const   ///< dito
      { bad_get("MAKE_OFFER", "key");   return 0; }

   /// access functions for signal OFFER_MATCHED...
   virtual uint64_t get__OFFER_MATCHED__key() const   ///< dito
      { bad_get("OFFER_MATCHED", "key");   return 0; }

   /// access functions for signal RETRACT_OFFER...
   virtual uint64_t get__RETRACT_OFFER__key() const   ///< dito
      { bad_get("RETRACT_OFFER", "key");   return 0; }


/// SVAR←X and X←SVAR
   /// access functions for signal GET_VALUE...
   virtual uint64_t get__GET_VALUE__key() const   ///< dito
      { bad_get("GET_VALUE", "key");   return 0; }

   /// access functions for signal VALUE_IS...
   virtual uint64_t get__VALUE_IS__key() const   ///< dito
      { bad_get("VALUE_IS", "key");   return 0; }
   virtual uint32_t get__VALUE_IS__error() const   ///< dito
      { bad_get("VALUE_IS", "error");   return 0; }
   virtual string get__VALUE_IS__error_loc() const   ///< dito
      { bad_get("VALUE_IS", "error_loc");   return string(); }
   virtual string get__VALUE_IS__value() const   ///< dito
      { bad_get("VALUE_IS", "value");   return string(); }

   /// access functions for signal ASSIGN_VALUE...
   virtual uint64_t get__ASSIGN_VALUE__key() const   ///< dito
      { bad_get("ASSIGN_VALUE", "key");   return 0; }
   virtual string get__ASSIGN_VALUE__value() const   ///< dito
      { bad_get("ASSIGN_VALUE", "value");  return string(); }

   /// access functions for signal ASSIGNED...
   virtual uint64_t get__ASSIGNED__key() const   ///< dito
      { bad_get("ASSIGNED", "key");   return 0; }
   virtual uint32_t get__ASSIGNED__error() const   ///< dito
      { bad_get("ASSIGNED", "error");   return 0; }
   virtual string get__ASSIGNED__error_loc() const   ///< dito
      { bad_get("ASSIGNED", "error_loc");   return string();; }


/// ⎕SVE
   /// access functions for signal START_EVENT_REPORTING...
   virtual uint16_t get__START_EVENT_REPORTING__event_port() const   ///< dito
      { bad_get("START_EVENT_REPORTING", "event_port");   return 0; }

   /// access functions for signal STOP_EVENT_REPORTING...

   /// access functions for signal GOT_EVENT...
   virtual uint64_t get__GOT_EVENT__key() const   ///< dito
      { bad_get("GOT_EVENT", "key");   return 0; }
   virtual uint32_t get__GOT_EVENT__event() const   ///< dito
      { bad_get("GOT_EVENT", "event");   return 0; }

   /// access functions for signal NEW_EVENT...
   virtual uint64_t get__NEW_EVENT__key() const   ///< dito
      { bad_get("NEW_EVENT", "key");   return 0; }
   virtual uint32_t get__NEW_EVENT__event() const   ///< dito
      { bad_get("NEW_EVENT", "event");   return 0; }



   /// receive a signal
   inline static Signal_base * recv(UdpSocket & sock, void * class_buffer,
                                    uint16_t & from_port, uint32_t & from_ip,
                                    uint32_t timeout_ms = 0);

   /// receive a signal, ignore sender's IP and port
   static Signal_base * recv(UdpSocket & sock, void * class_buffer,
                             uint32_t timeout_ms = 0)
      {
         uint16_t from_port = 0;
         uint32_t from_ip = 0;
         return recv(sock, class_buffer, from_port, from_ip, timeout_ms);
      }

protected:
   /// send this signal on sock
   void send(const UdpSocket & sock) const
       {
         string buffer;
         store(buffer);
         sock.send(buffer);
         if (ostream * out = sock.get_debug())   print(*out << "--> ");
       }
};

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


/// )OFF
//----------------------------------------------------------------------------
/// a class for DISCONNECT
class DISCONNECT_c : public Signal_base
{
public:
   /// contructor that creates the signal and sends it on socket ctx
   DISCONNECT_c(const UdpSocket & ctx)
   { send(ctx); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   DISCONNECT_c(const uint8_t * & buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 id(sid_DISCONNECT);
         id.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "DISCONNECT(";

        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_DISCONNECT; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "DISCONNECT"; }


protected:
};

/// ⎕SVO, ⎕SVR
//----------------------------------------------------------------------------
/// a class for NEW_VARIABLE
class NEW_VARIABLE_c : public Signal_base
{
public:
   /// contructor that creates the signal and sends it on socket ctx
   NEW_VARIABLE_c(const UdpSocket & ctx,
                Sig_item_x64 _key)
   : key(_key)
   { send(ctx); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   NEW_VARIABLE_c(const uint8_t * & buffer)
   : key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 id(sid_NEW_VARIABLE);
         id.store(buffer);
        key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "NEW_VARIABLE(";
        key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_NEW_VARIABLE; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "NEW_VARIABLE"; }

  /// return item key of this signal 
   virtual uint64_t get__NEW_VARIABLE__key() const { return key.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
};
//----------------------------------------------------------------------------
/// a class for MAKE_OFFER
class MAKE_OFFER_c : public Signal_base
{
public:
   /// contructor that creates the signal and sends it on socket ctx
   MAKE_OFFER_c(const UdpSocket & ctx,
                Sig_item_x64 _key)
   : key(_key)
   { send(ctx); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   MAKE_OFFER_c(const uint8_t * & buffer)
   : key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 id(sid_MAKE_OFFER);
         id.store(buffer);
        key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "MAKE_OFFER(";
        key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_MAKE_OFFER; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "MAKE_OFFER"; }

  /// return item key of this signal 
   virtual uint64_t get__MAKE_OFFER__key() const { return key.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
};
//----------------------------------------------------------------------------
/// a class for OFFER_MATCHED
class OFFER_MATCHED_c : public Signal_base
{
public:
   /// contructor that creates the signal and sends it on socket ctx
   OFFER_MATCHED_c(const UdpSocket & ctx,
                Sig_item_x64 _key)
   : key(_key)
   { send(ctx); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   OFFER_MATCHED_c(const uint8_t * & buffer)
   : key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 id(sid_OFFER_MATCHED);
         id.store(buffer);
        key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "OFFER_MATCHED(";
        key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_OFFER_MATCHED; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "OFFER_MATCHED"; }

  /// return item key of this signal 
   virtual uint64_t get__OFFER_MATCHED__key() const { return key.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
};
//----------------------------------------------------------------------------
/// a class for RETRACT_OFFER
class RETRACT_OFFER_c : public Signal_base
{
public:
   /// contructor that creates the signal and sends it on socket ctx
   RETRACT_OFFER_c(const UdpSocket & ctx,
                Sig_item_x64 _key)
   : key(_key)
   { send(ctx); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   RETRACT_OFFER_c(const uint8_t * & buffer)
   : key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 id(sid_RETRACT_OFFER);
         id.store(buffer);
        key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "RETRACT_OFFER(";
        key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_RETRACT_OFFER; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "RETRACT_OFFER"; }

  /// return item key of this signal 
   virtual uint64_t get__RETRACT_OFFER__key() const { return key.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
};

/// SVAR←X and X←SVAR
//----------------------------------------------------------------------------
/// a class for GET_VALUE
class GET_VALUE_c : public Signal_base
{
public:
   /// contructor that creates the signal and sends it on socket ctx
   GET_VALUE_c(const UdpSocket & ctx,
                Sig_item_x64 _key)
   : key(_key)
   { send(ctx); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   GET_VALUE_c(const uint8_t * & buffer)
   : key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 id(sid_GET_VALUE);
         id.store(buffer);
        key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "GET_VALUE(";
        key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_GET_VALUE; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "GET_VALUE"; }

  /// return item key of this signal 
   virtual uint64_t get__GET_VALUE__key() const { return key.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
};
//----------------------------------------------------------------------------
/// a class for VALUE_IS
class VALUE_IS_c : public Signal_base
{
public:
   /// contructor that creates the signal and sends it on socket ctx
   VALUE_IS_c(const UdpSocket & ctx,
                Sig_item_x64 _key,
                Sig_item_u32 _error,
                Sig_item_string _error_loc,
                Sig_item_string _value)
   : key(_key),
     error(_error),
     error_loc(_error_loc),
     value(_value)
   { send(ctx); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   VALUE_IS_c(const uint8_t * & buffer)
   : key(buffer),
     error(buffer),
     error_loc(buffer),
     value(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 id(sid_VALUE_IS);
         id.store(buffer);
        key.store(buffer);
        error.store(buffer);
        error_loc.store(buffer);
        value.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "VALUE_IS(";
        key.print(out);   out << ", ";
        error.print(out);   out << ", ";
        error_loc.print(out);   out << ", ";
        value.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_VALUE_IS; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "VALUE_IS"; }

  /// return item key of this signal 
   virtual uint64_t get__VALUE_IS__key() const { return key.get_value(); }

  /// return item error of this signal 
   virtual uint32_t get__VALUE_IS__error() const { return error.get_value(); }

  /// return item error_loc of this signal 
   virtual string get__VALUE_IS__error_loc() const { return error_loc.get_value(); }

  /// return item value of this signal 
   virtual string get__VALUE_IS__value() const { return value.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_u32 error;   ///< error
   Sig_item_string error_loc;   ///< error_loc
   Sig_item_string value;   ///< value
};
//----------------------------------------------------------------------------
/// a class for ASSIGN_VALUE
class ASSIGN_VALUE_c : public Signal_base
{
public:
   /// contructor that creates the signal and sends it on socket ctx
   ASSIGN_VALUE_c(const UdpSocket & ctx,
                Sig_item_x64 _key,
                Sig_item_string _value)
   : key(_key),
     value(_value)
   { send(ctx); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   ASSIGN_VALUE_c(const uint8_t * & buffer)
   : key(buffer),
     value(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 id(sid_ASSIGN_VALUE);
         id.store(buffer);
        key.store(buffer);
        value.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "ASSIGN_VALUE(";
        key.print(out);   out << ", ";
        value.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_ASSIGN_VALUE; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "ASSIGN_VALUE"; }

  /// return item key of this signal 
   virtual uint64_t get__ASSIGN_VALUE__key() const { return key.get_value(); }

  /// return item value of this signal 
   virtual string get__ASSIGN_VALUE__value() const { return value.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_string value;   ///< value
};
//----------------------------------------------------------------------------
/// a class for ASSIGNED
class ASSIGNED_c : public Signal_base
{
public:
   /// contructor that creates the signal and sends it on socket ctx
   ASSIGNED_c(const UdpSocket & ctx,
                Sig_item_x64 _key,
                Sig_item_u32 _error,
                Sig_item_string _error_loc)
   : key(_key),
     error(_error),
     error_loc(_error_loc)
   { send(ctx); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   ASSIGNED_c(const uint8_t * & buffer)
   : key(buffer),
     error(buffer),
     error_loc(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 id(sid_ASSIGNED);
         id.store(buffer);
        key.store(buffer);
        error.store(buffer);
        error_loc.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "ASSIGNED(";
        key.print(out);   out << ", ";
        error.print(out);   out << ", ";
        error_loc.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_ASSIGNED; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "ASSIGNED"; }

  /// return item key of this signal 
   virtual uint64_t get__ASSIGNED__key() const { return key.get_value(); }

  /// return item error of this signal 
   virtual uint32_t get__ASSIGNED__error() const { return error.get_value(); }

  /// return item error_loc of this signal 
   virtual string get__ASSIGNED__error_loc() const { return error_loc.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_u32 error;   ///< error
   Sig_item_string error_loc;   ///< error_loc
};

/// ⎕SVE
//----------------------------------------------------------------------------
/// a class for START_EVENT_REPORTING
class START_EVENT_REPORTING_c : public Signal_base
{
public:
   /// contructor that creates the signal and sends it on socket ctx
   START_EVENT_REPORTING_c(const UdpSocket & ctx,
                Sig_item_u16 _event_port)
   : event_port(_event_port)
   { send(ctx); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   START_EVENT_REPORTING_c(const uint8_t * & buffer)
   : event_port(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 id(sid_START_EVENT_REPORTING);
         id.store(buffer);
        event_port.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "START_EVENT_REPORTING(";
        event_port.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_START_EVENT_REPORTING; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "START_EVENT_REPORTING"; }

  /// return item event_port of this signal 
   virtual uint16_t get__START_EVENT_REPORTING__event_port() const { return event_port.get_value(); }


protected:
   Sig_item_u16 event_port;   ///< event_port
};
//----------------------------------------------------------------------------
/// a class for STOP_EVENT_REPORTING
class STOP_EVENT_REPORTING_c : public Signal_base
{
public:
   /// contructor that creates the signal and sends it on socket ctx
   STOP_EVENT_REPORTING_c(const UdpSocket & ctx)
   { send(ctx); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   STOP_EVENT_REPORTING_c(const uint8_t * & buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 id(sid_STOP_EVENT_REPORTING);
         id.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "STOP_EVENT_REPORTING(";

        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_STOP_EVENT_REPORTING; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "STOP_EVENT_REPORTING"; }


protected:
};
//----------------------------------------------------------------------------
/// a class for GOT_EVENT
class GOT_EVENT_c : public Signal_base
{
public:
   /// contructor that creates the signal and sends it on socket ctx
   GOT_EVENT_c(const UdpSocket & ctx,
                Sig_item_x64 _key,
                Sig_item_u32 _event)
   : key(_key),
     event(_event)
   { send(ctx); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   GOT_EVENT_c(const uint8_t * & buffer)
   : key(buffer),
     event(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 id(sid_GOT_EVENT);
         id.store(buffer);
        key.store(buffer);
        event.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "GOT_EVENT(";
        key.print(out);   out << ", ";
        event.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_GOT_EVENT; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "GOT_EVENT"; }

  /// return item key of this signal 
   virtual uint64_t get__GOT_EVENT__key() const { return key.get_value(); }

  /// return item event of this signal 
   virtual uint32_t get__GOT_EVENT__event() const { return event.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_u32 event;   ///< event
};
//----------------------------------------------------------------------------
/// a class for NEW_EVENT
class NEW_EVENT_c : public Signal_base
{
public:
   /// contructor that creates the signal and sends it on socket ctx
   NEW_EVENT_c(const UdpSocket & ctx,
                Sig_item_x64 _key,
                Sig_item_u32 _event)
   : key(_key),
     event(_event)
   { send(ctx); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   NEW_EVENT_c(const uint8_t * & buffer)
   : key(buffer),
     event(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 id(sid_NEW_EVENT);
         id.store(buffer);
        key.store(buffer);
        event.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "NEW_EVENT(";
        key.print(out);   out << ", ";
        event.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_NEW_EVENT; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "NEW_EVENT"; }

  /// return item key of this signal 
   virtual uint64_t get__NEW_EVENT__key() const { return key.get_value(); }

  /// return item event of this signal 
   virtual uint32_t get__NEW_EVENT__event() const { return event.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_u32 event;   ///< event
};

//----------------------------------------------------------------------------
size_t
Signal_base::get_class_size()
{
   // a union big enough for all signal classes
   struct _all_classes_
      {

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


/// )OFF
        char u_DISCONNECT[sizeof(DISCONNECT_c)];

/// ⎕SVO, ⎕SVR
        char u_NEW_VARIABLE[sizeof(NEW_VARIABLE_c)];
        char u_MAKE_OFFER[sizeof(MAKE_OFFER_c)];
        char u_OFFER_MATCHED[sizeof(OFFER_MATCHED_c)];
        char u_RETRACT_OFFER[sizeof(RETRACT_OFFER_c)];

/// SVAR←X and X←SVAR
        char u_GET_VALUE[sizeof(GET_VALUE_c)];
        char u_VALUE_IS[sizeof(VALUE_IS_c)];
        char u_ASSIGN_VALUE[sizeof(ASSIGN_VALUE_c)];
        char u_ASSIGNED[sizeof(ASSIGNED_c)];

/// ⎕SVE
        char u_START_EVENT_REPORTING[sizeof(START_EVENT_REPORTING_c)];
        char u_STOP_EVENT_REPORTING[sizeof(STOP_EVENT_REPORTING_c)];
        char u_GOT_EVENT[sizeof(GOT_EVENT_c)];
        char u_NEW_EVENT[sizeof(NEW_EVENT_c)];

      };

   return sizeof(_all_classes_);
}
//----------------------------------------------------------------------------
Signal_base *
Signal_base::recv(UdpSocket & sock, void * class_buffer,
                  uint16_t & from_port, uint32_t & from_ip,
                  uint32_t timeout_ms)
{
uint8_t buffer[2000];   // can be larger than *class_buffer due to strings!
size_t len = sock.recv(buffer, sizeof(buffer), from_port, from_ip, timeout_ms);
   if (len <= 0)   return 0;

const uint8_t * b = buffer;
Sig_item_u16 id(b);

Signal_base * ret = 0;
   switch(id.get_value())
      {

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


/// )OFF
        case sid_DISCONNECT: ret = new (class_buffer) DISCONNECT_c(b);   break;

/// ⎕SVO, ⎕SVR
        case sid_NEW_VARIABLE: ret = new (class_buffer) NEW_VARIABLE_c(b);   break;
        case sid_MAKE_OFFER: ret = new (class_buffer) MAKE_OFFER_c(b);   break;
        case sid_OFFER_MATCHED: ret = new (class_buffer) OFFER_MATCHED_c(b);   break;
        case sid_RETRACT_OFFER: ret = new (class_buffer) RETRACT_OFFER_c(b);   break;

/// SVAR←X and X←SVAR
        case sid_GET_VALUE: ret = new (class_buffer) GET_VALUE_c(b);   break;
        case sid_VALUE_IS: ret = new (class_buffer) VALUE_IS_c(b);   break;
        case sid_ASSIGN_VALUE: ret = new (class_buffer) ASSIGN_VALUE_c(b);   break;
        case sid_ASSIGNED: ret = new (class_buffer) ASSIGNED_c(b);   break;

/// ⎕SVE
        case sid_START_EVENT_REPORTING: ret = new (class_buffer) START_EVENT_REPORTING_c(b);   break;
        case sid_STOP_EVENT_REPORTING: ret = new (class_buffer) STOP_EVENT_REPORTING_c(b);   break;
        case sid_GOT_EVENT: ret = new (class_buffer) GOT_EVENT_c(b);   break;
        case sid_NEW_EVENT: ret = new (class_buffer) NEW_EVENT_c(b);   break;

        default: cerr << "UdpSocket::recv() failed: unknown id "
                      << id.get_value() << endl;
                 errno = EINVAL;
                 return 0;
      }

   if (ostream * out = sock.get_debug())   ret->print(*out << "<-- ");

   return ret;   // invalid id
}
//----------------------------------------------------------------------------
