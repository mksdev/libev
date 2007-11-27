#ifndef EVPP_H__
#define EVPP_H__

#include "ev.h"

namespace ev {

  template<class watcher>
  class callback
  {
    struct klass; // it is vital that this is never defined

    klass *o;
    void (klass::*m)(watcher &, int);

  public:
    template<class O1, class O2>
    explicit callback (O1 *object, void (O2::*method)(watcher &, int))
    {
      o = reinterpret_cast<klass *>(object);
      m = reinterpret_cast<void (klass::*)(watcher &, int)>(method);
    }

    // this works because a standards-compliant C++ compiler
    // basically can't help it: it doesn't have the knowledge
    // required to miscompile (klass is not defined anywhere
    // and nothing is known about the constructor arguments) :)
    void call (watcher *w, int revents)
    {
      (o->*m) (*w, revents);
    }
  };

  enum {
    UNDEF    = EV_UNDEF,
    NONE     = EV_NONE,
    READ     = EV_READ,
    WRITE    = EV_WRITE,
    TIMEOUT  = EV_TIMEOUT,
    PERIODIC = EV_PERIODIC,
    SIGNAL   = EV_SIGNAL,
    CHILD    = EV_CHILD,
    STAT     = EV_STAT,
    IDLE     = EV_IDLE,
    CHECK    = EV_CHECK,
    PREPARE  = EV_PREPARE,
    FORK     = EV_FORK,
    EMBED    = EV_EMBED,
    ERROR    = EV_ERROR,
  };

  typedef ev_tstamp tstamp;

  inline ev_tstamp now (EV_P)
  {
    return ev_now (EV_A);
  }

  #if EV_MULTIPLICITY

    #define EV_CONSTRUCT(cppstem)							\
      EV_P;                                                                             \
                                                                                        \
      void set (EV_P)                                                                   \
      {                                                                                 \
        this->EV_A = EV_A;                                                              \
      }                                                                                 \
                                                                                        \
      template<class O1, class O2>                                                      \
      explicit cppstem (O1 *object, void (O2::*method)(cppstem &, int), EV_P = ev_default_loop (0)) \
      : callback<cppstem> (object, method), EV_A (EV_A)

  #else

    #define EV_CONSTRUCT(cppstem)							\
      template<class O1, class O2>							\
      explicit cppstem (O1 *object, void (O2::*method)(cppstem &, int))                 \
      : callback<cppstem> (object, method)

  #endif

  /* using a template here would require quite a bit more lines,
   * so a macro solution was chosen */
  #define EV_BEGIN_WATCHER(cppstem,cstem)	                                        \
                                                                                        \
  struct cppstem : ev_ ## cstem, callback<cppstem>                                      \
  {                                                                                     \
    EV_CONSTRUCT (cppstem)                                                              \
    {                                                                                   \
      ev_init (static_cast<ev_ ## cstem *>(this), thunk);                               \
    }                                                                                   \
                                                                                        \
    bool is_active () const                                                             \
    {                                                                                   \
      return ev_is_active (static_cast<const ev_ ## cstem *>(this));                    \
    }                                                                                   \
                                                                                        \
    bool is_pending () const                                                            \
    {                                                                                   \
      return ev_is_pending (static_cast<const ev_ ## cstem *>(this));                   \
    }                                                                                   \
                                                                                        \
    void start ()                                                                       \
    {                                                                                   \
      ev_ ## cstem ## _start (EV_A_ static_cast<ev_ ## cstem *>(this));                 \
    }                                                                                   \
                                                                                        \
    void stop ()                                                                        \
    {                                                                                   \
      ev_ ## cstem ## _stop (EV_A_ static_cast<ev_ ## cstem *>(this));                  \
    }                                                                                   \
                                                                                        \
    void operator ()(int events = EV_UNDEF)                                             \
    {                                                                                   \
      return call (this, events);                                                       \
    }                                                                                   \
                                                                                        \
    ~cppstem ()                                                                         \
    {                                                                                   \
      stop ();                                                                          \
    }                                                                                   \
                                                                                        \
  private:                                                                              \
                                                                                        \
    cppstem (const cppstem &o)								\
    : callback<cppstem> (this, (void (cppstem::*)(cppstem &, int))0)                    \
    { /* disabled */ }                                        				\
                                                                                        \
    void operator =(const cppstem &o) { /* disabled */ }                                \
                                                                                        \
    static void thunk (EV_P_ struct ev_ ## cstem *w, int revents)                       \
    {                                                                                   \
      (*static_cast<cppstem *>(w))(revents);                                            \
    }                                                                                   \
                                                                                        \
  public:

  #define EV_END_WATCHER(cppstem,cstem)	                                                \
  };

  EV_BEGIN_WATCHER (io, io)
    void set (int fd, int events)
    {
      int active = is_active ();
      if (active) stop ();
      ev_io_set (static_cast<ev_io *>(this), fd, events);
      if (active) start ();
    }

    void set (int events)
    {
      int active = is_active ();
      if (active) stop ();
      ev_io_set (static_cast<ev_io *>(this), fd, events);
      if (active) start ();
    }

    void start (int fd, int events)
    {
      set (fd, events);
      start ();
    }
  EV_END_WATCHER (io, io)

  EV_BEGIN_WATCHER (timer, timer)
    void set (ev_tstamp after, ev_tstamp repeat = 0.)
    {
      int active = is_active ();
      if (active) stop ();
      ev_timer_set (static_cast<ev_timer *>(this), after, repeat);
      if (active) start ();
    }

    void start (ev_tstamp after, ev_tstamp repeat = 0.)
    {
      set (after, repeat);
      start ();
    }

    void again ()
    {
      ev_timer_again (EV_A_ static_cast<ev_timer *>(this));
    }
  EV_END_WATCHER (timer, timer)

  #if EV_PERIODIC_ENABLE
  EV_BEGIN_WATCHER (periodic, periodic)
    void set (ev_tstamp at, ev_tstamp interval = 0.)
    {
      int active = is_active ();
      if (active) stop ();
      ev_periodic_set (static_cast<ev_periodic *>(this), at, interval, 0);
      if (active) start ();
    }

    void start (ev_tstamp at, ev_tstamp interval = 0.)
    {
      set (at, interval);
      start ();
    }

    void again ()
    {
      ev_periodic_again (EV_A_ static_cast<ev_periodic *>(this));
    }
  EV_END_WATCHER (periodic, periodic)
  #endif

  EV_BEGIN_WATCHER (sig, signal)
    void set (int signum)
    {
      int active = is_active ();
      if (active) stop ();
      ev_signal_set (static_cast<ev_signal *>(this), signum);
      if (active) start ();
    }

    void start (int signum)
    {
      set (signum);
      start ();
    }
  EV_END_WATCHER (sig, signal)

  EV_BEGIN_WATCHER (child, child)
    void set (int pid)
    {
      int active = is_active ();
      if (active) stop ();
      ev_child_set (static_cast<ev_child *>(this), pid);
      if (active) start ();
    }

    void start (int pid)
    {
      set (pid);
      start ();
    }
  EV_END_WATCHER (child, child)

  #if EV_STAT_ENABLE
  EV_BEGIN_WATCHER (stat, stat)
    void set (const char *path, ev_tstamp interval = 0.)
    {
      int active = is_active ();
      if (active) stop ();
      ev_stat_set (static_cast<ev_stat *>(this), path, interval);
      if (active) start ();
    }

    void start (const char *path, ev_tstamp interval = 0.)
    {
      set (path, interval);
      start ();
    }

    void update ()
    {
      ev_stat_stat (EV_A_ static_cast<ev_stat *>(this));
    }
  EV_END_WATCHER (stat, stat)
  #endif

  EV_BEGIN_WATCHER (idle, idle)
    void set () { }
  EV_END_WATCHER (idle, idle)

  EV_BEGIN_WATCHER (prepare, prepare)
    void set () { }
  EV_END_WATCHER (prepare, prepare)

  EV_BEGIN_WATCHER (check, check)
    void set () { }
  EV_END_WATCHER (check, check)

  #if EV_EMBED_ENABLE
  EV_BEGIN_WATCHER (embed, embed)
    void set (struct ev_loop *loop)
    {
      int active = is_active ();
      if (active) stop ();
      ev_embed_set (static_cast<ev_embed *>(this), loop);
      if (active) start ();
    }

    void start (struct ev_loop *embedded_loop)
    {
      set (embedded_loop);
      start ();
    }

    void sweep ()
    {
      ev_embed_sweep (EV_A_ static_cast<ev_embed *>(this));
    }
  EV_END_WATCHER (embed, embed)
  #endif

  #if EV_FORK_ENABLE
  EV_BEGIN_WATCHER (fork, fork)
    void set () { }
  EV_END_WATCHER (fork, fork)
  #endif

  #undef EV_CONSTRUCT
  #undef EV_BEGIN_WATCHER
  #undef EV_END_WATCHER
}

#endif

