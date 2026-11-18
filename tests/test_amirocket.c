#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_amirocket.h"
int main(void)
{
    struct NDRocketConfig c;struct NDRocketStatus s,old;char badge[8];
    const char *good="version=2 epoch=client-123 seq=4 connected=1 connected_servers=2 servers=3 unread=42 alerts=1 iconified=0\nserver=1 state=connected unread=42\n";
    nd_rocket_defaults(&c);assert(c.badge && !strcmp(c.port,"AUTO"));
    assert(nd_rocket_option(&c,"PORT","AMIROCKET.2"));assert(!nd_rocket_option(&c,"PORT","AMIROCKET.EVIL"));
    assert(!nd_rocket_option(&c,"PORT","AMIROCKET.2 SEND bad"));assert(!nd_rocket_option(&c,"PROGRAM","bad\nSHOW"));
    assert(nd_rocket_option(&c,"BADGE","NO") && !c.badge);
    assert(nd_rocket_port("AMIROCKET"));assert(nd_rocket_port("AMIROCKET.12"));assert(!nd_rocket_port("AMIROCKET."));assert(!nd_rocket_port("OTHERAMIROCKET"));
    assert(nd_rocket_parse(&s,good));assert(s.unread==42 && s.connected_servers==2 && s.servers==3);old=s;
    assert(!nd_rocket_parse(&s,"version=1 connected=1"));assert(!memcmp(&s,&old,sizeof(s)));
    assert(!nd_rocket_parse(&s,"version=2 epoch=a connected=1 connected_servers=1 servers=1 unread=4294967296 alerts=0 iconified=0"));
    assert(!nd_rocket_parse(&s,"version=2 epoch=a connected=1 connected_servers=0 servers=1 unread=1 alerts=0 iconified=0"));
    assert(!nd_rocket_parse(&s,"version=2 epoch=a connected=1 connected_servers=1 servers=1 unread=-1 alerts=0 iconified=0"));
    assert(!nd_rocket_parse(&s,"version=2 epoch=a connected=1 connected_servers=1 servers=1 unread=1 unread=2 alerts=0 iconified=0"));
    assert(!nd_rocket_parse(&s,"version=2 epoch=a connected=1 connected_servers=1 servers=9 unread=1 alerts=0 iconified=0"));
    assert(!nd_rocket_parse(&s,"version=2 epoch=a connected=1 connected_servers=1 servers=1 unread=1 alerts=0\niconified=0"));
    assert(nd_rocket_parse(&s,"version=2 epoch=b connected=0 connected_servers=0 servers=0 unread=0 alerts=0 iconified=1"));
    assert(nd_rocket_parse(&s,"version=2 epoch=c connected=1 connected_servers=1 servers=1 unread=4294967295 alerts=0 iconified=0"));
    nd_rocket_badge(badge,42,1,1);assert(!strcmp(badge,"42"));nd_rocket_badge(badge,1000,1,1);assert(!strcmp(badge,"999+"));
    nd_rocket_badge(badge,42,1,0);assert(!*badge);nd_rocket_badge(badge,42,0,1);assert(!*badge);nd_rocket_badge(badge,0,1,1);assert(!*badge);
    puts("AmiRocket: validated snapshots, client selection, settings and confirmed unread badges passed.");return 0;
}
