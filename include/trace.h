// Included by the recompiled functions (us.toml's trace_mode): TRACE_ENTRY at
// each function's start, TRACE_RETURN before each return.
#ifndef DK64_TRACE_H
#define DK64_TRACE_H

#ifdef __3DS__
// The 3DS build's preemption check (rt64-3ds's runtime patches, nmr 0025):
// when a host thread has queued an external event (a VI retrace, a task's
// completion, a timer), the runtime delivers it here and switches to a woken
// higher-priority game thread, as the console's interrupt would have.
// Without it a long stretch of game logic held DK64's audio thread back until
// it next touched a message queue: on an Old 3DS the game handed over too few
// audio frames and the music slowed and stalled.
extern unsigned int ultramodern_preempt_pending;
extern void ultramodern_preempt(unsigned char* rdram);
#define TRACE_ENTRY() \
    if (*(volatile unsigned int*)&ultramodern_preempt_pending) { \
        ultramodern_preempt(rdram); \
    }
#else
// The desktop runtime has no such check: nothing.
#define TRACE_ENTRY()
#endif

#define TRACE_RETURN()

#endif
