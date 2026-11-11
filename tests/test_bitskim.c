#include "../plugin_src/bitskim/nd_bitskim.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const char *ready="ACTIVE=0 INCOMING=0 AWAITING=0 ICONIFIED=1 RECEIVING=1 VISIBLE=1 PORT=46440 PEERS=3 BYTES=0 TOTAL=0 RELOAD=0";
int main(void)
{
    struct NDBitSkimConfig c;struct NDBitSkimStatus s,old;char value[32],badge[8];unsigned int meter;unsigned long i;
    nd_bitskim_defaults(&c);assert(!strcmp(c.program,"SYS:Utilities/BitSkim/BitSkim"));assert(!strcmp(c.port,"AUTO") && c.poll_ticks==50);
    assert(nd_bitskim_port("BITSKIM") && nd_bitskim_port("BITSKIM.9"));
    assert(!nd_bitskim_port("BITSKIMX") && !nd_bitskim_port("BITSKIM.") && !nd_bitskim_port("BITSKIM.1;QUIT"));
    assert(nd_bitskim_option(&c,"PROGRAM","Bin:BitSkim/BitSkim"));
    assert(!strcmp(c.program,"SYS:Utilities/BitSkim/BitSkim"));
    assert(nd_bitskim_option(&c,"PROGRAM","Work:File tools/BitSkim"));
    assert(!nd_bitskim_option(&c,"PROGRAM","bad\npath"));
    assert(nd_bitskim_option(&c,"PORT","BITSKIM.2"));assert(!nd_bitskim_option(&c,"PORT","OTHER"));
    assert(!nd_bitskim_option(&c,"POLL_TICKS","-1") && !nd_bitskim_option(&c,"POLL_TICKS","24"));
    assert(nd_bitskim_parse(&s,ready));assert(s.peers==3 && s.receiving && s.iconified);
    nd_bitskim_display(&s,value,&meter,badge,1);assert(!strcmp(value,"3 DEVICES") && !strcmp(badge,"3") && meter==65535U);
    old=s;assert(!nd_bitskim_parse(&s,"ACTIVE=1"));assert(!memcmp(&s,&old,sizeof(s)));
    assert(!nd_bitskim_parse(&s,"ACTIVE=2 INCOMING=0 AWAITING=0 ICONIFIED=0 RECEIVING=1 VISIBLE=1 PEERS=0 BYTES=0 TOTAL=0"));
    assert(!nd_bitskim_parse(&s,"ACTIVE=0 ACTIVE=0 INCOMING=0 AWAITING=0 ICONIFIED=0 RECEIVING=1 VISIBLE=1 PEERS=0 BYTES=0 TOTAL=0"));
    assert(!nd_bitskim_parse(&s,"ACTIVE=1 INCOMING=0 AWAITING=0 ICONIFIED=0 RECEIVING=1 VISIBLE=1 PEERS=0 BYTES=20 TOTAL=10"));
    assert(!nd_bitskim_parse(&s,"ACTIVE=1 INCOMING=0 AWAITING=0 ICONIFIED=0 RECEIVING=1 VISIBLE=1 PEERS=0 BYTES=-1 TOTAL=10"));
    assert(nd_bitskim_parse(&s,"ACTIVE=1 INCOMING=1 AWAITING=0 ICONIFIED=0 RECEIVING=1 VISIBLE=1 PEERS=1 BYTES=1073741823 TOTAL=2147483647"));
    nd_bitskim_display(&s,value,&meter,badge,2);assert(!strcmp(value,"RECV 49%") && meter==49 && !strcmp(badge,"*"));
    s.awaiting=1;nd_bitskim_display(&s,value,&meter,badge,1);assert(!strcmp(value,"INCOMING FILE") && !strcmp(badge,"!"));
    s.incoming=0;nd_bitskim_display(&s,value,&meter,badge,0);assert(!strcmp(value,"WAIT ACCEPT") && !badge[0]);
    s.active=s.awaiting=s.receiving=0;nd_bitskim_display(&s,value,&meter,badge,2);assert(!strcmp(value,"RECEIVING OFF") && !badge[0]);
    for(i=0;i<=1000;++i)assert(nd_bitskim_percent(i,1000)==i/10);
    assert(nd_bitskim_percent(0,0)==0 && nd_bitskim_percent(1,1)==100 && nd_bitskim_percent(2147483647UL,2147483647UL)==100);
    {
        struct NDBitSkimTransfer t,previous;struct NDBitSkimTracker tracker;
        char reply[1600],out[1200],decoded[256],local[256];unsigned long n;
        const char *session="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        assert(nd_bitskim_uint("4294967295",&n) && n==4294967295UL);
        assert(!nd_bitskim_uint("4294967296",&n) && !nd_bitskim_uint("-1",&n));
        assert(nd_bitskim_field("A=1 EMPTY= Z=3","EMPTY",out,sizeof(out)) && !out[0]);
        assert(!nd_bitskim_field("A=1 A=2","A",out,sizeof(out)));
        assert(nd_bitskim_decode("caf%C3%A9%20%22A%22%25+",decoded,sizeof(decoded)));
        assert(!strcmp(decoded,"caf\303\251 \"A\"%+"));
        nd_bitskim_local(local,sizeof(local),decoded);assert(!strcmp(local,"caf\351 \"A\"%+"));
        assert(nd_bitskim_decode("%2520",decoded,sizeof(decoded)) && !strcmp(decoded,"%20"));
        assert(!nd_bitskim_decode("%00",decoded,sizeof(decoded)) && !nd_bitskim_decode("%GG",decoded,sizeof(decoded)));
        assert(!nd_bitskim_decode("%2",decoded,sizeof(decoded)) && !nd_bitskim_decode("abc",decoded,3));
        sprintf(reply,"AVAILABLE=1 SESSION=%s ID=%s-00000001 ACTIVE=0 INCOMING=1 BYTES=123 TOTAL=123 RESULT=1 FILENAME=caf%%C3%%A9%%20%%22A%%22.txt PEER=My%%20Mac ADDRESS=1.2.3.4 ERROR=",session,session);
        assert(nd_bitskim_transfer(&t,reply) && t.available && t.result==1 && !strcmp(t.peer,"My Mac"));
        assert(!strcmp(t.filename,"caf\303\251 \"A\".txt"));previous=t;
        assert(!nd_bitskim_transfer(&t,"AVAILABLE=1 SESSION=bad") && !memcmp(&t,&previous,sizeof(t)));
        assert(nd_bitskim_transfer(&t,"AVAILABLE=0") && !t.available);
        memset(&tracker,0,sizeof(tracker));
        sprintf(reply,"LATEST=3 LOST=0 SESSION=%s 3:4:%s-00000001:1",session,session);
        assert(nd_bitskim_events(&tracker,reply)==1 && tracker.baseline);
        nd_bitskim_track(&tracker,&previous);assert(!tracker.unread && !tracker.baseline);
        assert(nd_bitskim_events(&tracker,reply)==0);
        previous.id[40]='2';previous.result=2;
        sprintf(reply,"LATEST=9 LOST=1 SESSION=%s",session);
        assert(nd_bitskim_events(&tracker,reply)==1);nd_bitskim_track(&tracker,&previous);assert(tracker.unread && tracker.last.result==2);
        tracker.unread=0;nd_bitskim_track(&tracker,&previous);assert(!tracker.unread);
        assert(nd_bitskim_events(&tracker,"LATEST=1 LOST=1 SESSION=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb")==1 && tracker.baseline && !tracker.unread);
        nd_bitskim_track(&tracker,&previous);assert(!tracker.unread);
        assert(nd_bitskim_events(&tracker,"LATEST=4294967296 LOST=0 SESSION=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")==-1);
        assert(nd_bitskim_send_command(out,sizeof(out),session,"Work:Shared \"Old\"/a%b.txt"));
        assert(!strcmp(out,"SENDID aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa \"Work:Shared \"\"Old\"\"/a%b.txt\""));
        assert(!nd_bitskim_send_command(out,20,session,"a") && !nd_bitskim_send_command(out,sizeof(out),"bad","a"));
        assert(!nd_bitskim_send_command(out,sizeof(out),session,"bad\npath"));
        assert(nd_bitskim_parse(&s,"ACTIVE=1 INCOMING=0 AWAITING=0 ICONIFIED=0 RECEIVING=1 VISIBLE=1 PEERS=0 BYTES=50 TOTAL=100 BPS=2048 FUTURE=4294967295"));
        nd_bitskim_display(&s,value,&meter,badge,1);assert(!strcmp(value,"SEND 50% 2K/s"));
        assert(!nd_bitskim_parse(&s,"ACTIVE=1 INCOMING=0 AWAITING=0 ICONIFIED=0 RECEIVING=1 VISIBLE=1 PEERS=0 BYTES=50 TOTAL=100 BPS=4294967296"));
    }
    puts("BitSkim STATUS parsing, port discovery names, badges and large-file progress passed.");return 0;
}
