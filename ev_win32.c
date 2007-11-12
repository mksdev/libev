/*
 * libev win32 compatibility cruft
 *
 * Copyright (c) 2007 Marc Alexander Lehmann <libev@schmorp.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef _WIN32

#include <sys/timeb.h>

/* note: the comment below could not be substantiated, but what would I care */
/* MSDN says this is required to handle SIGFPE */
volatile double SIGFPE_REQ = 0.0f; 

/* oh, the humanity! */
static int
ev_pipe (int filedes [2])
{
  struct sockaddr_in addr = { 0 };
  int addr_size = sizeof (addr);
  SOCKET listener;
  SOCKET sock [2] = { -1, -1 };

  if ((listener = socket (AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) 
    return -1;

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
  addr.sin_port = 0;

  if (bind (listener, (struct sockaddr *)&addr, addr_size))
    goto fail;

  if (getsockname(listener, (struct sockaddr *)&addr, &addr_size))
    goto fail;

  if (listen (listener, 1))
    goto fail;

  if ((sock [0] = socket (AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) 
    goto fail;

  if (connect (sock[0], (struct sockaddr *)&addr, addr_size))
    goto fail;

  if ((sock[1] = accept (listener, 0, 0)) < 0)
    goto fail;

  closesocket (listener);

  filedes [0] = sock [0];
  filedes [1] = sock [1];

  return 0;

fail:
  closesocket (listener);

  if (sock [0] != INVALID_SOCKET) closesocket (sock [0]);
  if (sock [1] != INVALID_SOCKET) closesocket (sock [1]);

  return -1;
}

#undef pipe
#define pipe(filedes) ev_pipe (filedes)

static int
ev_gettimeofday (struct timeval *tv, struct timezone *tz)
{
  struct _timeb tb;

  _ftime (&tb);

  tv->tv_sec  = (long)tb.time;
  tv->tv_usec = ((long)tb.millitm) * 1000;

  return 0;
}

#undef gettimeofday
#define gettimeofday(tv,tz) ev_gettimeofday (tv, tz)

#endif

