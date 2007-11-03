/*
 * libevent compatibility layer
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

#include <stddef.h>
#include <stdlib.h>
#include <sys/time.h>

#include "event.h"

struct event_base
{
  int dummy;
};

static int x_actives;

static struct event_base x_base, *x_cur;

static void
tv_set (struct timeval *tv, ev_tstamp at)
{
  tv->tv_sec  = (long)at;
  tv->tv_usec = (long)((at - (ev_tstamp)tv->tv_sec) * 1e6);
}

static ev_tstamp
tv_get (struct timeval *tv)
{
  if (tv)
    return tv->tv_sec + tv->tv_usec * 1e-6;
  else
    return -1.;
}

#define EVENT_VERSION(a,b) # a "." # b

const char *event_get_version (void)
{
  return EVENT_VERSION (EV_VERSION_MAJOR, EV_VERSION_MINOR);
}

const char *event_get_method (void)
{
  return "libev";
}

void *event_init (void)
{
  if (!x_cur && ev_init (0))
    return x_cur = &x_base;

  return 0;
}

void event_base_free (struct event_base *base)
{
  /* nop */
}

int event_dispatch (void)
{
  return event_base_dispatch (x_cur);
}

#ifdef EV_STANDALONE
void event_set_log_callback (event_log_cb cb)
{
  /* nop */
}
#endif

int event_loop (int flags)
{
  return event_base_loop (x_cur, flags);
}

int event_loopexit (struct timeval *tv)
{
  return event_base_loopexit (x_cur, tv);
}

static void
x_cb (struct event *ev, int revents)
{
  if (ev->ev_events & EV_SIGNAL)
    {
      /* sig */
      if (ev_is_active (&ev->iosig.sig))
        {
          ev_signal_stop (&ev->iosig.sig);
          --x_actives;
        }
    }
  else
    {
       /* io */
      if (!(ev->ev_events & EV_PERSIST) && ev_is_active (&ev->iosig.io))
        {
          ev_io_stop (&ev->iosig.io);
          --x_actives;
        }
    }

  revents &= EV_READ | EV_WRITE | EV_TIMEOUT | EV_SIGNAL;

  if (revents & EV_TIMEOUT)
    --x_actives;

  ev->ev_res = revents;
  ev->ev_callback (ev->ev_fd, revents, ev->ev_arg);
}

static void
x_cb_io (struct ev_io *w, int revents)
{
  x_cb ((struct event *)(((char *)w) - offsetof (struct event, iosig.io)), revents);
}

static void
x_cb_to (struct ev_timer *w, int revents)
{
  x_cb ((struct event *)(((char *)w) - offsetof (struct event, to)), revents);
}

static void
x_cb_sig (struct ev_signal *w, int revents)
{
  x_cb ((struct event *)(((char *)w) - offsetof (struct event, iosig.sig)), revents);
}

void event_set (struct event *ev, int fd, short events, void (*cb)(int, short, void *), void *arg)
{
  if (events & EV_SIGNAL)
    ev_watcher_init (&ev->iosig.sig, x_cb_sig);
  else
    ev_watcher_init (&ev->iosig.io, x_cb_io);

  ev_watcher_init (&ev->to, x_cb_to);

  ev->ev_base     = x_cur;
  ev->ev_fd       = fd;
  ev->ev_events   = events;
  ev->ev_pri      = 0;
  ev->ev_callback = cb;
  ev->ev_arg      = arg;
  ev->ev_res      = 0;
}

int event_once (int fd, short events, void (*cb)(int, short, void *), void *arg, struct timeval *tv)
{
  return event_base_once (x_cur, fd, events, cb, arg, tv);
}

int event_add (struct event *ev, struct timeval *tv)
{
  /* disable all watchers */
  event_del (ev);

  if (ev->ev_events & EV_SIGNAL)
    {
      ev_signal_set (&ev->iosig.sig, ev->ev_fd);
      ev_signal_start (&ev->iosig.sig);
      ++x_actives;
    }
  else if (ev->ev_events & (EV_READ | EV_WRITE))
    {
      ev_io_set (&ev->iosig.io, ev->ev_fd, ev->ev_events & (EV_READ | EV_WRITE));
      ev_io_start (&ev->iosig.io);
      ++x_actives;
    }

  if (tv)
    {
      ev_timer_set (&ev->to, tv_get (tv), 0.);
      ev_timer_start (&ev->to);
      ++x_actives;
    }

  return 0;
}

int event_del (struct event *ev)
{
  if (ev->ev_events & EV_SIGNAL)
    {
      /* sig */
      if (ev_is_active (&ev->iosig.sig))
        {
          ev_signal_stop (&ev->iosig.sig);
          --x_actives;
        }
    }
  else
    {
      /* io */
      if (ev_is_active (&ev->iosig.io))
        {
          ev_io_stop (&ev->iosig.io);
          --x_actives;
        }
    }

  if (ev_is_active (&ev->to))
    {
      ev_timer_stop (&ev->to);
      --x_actives;
    }

  return 0;
}

int event_pending (struct event *ev, short events, struct timeval *tv)
{
  short revents = 0;

  if (ev->ev_events & EV_SIGNAL)
    {
      /* sig */
      if (ev->iosig.sig.pending)
        revents |= EV_SIGNAL;
    }
  else
    {
      /* io */
      if (ev->iosig.io.pending)
        revents |= ev->ev_events & (EV_READ | EV_WRITE);
    }

  if (ev->to.pending)
    {
      revents |= EV_TIMEOUT;

      if (tv)
        tv_set (tv, ev_now); /* not sure if this is right :) */
    }

  return events & revents;
}

int event_priority_init (int npri)
{
  return event_base_priority_init (x_cur, npri);
}

int event_priority_set (struct event *ev, int pri)
{
  ev->ev_pri = pri;

  return 0;
}

int event_base_set (struct event_base *base, struct event *ev)
{
  ev->ev_base = base;

  return 0;
}

int event_base_loop (struct event_base *base, int flags)
{
  do
    {
      ev_loop (flags | EVLOOP_ONESHOT);
    }
  while (!(flags & (EVLOOP_ONESHOT | EVLOOP_NONBLOCK)) && x_actives && !ev_loop_done);

  return 0;
}

int event_base_dispatch (struct event_base *base)
{
  return event_base_loop (base, 0);
}

static void
x_loopexit_cb (int revents, void *arg)
{
  ev_loop_done = 2;
}

int event_base_loopexit (struct event_base *base, struct timeval *tv)
{
  ev_tstamp after = tv_get (tv);

  ev_once (-1, 0, after >= 0. ? after : 0., x_loopexit_cb, (void *)base);

  return -1;
}

struct x_once
{
  int fd;
  void (*cb)(int, short, void *);
  void *arg;
};

static void
x_once_cb (int revents, void *arg)
{
  struct x_once *once = arg;

  once->cb (once->fd, revents, once->arg);
  free (once);
}

int event_base_once (struct event_base *base, int fd, short events, void (*cb)(int, short, void *), void *arg, struct timeval *tv)
{
  struct x_once *once = malloc (sizeof (struct x_once));

  if (!once)
    return -1;

  once->fd  = fd;
  once->cb  = cb;
  once->arg = arg;

  ev_once (fd, events & (EV_READ | EV_WRITE), tv_get (tv), x_once_cb, (void *)once);

  return 0;
}

int event_base_priority_init (struct event_base *base, int npri)
{
  return 0;
}

