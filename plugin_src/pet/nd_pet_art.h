#ifndef ND_PET_ART_H
#define ND_PET_ART_H
/* PET artwork, drawn from rectangles so the same shapes build the 32x32
 * bank (tools/generate_bundled_plugin_art.py).  Two characters share one
 * 16-colour palette; each has 16 states:
 *   cat    0-15: 0-7 idle (tail sway, a blink), 8-11 asleep (rising Zs),
 *                12-15 happy (hops with a heart);
 *   Gerald 16-31: a grumpy stapler - idle grumbling, asleep, and chomping;
 *   dog    32-47: tail wags, a snooze, happy hops with its tongue out;
 *   rabbit 48-63: nose twitches, a loaf-shaped nap, happy jumps (binkies);
 *   Bob    64-79: a polka-dot rubber ball that bounces, naps and chatters.
 * Every character uses the same idle 0-7 / asleep 8-11 / happy 12-15 order.
 * Colour 0 is transparent. */
#define ND_PET_IDLE_FIRST 0
#define ND_PET_IDLE_FRAMES 8
#define ND_PET_SLEEP_FIRST 8
#define ND_PET_SLEEP_FRAMES 4
#define ND_PET_HAPPY_FIRST 12
#define ND_PET_HAPPY_FRAMES 4
#define ND_PET_STATES 16          /* per character */
#define ND_PET_CAT 0
#define ND_PET_GERALD 1
#define ND_PET_DOG 2
#define ND_PET_RABBIT 3
#define ND_PET_BOB 4
#define ND_PET_CHARACTERS 5
/* outline, fur, light fur, white, pink, eye green, pupil, cushion, cushion
 * light, heart, Z, shadow, stapler red, light red, metal */
static const unsigned char nd_pet_rgb[16][3]={{0,0,0},{52,40,40},{226,138,62},{248,198,132},
 {244,242,234},{238,146,168},{98,186,96},{24,24,24},{72,98,168},{122,152,212},
 {220,58,72},{206,222,255},{92,72,62},{196,52,48},{232,104,92},{150,154,162}};
typedef void (*NDPetRect)(void *,int,int,int,int,int);
struct NDPetDraw {void *ctx;NDPetRect box;int dy;};
static void nd_pet_box(struct NDPetDraw *d,int c,int x,int y,int w,int h)
{
    y+=d->dy;
    if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}if(x+w>32)w=32-x;if(y+h>32)h=32-y;
    if(w>0&&h>0)d->box(d->ctx,c,x,y,w,h);
}
static void nd_pet_cushion(struct NDPetDraw *d)
{
    int dy=d->dy;d->dy=0;
    nd_pet_box(d,8,3,27,26,4);nd_pet_box(d,9,4,27,24,1);nd_pet_box(d,12,4,30,24,1);
    d->dy=dy;
}
static void nd_pet_sitting(struct NDPetDraw *d,int sway,int eyes)
{
    /* tail */
    nd_pet_box(d,1,24+sway,14,4,12);nd_pet_box(d,2,25+sway,15,2,10);nd_pet_box(d,3,25+sway,15,2,2);
    /* body and belly */
    nd_pet_box(d,1,9,15,15,12);nd_pet_box(d,2,10,16,13,10);nd_pet_box(d,3,13,19,7,7);
    /* paws */
    nd_pet_box(d,1,11,25,4,2);nd_pet_box(d,4,11,25,3,2);nd_pet_box(d,1,18,25,4,2);nd_pet_box(d,4,18,25,3,2);
    /* ears and head */
    nd_pet_box(d,1,8,2,5,5);nd_pet_box(d,2,9,3,3,4);nd_pet_box(d,5,10,4,1,2);
    nd_pet_box(d,1,19,2,5,5);nd_pet_box(d,2,20,3,3,4);nd_pet_box(d,5,21,4,1,2);
    nd_pet_box(d,1,7,5,17,11);nd_pet_box(d,2,8,6,15,9);
    /* muzzle, nose, mouth, whiskers */
    nd_pet_box(d,3,12,11,7,3);nd_pet_box(d,5,15,11,2,1);nd_pet_box(d,1,14,13,1,1);nd_pet_box(d,1,17,13,1,1);
    nd_pet_box(d,1,4,11,3,1);nd_pet_box(d,1,24,11,3,1);nd_pet_box(d,1,4,13,3,1);nd_pet_box(d,1,24,13,3,1);
    /* eyes: 0 open, 1 blink, 2 happy arcs */
    if(eyes==0){nd_pet_box(d,6,10,8,3,3);nd_pet_box(d,7,11,8,1,3);nd_pet_box(d,6,18,8,3,3);nd_pet_box(d,7,19,8,1,3);}
    else if(eyes==1){nd_pet_box(d,1,10,9,3,1);nd_pet_box(d,1,18,9,3,1);}
    else {nd_pet_box(d,1,10,9,1,1);nd_pet_box(d,1,11,8,1,1);nd_pet_box(d,1,12,9,1,1);
          nd_pet_box(d,1,18,9,1,1);nd_pet_box(d,1,19,8,1,1);nd_pet_box(d,1,20,9,1,1);}
}
static void nd_pet_z(struct NDPetDraw *d,int x,int y)
{
    nd_pet_box(d,11,x,y,4,1);nd_pet_box(d,11,x+2,y+1,1,1);nd_pet_box(d,11,x+1,y+2,1,1);nd_pet_box(d,11,x,y+3,4,1);
}
static void nd_pet_sleeping(struct NDPetDraw *d,int frame)
{
    /* curled up: body, head on the left, tail wrapped in front */
    nd_pet_box(d,1,6,17,22,10);nd_pet_box(d,2,7,18,20,8);nd_pet_box(d,3,15,20,10,5);
    nd_pet_box(d,1,4,10,5,4);nd_pet_box(d,2,5,11,3,3);nd_pet_box(d,1,11,10,5,4);nd_pet_box(d,2,12,11,3,3);
    nd_pet_box(d,1,3,13,15,12);nd_pet_box(d,2,4,14,13,10);nd_pet_box(d,3,6,19,9,4);
    nd_pet_box(d,1,6,17,3,1);nd_pet_box(d,1,11,17,3,1);nd_pet_box(d,5,9,19,2,1);
    nd_pet_box(d,1,5,25,20,2);nd_pet_box(d,2,6,25,18,1);
    /* Zs rise and fade in turn */
    nd_pet_z(d,20,10-2*(frame%4));
    if(frame%4>=2)nd_pet_z(d,26,5-(frame%4-2)*2);
}
static void nd_pet_heart(struct NDPetDraw *d,int x,int y)
{
    nd_pet_box(d,10,x,y,2,2);nd_pet_box(d,10,x+3,y,2,2);nd_pet_box(d,10,x,y+1,5,2);
    nd_pet_box(d,10,x+1,y+3,3,1);nd_pet_box(d,10,x+2,y+4,1,1);
}
/* ---- Gerald, the grumpy stapler (facing left) ----------------------- */
/* OPEN: 0 closed, 1-3 how far the jaw is open; EYES: 0 open and glaring,
 * 1 blink, 2 closed (asleep), 3 wide (surprised); BROW lifts the eyebrows. */
static void nd_gerald_body(struct NDPetDraw *d,int lift,int open,int eyes,int brow)
{
    int a=-lift-open*2;        /* the arm rises at the front when the jaw opens */
    /* desk shadow and base plate with its curved lip */
    nd_pet_box(d,12,2,29,28,2);
    nd_pet_box(d,1,2,24,28,5);nd_pet_box(d,15,3,25,26,3);nd_pet_box(d,4,4,25,10,1);
    nd_pet_box(d,1,2,21,3,4);nd_pet_box(d,15,3,22,1,2);
    /* mouth: dark inside, tongue and a row of staples on the jaw */
    if(open){
        nd_pet_box(d,1,4,22+a,14,24-(22+a));nd_pet_box(d,5,6,23,8,1);
        nd_pet_box(d,4,5,22+a,2,1);nd_pet_box(d,4,8,22+a,2,1);nd_pet_box(d,4,11,22+a,2,1);
    }
    /* hinge at the back */
    nd_pet_box(d,1,25,15-lift,6,10+lift);nd_pet_box(d,15,26,16-lift,4,8+lift);nd_pet_box(d,4,27,17-lift,1,2);
    /* the red arm: front part rises with OPEN, back part stays on the hinge */
    nd_pet_box(d,1,3,12+a,13,11);nd_pet_box(d,13,4,13+a,11,9);
    nd_pet_box(d,1,15,12-lift-open,12,11);nd_pet_box(d,13,15,13-lift-open,11,9);
    /* chrome cap along the top, a light red edge below it */
    nd_pet_box(d,15,4,13+a,11,2);nd_pet_box(d,4,5,13+a,6,1);
    nd_pet_box(d,15,15,13-lift-open,9,2);nd_pet_box(d,4,16,13-lift-open,4,1);
    nd_pet_box(d,14,4,15+a,11,1);nd_pet_box(d,14,15,15-lift-open,9,1);
    nd_pet_box(d,1,3,21+a,13,1);nd_pet_box(d,1,15,21-lift-open,11,1);
    /* rounded nose (colour 0 is transparent) */
    nd_pet_box(d,0,3,12+a,1,1);nd_pet_box(d,1,4,12+a,1,1);nd_pet_box(d,1,3,13+a,1,1);
    /* eyes */
    if(eyes==2){nd_pet_box(d,1,6,17+a,4,1);nd_pet_box(d,1,12,17+a,4,1);}
    else if(eyes==1){nd_pet_box(d,1,6,17+a,4,1);nd_pet_box(d,1,12,17+a,4,1);
                     nd_pet_box(d,4,6,16+a,4,1);nd_pet_box(d,4,12,16+a,4,1);}
    else{
        int h=eyes==3 ? 5:4;
        nd_pet_box(d,4,6,15+a-(h-4),4,h);nd_pet_box(d,4,12,15+a-(h-4),4,h);
        nd_pet_box(d,7,eyes==3 ? 7:6,16+a,2,2);nd_pet_box(d,7,eyes==3 ? 13:12,16+a,2,2);
    }
    /* eyebrows: slanted into a frown, unless surprised */
    if(eyes==3){nd_pet_box(d,1,6,11+a,4,1);nd_pet_box(d,1,12,11+a,4,1);}
    else if(eyes!=2){
        nd_pet_box(d,1,5,12+a-brow,2,2);nd_pet_box(d,1,7,13+a-brow,3,2);
        nd_pet_box(d,1,12,13+a-brow,3,2);nd_pet_box(d,1,15,12+a-brow,2,2);
    }
}
static void nd_gerald_art(void *ctx,NDPetRect box,int state)
{
    static const signed char breathe[8]={0,0,1,1,0,0,1,0};
    struct NDPetDraw d;
    d.ctx=ctx;d.box=box;d.dy=0;
    if(state>=ND_PET_HAPPY_FIRST){
        /* chomp: wide open with a "!", snap shut, open, snap with a staple
         * flying out of the front */
        int f=state-ND_PET_HAPPY_FIRST,open=(f&1) ? 0:(f==0 ? 3:2);
        nd_gerald_body(&d,0,open,open ? 3:0,0);
        if(f==0){nd_pet_box(&d,10,22,1,2,6);nd_pet_box(&d,10,22,8,2,2);}
        if(f==3){nd_pet_box(&d,4,0,21,2,1);nd_pet_box(&d,4,0,22,1,2);}
    }else if(state>=ND_PET_SLEEP_FIRST){
        nd_gerald_body(&d,0,0,2,0);
        nd_pet_z(&d,22,7-2*((state-ND_PET_SLEEP_FIRST)%4));
        if((state-ND_PET_SLEEP_FIRST)%4>=2)nd_pet_z(&d,27,3-((state-ND_PET_SLEEP_FIRST)%4-2)*2);
    }else nd_gerald_body(&d,breathe[state&7],0,state==5 ? 1:0,state==2 ? 1:0);
}
/* Draw STATE of the cat. */
static void nd_pet_cat_art(void *ctx,NDPetRect box,int state)
{
    static const signed char sway[8]={0,1,2,1,0,-1,-2,-1};
    struct NDPetDraw d;
    d.ctx=ctx;d.box=box;d.dy=0;
    nd_pet_cushion(&d);
    if(state>=ND_PET_HAPPY_FIRST){
        int f=state-ND_PET_HAPPY_FIRST;
        d.dy=(f&1) ? -2:0;
        nd_pet_sitting(&d,f&1 ? 2:-1,2);
        d.dy=0;nd_pet_heart(&d,26,1+(f>>1));
    }else if(state>=ND_PET_SLEEP_FIRST)nd_pet_sleeping(&d,state-ND_PET_SLEEP_FIRST);
    else nd_pet_sitting(&d,sway[state&7],state==6 ? 1:0);
}
/* ---- the dog ------------------------------------------------------- */
static void nd_dog_sitting(struct NDPetDraw *d,int wag,int eyes,int happy)
{
    int ear=happy ? -2:0;
    nd_pet_box(d,1,23+wag,17,4,8);nd_pet_box(d,12,24+wag,18,2,6);            /* tail */
    nd_pet_box(d,1,9,16,15,10);nd_pet_box(d,3,10,17,13,8);nd_pet_box(d,4,13,18,7,6);
    nd_pet_box(d,1,10,25,5,2);nd_pet_box(d,4,11,25,3,2);nd_pet_box(d,1,17,25,5,2);nd_pet_box(d,4,18,25,3,2);
    nd_pet_box(d,1,8,4,16,12);nd_pet_box(d,3,9,5,14,10);nd_pet_box(d,12,17,5,5,5);  /* head, eye patch */
    nd_pet_box(d,1,5,5+ear,5,10);nd_pet_box(d,12,6,6+ear,3,8);                   /* floppy ears */
    nd_pet_box(d,1,22,5+ear,5,10);nd_pet_box(d,12,23,6+ear,3,8);
    if(eyes==0){nd_pet_box(d,7,11,8,2,2);nd_pet_box(d,7,18,8,2,2);nd_pet_box(d,4,18,8,1,1);nd_pet_box(d,4,11,8,1,1);}
    else if(eyes==1){nd_pet_box(d,1,10,9,3,1);nd_pet_box(d,1,17,9,3,1);}
    else {nd_pet_box(d,1,10,9,1,1);nd_pet_box(d,1,11,8,1,1);nd_pet_box(d,1,12,9,1,1);
          nd_pet_box(d,1,17,9,1,1);nd_pet_box(d,1,18,8,1,1);nd_pet_box(d,1,19,9,1,1);}
    nd_pet_box(d,4,12,11,8,4);nd_pet_box(d,7,14,11,4,2);nd_pet_box(d,1,15,13,2,1);  /* muzzle, nose */
    if(happy){nd_pet_box(d,5,15,14,3,3);nd_pet_box(d,10,16,15,1,2);}            /* tongue */
}
static void nd_dog_art(void *ctx,NDPetRect box,int state)
{
    static const signed char wag[8]={0,2,0,-1,0,2,0,-1};
    struct NDPetDraw d;
    d.ctx=ctx;d.box=box;d.dy=0;
    nd_pet_cushion(&d);
    if(state>=ND_PET_HAPPY_FIRST){
        int f=state-ND_PET_HAPPY_FIRST;
        d.dy=(f&1) ? -2:0;nd_dog_sitting(&d,f&1 ? 2:-1,2,1);
        d.dy=0;nd_pet_heart(&d,26,1+(f>>1));
    }else if(state>=ND_PET_SLEEP_FIRST){
        int f=state-ND_PET_SLEEP_FIRST;
        nd_pet_box(&d,1,6,19,22,8);nd_pet_box(&d,3,7,20,20,6);nd_pet_box(&d,12,18,20,6,3);
        nd_pet_box(&d,1,4,15,12,10);nd_pet_box(&d,3,5,16,10,8);
        nd_pet_box(&d,1,3,16,4,9);nd_pet_box(&d,12,4,17,2,7);
        nd_pet_box(&d,1,8,19,3,1);nd_pet_box(&d,1,12,19,3,1);
        nd_pet_box(&d,4,6,21,6,3);nd_pet_box(&d,7,6,21,3,2);
        nd_pet_box(&d,1,5,25,8,2);nd_pet_box(&d,4,6,25,6,1);                     /* paws */
        nd_pet_z(&d,20,10-2*(f%4));if(f%4>=2)nd_pet_z(&d,26,5-(f%4-2)*2);
    }else nd_dog_sitting(&d,wag[state&7],state==6 ? 1:0,0);
}
/* ---- the rabbit ---------------------------------------------------- */
static void nd_rabbit_sitting(struct NDPetDraw *d,int twitch,int droop,int eyes)
{
    int g=d->dy;
    d->dy=0;nd_pet_box(d,6,4,27,24,2);nd_pet_box(d,6,6,26,2,1);nd_pet_box(d,6,21,26,2,1);d->dy=g;  /* grass */
    nd_pet_box(d,1,22,20,6,6);nd_pet_box(d,4,23,21,4,4);                         /* tail puff */
    nd_pet_box(d,1,9,15,15,12);nd_pet_box(d,4,10,16,13,10);nd_pet_box(d,15,10,24,13,2);
    nd_pet_box(d,1,8,25,7,2);nd_pet_box(d,4,9,25,5,1);nd_pet_box(d,1,18,25,7,2);nd_pet_box(d,4,19,25,5,1);
    nd_pet_box(d,1,10,0,5,9);nd_pet_box(d,4,11,1,3,8);nd_pet_box(d,5,12,2,1,6);  /* ears */
    nd_pet_box(d,1,18,droop,5,9-droop);nd_pet_box(d,4,19,1+droop,3,8-droop);nd_pet_box(d,5,20,2+droop,1,6-droop);
    nd_pet_box(d,1,8,7,17,10);nd_pet_box(d,4,9,8,15,8);                          /* head */
    if(eyes==0){nd_pet_box(d,7,11,10,2,3);nd_pet_box(d,7,19,10,2,3);nd_pet_box(d,4,11,10,1,1);nd_pet_box(d,4,19,10,1,1);}
    else if(eyes==1){nd_pet_box(d,1,11,11,3,1);nd_pet_box(d,1,18,11,3,1);}
    else {nd_pet_box(d,1,10,11,1,1);nd_pet_box(d,1,11,10,1,1);nd_pet_box(d,1,12,11,1,1);
          nd_pet_box(d,1,18,11,1,1);nd_pet_box(d,1,19,10,1,1);nd_pet_box(d,1,20,11,1,1);}
    nd_pet_box(d,5,15,13+twitch,3,1);nd_pet_box(d,1,16,14+twitch,1,1);            /* nose */
    nd_pet_box(d,1,5,13,4,1);nd_pet_box(d,1,24,13,4,1);nd_pet_box(d,5,10,14,2,1);nd_pet_box(d,5,21,14,2,1);
}
static void nd_rabbit_art(void *ctx,NDPetRect box,int state)
{
    static const signed char twitch[8]={0,1,0,1,0,0,0,0};
    struct NDPetDraw d;
    d.ctx=ctx;d.box=box;d.dy=0;
    if(state>=ND_PET_HAPPY_FIRST){
        int f=state-ND_PET_HAPPY_FIRST;
        d.dy=(f&1) ? -4:0;nd_rabbit_sitting(&d,0,0,2);
        d.dy=0;nd_pet_heart(&d,26,1+(f>>1));
    }else if(state>=ND_PET_SLEEP_FIRST){
        int f=state-ND_PET_SLEEP_FIRST;
        nd_pet_box(&d,6,4,27,24,2);
        nd_pet_box(&d,1,7,17,20,10);nd_pet_box(&d,4,8,18,18,8);nd_pet_box(&d,15,8,25,18,1);
        nd_pet_box(&d,1,25,19,4,4);nd_pet_box(&d,4,26,20,2,2);
        nd_pet_box(&d,1,13,13,13,4);nd_pet_box(&d,4,14,14,11,2);nd_pet_box(&d,5,15,15,9,1);  /* ears laid back */
        nd_pet_box(&d,1,4,14,12,11);nd_pet_box(&d,4,5,15,10,9);
        nd_pet_box(&d,1,6,19,3,1);nd_pet_box(&d,1,11,19,3,1);nd_pet_box(&d,5,9,21,2,1);
        nd_pet_z(&d,20,9-2*(f%4));if(f%4>=2)nd_pet_z(&d,26,4-(f%4-2)*2);
    }else nd_rabbit_sitting(&d,twitch[state&7],state==3 || state==4 ? 3:0,state==6 ? 1:0);
}
/* ---- Bob, the rubber ball ------------------------------------------ */
static int nd_pet_isqrt(int v)
{
    int r=0;
    while((r+1)*(r+1)<=v)++r;
    return r;
}
/* An ellipse of radii RX, RY around CX, CY. */
static void nd_pet_ellipse(struct NDPetDraw *d,int c,int cx,int cy,int rx,int ry)
{
    int dy,r2=(2*ry+1)*(2*ry+1);
    /* Sample each row at its centre so the top and bottom rows have width. */
    for(dy=-ry;dy<=ry;++dy){
        int w=(2*rx+1)*nd_pet_isqrt((r2-4*dy*dy)*256/r2)/32;
        nd_pet_box(d,c,cx-w,cy+dy,2*w+1,1);
    }
}
/* MOUTH: 0 smile, 1 open (talking or wheee), 2 closed (asleep). */
static void nd_bob(struct NDPetDraw *d,int cy,int rx,int ry,int mouth,int eyes)
{
    int cx=15,shadow=8+(cy-17)/2;
    if(shadow<4)shadow=4;
    nd_pet_box(d,12,cx-shadow,29,2*shadow+1,2);                                   /* shadow */
    nd_pet_ellipse(d,1,cx,cy,rx+1,ry+1);nd_pet_ellipse(d,8,cx,cy,rx,ry);
    nd_pet_ellipse(d,9,cx-3,cy-ry/2,rx/3,ry/4);                                   /* shine */
    nd_pet_box(d,4,cx-rx+2,cy+1,2,2);nd_pet_box(d,4,cx+rx-4,cy-3,2,2);           /* polka dots */
    nd_pet_box(d,4,cx+2,cy+ry-3,2,2);nd_pet_box(d,4,cx-6,cy-ry+3,2,2);
    if(eyes==2){nd_pet_box(d,1,cx-5,cy-2,3,1);nd_pet_box(d,1,cx+2,cy-2,3,1);}
    else{nd_pet_box(d,4,cx-5,cy-4,4,4);nd_pet_box(d,4,cx+1,cy-4,4,4);
         nd_pet_box(d,7,cx-4+(eyes==1),cy-3,2,2);nd_pet_box(d,7,cx+2+(eyes==1),cy-3,2,2);}
    if(mouth==1){nd_pet_box(d,1,cx-3,cy+2,6,4);nd_pet_box(d,5,cx-2,cy+4,4,1);}
    else if(mouth==2)nd_pet_box(d,1,cx-1,cy+3,3,1);
    else{nd_pet_box(d,1,cx-3,cy+3,6,1);nd_pet_box(d,1,cx-4,cy+2,1,1);nd_pet_box(d,1,cx+3,cy+2,1,1);}
}
static void nd_bob_art(void *ctx,NDPetRect box,int state)
{
    /* one bounce per idle cycle: squashed on landing, round in the air */
    static const signed char height[8]={0,3,6,8,8,6,3,0};
    struct NDPetDraw d;
    d.ctx=ctx;d.box=box;d.dy=0;
    if(state>=ND_PET_HAPPY_FIRST){
        int f=state-ND_PET_HAPPY_FIRST;
        static const signed char jump[4]={2,10,12,4};
        nd_bob(&d,18-jump[f],10,10,f&1 ? 1:0,f==2 ? 1:0);
        if(f==1 || f==2){nd_pet_box(&d,10,27,2,2,6);nd_pet_box(&d,10,27,9,2,2);}  /* "!" */
    }else if(state>=ND_PET_SLEEP_FIRST){
        int f=state-ND_PET_SLEEP_FIRST;
        nd_bob(&d,19,11,9,2,2);
        nd_pet_z(&d,22,7-2*(f%4));if(f%4>=2)nd_pet_z(&d,27,3-(f%4-2)*2);
    }else{
        int h=height[state&7],squash=(state&7)==0 || (state&7)==7;
        nd_bob(&d,18-h+(squash ? 1:0),squash ? 11:10,squash ? 9:10,0,0);
    }
}
/* Draw bank STATE: character*16 + state (cat, Gerald, dog, rabbit, Bob). */
static void nd_pet_art(void *ctx,NDPetRect box,int state)
{
    int who=state/ND_PET_STATES,s=state%ND_PET_STATES;
    if(who==ND_PET_GERALD)nd_gerald_art(ctx,box,s);
    else if(who==ND_PET_DOG)nd_dog_art(ctx,box,s);
    else if(who==ND_PET_RABBIT)nd_rabbit_art(ctx,box,s);
    else if(who==ND_PET_BOB)nd_bob_art(ctx,box,s);
    else nd_pet_cat_art(ctx,box,s);
}
#endif
