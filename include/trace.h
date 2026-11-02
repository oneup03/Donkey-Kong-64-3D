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
#define TRACE_PREEMPT_CHECK() \
    if (*(volatile unsigned int*)&ultramodern_preempt_pending) { \
        ultramodern_preempt(rdram); \
    }

#ifdef DK64_GAME_PROFILE
// The function running, for rt64-3ds's sampling profile of the game
// (settings.ini's dbg_profile=2, game_profile.h): set at entry, the
// caller's put back at return, so a sample names the function executing.
// Only in a build with DK64_GAME_PROFILE=ON (platform/3ds/CMakeLists.txt):
// in every call it made the game's code bigger and cost two stores a call.
extern const char* volatile recomp3ds_trace_func;
#define TRACE_ENTRY() \
    TRACE_PREEMPT_CHECK() \
    const char* const trace_prev_ = recomp3ds_trace_func; \
    recomp3ds_trace_func = __func__;
#define TRACE_RETURN() \
    recomp3ds_trace_func = trace_prev_;
#else
#define TRACE_ENTRY() TRACE_PREEMPT_CHECK()
#define TRACE_RETURN()
#endif
#else
// The desktop runtime has neither: nothing.
#define TRACE_ENTRY()
#define TRACE_RETURN()
#endif

#endif
