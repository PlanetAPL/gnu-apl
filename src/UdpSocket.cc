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
    this file implements classes UdpClientSocket and UdpServerSocket which
    are used to send signals from one process to another process.
 */

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <string>
#include <iostream>
#include <iomanip>

#include "UdpSocket.hh"

using namespace std;

//----------------------------------------------------------------------------
UdpSocket::UdpSocket(bool _is_server, const char * _loc)
  : loc(_loc),
    local_port(-1),
    remote_port(-1),
    local_ip(-1),
    remote_ip(-1),
    is_server(_is_server),
    udp_socket(-1),
    debug(0)
{
}
//----------------------------------------------------------------------------
UdpSocket::UdpSocket(uint16_t _local_port, int _remote_port,
             uint32_t _local_ip,  uint32_t _remote_ip,
             bool _is_server, const char * _loc)
  : loc(_loc),
    local_port(_local_port),
    remote_port(_remote_port),
    local_ip(_local_ip),
    remote_ip(_remote_ip),
    is_server(_is_server),
    udp_socket(-1),
    debug(0)
{
   if (remote_port == -1)   return;

   udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
   assert(udp_socket != -1);

sockaddr_in local;
   memset(&local, 0, sizeof(sockaddr_in));
   local.sin_family = AF_INET;
   local.sin_port = htons(local_port);
   local.sin_addr.s_addr = htonl(local_ip);

   if (::bind(udp_socket, (const sockaddr *)&local, sizeof(sockaddr_in)))
      {
        cerr << "*** UdpSocket::UdpSocket() : bind("
             << local_port << ") failed:" << strerror(errno) << endl;
        assert(0);
      }

   // refresh the local port and IP, since bind() may have set them.
   //
   socklen_t size = sizeof(sockaddr_in);
   getsockname(udp_socket, (sockaddr *)&local, &size);
   local_port = ntohs(local.sin_port);
   local_ip = ntohl(local.sin_addr.s_addr);
}
//----------------------------------------------------------------------------
int
UdpSocket::send(const std::string & buffer) const
{
sockaddr_in remote;
   memset(&remote, 0, sizeof(sockaddr_in));
   remote.sin_family = AF_INET;
   remote.sin_port = htons(remote_port);
   remote.sin_addr.s_addr = htonl(remote_ip);

   for (;;)
       {
         errno = 0;
         const int sent = sendto(udp_socket, buffer.c_str(), buffer.size(), 0,
                                  (const sockaddr *)&remote, sizeof(remote));

         if (sent != buffer.size())   // something went wrong
            {
              if (errno == EINTR)   continue;   // signal received

              cerr << "UdpSocket::sendto() failed: " << strerror(errno) << endl
                   << "socket was created at: " << loc << endl
                   << "local address:  ";
              print_addr(cerr,  local_ip, local_port);
              cerr << "remote address: " << endl;
              print_addr(cerr,  remote_ip, remote_port);
              cerr << endl;

              return -errno;
            }

            break;
       }

   return 0;
}
//----------------------------------------------------------------------------
int
UdpSocket::recv(void * buffer, int bufsize, uint16_t & from_port,
                uint32_t & from_ip, uint32_t timeout_ms)
{
sockaddr_in remote;
socklen_t remote_len = sizeof(remote);

   if (timeout_ms)   // abort recv after timeout_ms
      {
        for (;;)
            {
              fd_set read_fds;
              FD_ZERO(&read_fds);
              FD_SET(udp_socket, &read_fds);
              const int tv_sec  = timeout_ms/1000;
              const int tv_usec = 1000*(timeout_ms%1000);
              timeval tv = { tv_sec, tv_usec };

              errno = 0;
              const int cnt = select(udp_socket + 1, &read_fds, 0, 0, &tv);
              if (cnt == 0)
                 {
                   errno = ETIMEDOUT;
                   return 0;
                 }
              else if (cnt < 0)
                 {
                   if (errno == EINTR)   continue;   // signal received

                   cerr << "UdpSocket::recv() failed: select() returned "
                        << strerror(errno) << endl;
                   return 0;
                 }

              break;   // success: proceed with recv
            }
      }

int len = -1;

   for (;;)
       {
         errno = 0;
         len = recvfrom(udp_socket, buffer, bufsize, 0,
                         (sockaddr *)&remote, &remote_len);

         if (len <= 0)
            {
              if (errno == EINTR)   continue;   // signal received

              cerr << "UdpSocket::recv() failed: " << strerror(errno) << endl;
              return 0;
            }

         break;
       }

   from_port = ntohs(remote.sin_port);
   from_ip = ntohl(remote.sin_addr.s_addr);

   if (is_server)
      {
        remote_port = from_port;
        remote_ip = from_ip;
      }

   return len;
}
//----------------------------------------------------------------------------
void
UdpSocket::print_addr(std::ostream & out, uint32_t ip, int port)
{
   out << (ip >> 24 & 0xFF) << "."
       << (ip >> 16 & 0xFF) << "."
       << (ip >>  8 & 0xFF) << "."
       << (ip       & 0xFF) << ":" << port;
}
//----------------------------------------------------------------------------

