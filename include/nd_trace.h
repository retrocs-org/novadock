/* Runtime support capture in shipped programs; other clients compile hooks away. */
#ifndef ND_TRACE_H
#define ND_TRACE_H
#if defined(ND_TRACE) || defined(ND_RUNTIME_TRACE)
#include <exec/types.h>
#include "nd_plugin_support.h"
void nd_trace_plugin_support(unsigned int,const struct NDPluginSupportStats *);
enum { NDT_LOOP,NDT_TIMER,NDT_INPUT,NDT_IPC,NDT_PAINT,NDT_HOVER,NDT_QUEUE,
       NDT_PREF_EVENT,NDT_DOPUS_PROBE,NDT_DOPUS_UPDATE,NDT_DOPUS_ACTION,NDT_PACKET,NDT_SNAPSHOT,NDT_PREF_TICK,NDT_COUNT };
extern int nd_trace_level;
void nd_trace_init(const char *component);
void nd_trace_reload(void);
void nd_trace_marker(void);
void nd_trace_status(ULONG *queued,ULONG *dropped,ULONG *errors,int *persist);
ULONG nd_trace_now(void);
ULONG nd_trace_wait(ULONG signals);
void nd_trace_end(int metric,ULONG start);
void nd_trace_queue(ULONG seconds,ULONG micros,ULONG kind,unsigned int gadget);
void nd_trace_note(const char *event,LONG a,LONG b,LONG c,LONG d);
struct Screen;struct IntuitionBase;struct MsgPort;
void nd_trace_port_queue(struct MsgPort *);
void nd_trace_scene(struct Screen *,void *);
int nd_trace_due(void);
void nd_trace_options(LONG,LONG,LONG,LONG,LONG,LONG);
void nd_trace_pulse(void);
void nd_trace_flush(void);
void nd_trace_packet(unsigned int,unsigned int,unsigned int);
void nd_trace_packet_time(unsigned int,ULONG);
void nd_trace_tile_kind(unsigned int,const char *);
void nd_trace_phase(const char *,int,ULONG);
void nd_trace_page(unsigned int,const char *);
#define ND_TRACE_WAIT(s) nd_trace_wait(s)
#define ND_TRACE_TILE_KIND(i,s) do {if(nd_trace_level)nd_trace_tile_kind(i,s);} while(0)
#define ND_TRACE_PACKET(i,b,g) do {if(nd_trace_level)nd_trace_packet(i,b,g);} while(0)
#define ND_TRACE_PHASE_BEGIN(v,n) ULONG v=nd_trace_level ? nd_trace_now():0;if(nd_trace_level)nd_trace_phase(n,1,v)
#define ND_TRACE_PHASE_END(v,n) do {if(nd_trace_level)nd_trace_phase(n,0,v);} while(0)
#define ND_TRACE_PAGE(p,s) do {if(nd_trace_level)nd_trace_page(p,s);} while(0)
#define ND_TRACE_INIT(c) nd_trace_init(c)
#define ND_TRACE_RELOAD() nd_trace_reload()
#define ND_TRACE_START(v) ULONG v=nd_trace_level ? nd_trace_now():0
#define ND_TRACE_END(m,v) do {if(nd_trace_level)nd_trace_end(m,v);} while(0)
#define ND_TRACE_NOTE(e,a,b,c,d) do {if(nd_trace_level)nd_trace_note(e,a,b,c,d);} while(0)
#define ND_TRACE_SCENE(s,b) do {if(nd_trace_level)nd_trace_scene(s,b);} while(0)
#define ND_TRACE_PULSE() nd_trace_pulse()
#define ND_TRACE_FLUSH() nd_trace_flush()
#define ND_TRACE_PORT(p) do {if(nd_trace_level)nd_trace_port_queue(p);} while(0)
#define ND_TRACE_QUEUE(m,id) do {if(nd_trace_level)nd_trace_queue((m)->Seconds,(m)->Micros,(m)->Class,id);} while(0)
#else
#define nd_trace_level 0
#define nd_trace_plugin_support(i,s) ((void)0)
#define ND_TRACE_WAIT(s) Wait(s)
#define ND_TRACE_SCENE(s,b) ((void)0)
#define ND_TRACE_TILE_KIND(i,s) ((void)0)
#define ND_TRACE_PACKET(i,b,g) ((void)0)
#define ND_TRACE_PHASE_BEGIN(v,n)
#define ND_TRACE_PHASE_END(v,n) ((void)0)
#define ND_TRACE_PAGE(p,s) ((void)0)
#define ND_TRACE_INIT(c) ((void)0)
#define ND_TRACE_RELOAD() ((void)0)
#define ND_TRACE_START(v)
#define ND_TRACE_END(m,v) ((void)0)
#define ND_TRACE_NOTE(e,a,b,c,d) ((void)0)
#define ND_TRACE_PULSE() ((void)0)
#define ND_TRACE_FLUSH() ((void)0)
#define ND_TRACE_QUEUE(m,id) ((void)0)
#define ND_TRACE_PORT(p) ((void)0)
#endif
#endif
