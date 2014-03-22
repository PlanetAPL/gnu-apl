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

/*  Copyright (C) 2012 Dr. Juergen Sauermann
    This file is part of udp_signal.

    udp_signal is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    udp_signal is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with udp_signal.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
    this file defines classes UdpClientSocket and UdpServerSocket which
    are used to send signals from one process to another process.
 */

#ifndef __UDP_SOCKET_HH_DEFINE__
#define __UDP_SOCKET_HH_DEFINE__

#include <iostream>
#include <stdint.h>
#include <string>
#include <unistd.h>

//----------------------------------------------------------------------------
/// a UDP socket for sending and receiving signals
class UdpSocket
{
public:
   /// constructor: client or server socket
   UdpSocket(bool _is_server, const char * _loc);

   /// constructor: socket with given port numbers and IP addresses
   UdpSocket(uint16_t _local_port, int _remote_port,
             uint32_t _local_ip,   uint32_t _remote_ip,
             bool _is_server, const char * _loc);

   /// destructor
   ~UdpSocket()   { udp_close(); }

   /// close the socket (if open)
   void udp_close()
      { if (udp_socket != -1)   close(udp_socket);   udp_socket = -1; }

   /// prevent closing of \b udp_socket (e.g. when destructed after fork())
   void forked()   { udp_socket = -1; }

   /// return the local (own) UDP port number
   int get_local_port() const   { return local_port; }

   /// return the remote (peer's) UDP port number
   int get_remote_port() const   { return remote_port; }

   /// return true if this socket is valid (usable)
   bool is_valid() const   { return udp_socket != -1; }

   /// enable debug printouts on \b out
   void set_debug(std::ostream & out)   { debug = &out; }

   /// return output channel for debug printouts, or NULL
   std::ostream * get_debug() const   { return debug; }

   /// send buffer to peer
   int send(const std::string & buffer) const;

   /// receive buffer from peer
   int recv(void * buffer, int bufsize, uint16_t & from_port,
            uint32_t & from_ip, uint32_t timeout_ms);

   /// IP address of localhost
   enum { IP_LOCALHOST = 0x7F000001 };   /// 127.0.0.1

   /// print IPv4 address and port.
   static void print_addr(std::ostream & out, uint32_t ip, int port);

protected:
   /// copy from other
   UdpSocket & operator=(const UdpSocket & other)
      { memcpy(this, &other, sizeof(*this));   return *this; }

   /// where the socket was created
   const char * loc;

   /// the local port number
   int local_port;

   /// the peer's port number
   int remote_port;

   /// the local IP address
   uint32_t local_ip;

   /// the peer's IP address
   uint32_t remote_ip;

   /// true for a server socket, false for a client socket
   const bool is_server;

   /// the file descriptor
   int udp_socket;

   /// debug channel or NULL
   std::ostream * debug;
};
//----------------------------------------------------------------------------
/// a client socket
class UdpClientSocket : public UdpSocket
{
public:
   /// an invalid client socket
   UdpClientSocket(const char * loc)
   : UdpSocket(-1, -1, -1, -1, false, loc)
   {}

   /// a client socket towards the remote \b server_port
   UdpClientSocket(const char * loc, int server_port, int client_port = 0,
                   uint32_t server_ip = IP_LOCALHOST,
                   uint32_t client_ip = IP_LOCALHOST)
   : UdpSocket(client_port, server_port, client_ip, server_ip, false, loc)
   {}
};
//----------------------------------------------------------------------------
/// a server socket
class UdpServerSocket : public UdpSocket
{
public:
   /// a server socket receiving client signals on port \b server_port
   UdpServerSocket(const char * loc, uint16_t server_port,
                   uint32_t server_ip = IP_LOCALHOST)
   : UdpSocket(server_port, 0, server_ip, 0, true, loc)
   {}

   /// set the client's port number
   void set_remote_port(int port)   { remote_port = port; }
};
//----------------------------------------------------------------------------

#endif // __UDP_SOCKET_HH_DEFINE__
