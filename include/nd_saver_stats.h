#ifndef ND_SAVER_STATS_H
#define ND_SAVER_STATS_H
/* DateStamp ticks are 20 ms. Aggregate rather than timing individual pixels. */
struct NDSaverStats {
 unsigned long start,last,frames,draw_ticks,max_ticks,fps10,average_ms,peak_ms;
 unsigned long total_frames,total_intervals,total_ticks,total_draw,total_max;
};
static void nd_saver_stats_frame(struct NDSaverStats *s,unsigned long now,
                                unsigned long draw)
{
 unsigned long elapsed;
 if(!s->frames)s->start=now;
 if(s->last && now-s->last>1500UL){s->frames=s->draw_ticks=s->max_ticks=0;s->start=now;}
 elapsed=s->total_frames && s->last && now-s->last<=1500UL ? now-s->last:0;
 if(elapsed)++s->total_intervals;s->last=now;
 ++s->frames;s->draw_ticks+=draw;if(draw>s->max_ticks)s->max_ticks=draw;
 ++s->total_frames;s->total_ticks+=elapsed;s->total_draw+=draw;
 if(draw>s->total_max)s->total_max=draw;
 if(now-s->start>=100UL){
  s->fps10=(s->frames-1UL)*500UL/(now-s->start);
  s->average_ms=s->draw_ticks*20UL/s->frames;s->peak_ms=s->max_ticks*20UL;
  s->frames=s->draw_ticks=s->max_ticks=0;
 }
}
#endif
