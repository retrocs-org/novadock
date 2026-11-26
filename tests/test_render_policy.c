#include <assert.h>
#include "nd_render_policy.h"
int main(void)
{
 unsigned long us,average=0;unsigned int previous=40,interval;int i;
 assert(nd_frame_interval(0)==40);
 for(us=0;us<2000000UL;us+=1000UL){interval=nd_frame_interval(us);assert(interval>=40&&interval<=200&&interval%20==0&&interval>=previous);previous=interval;}
 for(i=0;i<16;++i)average=nd_frame_average(average,100000UL);
 assert(nd_frame_interval(average)>=140);
 for(i=0;i<64;++i)average=nd_frame_average(average,1000UL);
 assert(nd_frame_interval(average)==40);
 assert(nd_frame_average(0,~0UL)==1000000UL);
 {
  struct NDFrameClock clock={0,0,0,0};unsigned long now=0,total=0;unsigned int q;
  assert(nd_timer_quanta(&clock,1,0,0)==0);
  /* Alternating animation waits plus render work: no lost time or drift. */
  for(i=0;i<1000;++i){unsigned long step=(i%5==0?190000UL:40000UL)+23000UL;now+=step;
   q=nd_timer_quanta(&clock,1+now/1000000UL,now%1000000UL,40000UL);total+=q*200000UL;
   assert(clock.carry<200000UL && total+clock.carry==now);
  }
  assert(nd_timer_quanta(&clock,0,0,200000UL)==1); /* backward clock */
 }
 return 0;
}
