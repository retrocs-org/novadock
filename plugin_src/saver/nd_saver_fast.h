/* Cached native Boing frames. All caches are optional and bounded. */
#define ND_SAVER_CACHE_LIMIT 786432UL
static void saver_free_cache(struct NDSaverWindow *s)
{
 if(s->background || s->sprites)WaitBlit();
 if(s->background)FreeBitMap(s->background);
 if(s->sprites)FreeBitMap(s->sprites);
 s->background=s->sprites=NULL;s->sprite_ready=0;s->fast_previous=0;
}
static int saver_cache_open(struct NDSaverWindow *s)
{
 ULONG depth,cost;struct BitMap *original;unsigned int at,end;int row,start,scale,left[ND_BOING_SIZE],right[ND_BOING_SIZE],x,r;
 if(s->cache_failed)return 0;if(s->background)return 1;
 /* Integer scaling makes cached runs exactly match the portable renderer. */
 if(s->width%320 || s->height%160 || s->width/320!=s->height/160 || s->width>640)return 0;
 s->sprite_w=s->width/4;s->sprite_h=s->height/2;scale=s->width/320;
 depth=GetBitMapAttr(s->bitmap,BMA_DEPTH);
 cost=((ULONG)s->width*s->height+(ULONG)s->sprite_w*s->sprite_h*ND_BOING_FRAMES)*depth/8UL;
 if(cost>ND_SAVER_CACHE_LIMIT){s->cache_failed=1;return 0;}
 s->background=AllocBitMap(s->width,s->height,depth,BMF_CLEAR,s->bitmap);
 s->sprites=AllocBitMap(s->sprite_w,s->sprite_h*ND_BOING_FRAMES,depth,BMF_CLEAR,s->bitmap);
 if(!s->background || !s->sprites){saver_free_cache(s);s->cache_failed=1;return 0;}
 /* Copy opaque silhouette bands rather than using masked deep-bitplane blits.
  * This also avoids the V39/V40 interleaved-mask incompatibility. */
 for(row=0;row<ND_BOING_SIZE;++row){left[row]=80*scale;right[row]=0;}
 end=nd_boing_offsets[1];for(at=0;at<end;at+=4){row=nd_boing_runs[at+1];x=nd_boing_runs[at]*2*scale;r=x+nd_boing_runs[at+2]*2*scale;if(x<left[row])left[row]=x;if(r>right[row])right[row]=r;}
 s->band_count=0;
 for(row=0;row<ND_BOING_SIZE;){start=row++;while(row<ND_BOING_SIZE && left[row]==left[start] && right[row]==right[start])++row;
  if(right[start]>left[start]){struct NDSaverBand *band=&s->bands[s->band_count++];band->x=(UWORD)left[start];band->y=(UWORD)(start*2*scale);band->w=(UWORD)(right[start]-left[start]);band->h=(UWORD)((row-start)*2*scale);}}
 original=s->paint.BitMap;s->paint.BitMap=s->background;s->last_pen=-1;
 nd_saver_boing_background(s,saver_rect);
 s->paint.BitMap=original;s->last_pen=-1;return 1;
}
static void saver_cache_frame(struct NDSaverWindow *s,unsigned int phase)
{
 struct RastPort sprite;unsigned int at,end;int scale=s->width/320,x,y,w,h,c,last=-1,offset=phase*s->sprite_h;
 if(s->sprite_ready&(1UL<<phase))return;
 InitRastPort(&sprite);sprite.BitMap=s->sprites;SetDrMd(&sprite,JAM1);
 at=nd_boing_offsets[phase];end=nd_boing_offsets[phase+1];
 for(;at<end;at+=4){x=nd_boing_runs[at]*2*scale;y=nd_boing_runs[at+1]*2*scale;
  w=nd_boing_runs[at+2]*2*scale;h=2*scale;c=nd_boing_runs[at+3];
  if(c!=last){SetAPen(&sprite,s->pens[c]);last=c;}
  RectFill(&sprite,x,offset+y,x+w-1,offset+y+h-1);
 }
 s->sprite_ready|=1UL<<phase;
}
static int saver_fast_boing(struct NDSaverWindow *s)
{
 int x,y,left,top,right,bottom,scale=s->width/320;unsigned int band,phase=(unsigned int)(s->frame%ND_BOING_FRAMES);
 if(!saver_cache_open(s))return 0;
 nd_saver_boing_position(s->frame,&x,&y);left=0;top=0;right=320;bottom=160;
 if(s->fast_previous){
  left=(x<s->fast_x ? x:s->fast_x)-40;right=(x>s->fast_x ? x:s->fast_x)+51;
  top=(y<s->fast_y ? y:s->fast_y)-40;bottom=150;
  if(left<0)left=0;if(right>320)right=320;if(top<0)top=0;
 }
 s->dirty_x=s->xmap[left];s->dirty_y=s->ymap[top];
 s->dirty_w=s->xmap[right]-s->dirty_x;s->dirty_h=s->ymap[bottom]-s->dirty_y;
 BltBitMapRastPort(s->background,s->dirty_x,s->dirty_y,&s->paint,
  s->dirty_x,s->dirty_y,s->dirty_w,s->dirty_h,0xc0);
 nd_saver_boing_shadow(s,saver_rect,x,y);saver_cache_frame(s,phase);
 for(band=0;band<s->band_count;++band){const struct NDSaverBand *r=&s->bands[band];
  BltBitMapRastPort(s->sprites,r->x,phase*s->sprite_h+r->y,&s->paint,
   (x-40)*scale+r->x,(y-40)*scale+r->y,r->w,r->h,0xc0);}
 s->fast_previous=1;s->fast_x=x;s->fast_y=y;return 1;
}
