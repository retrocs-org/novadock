#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_network.h"
int main(void)
{
    struct NDNetConfig c;
    unsigned long rx, tx;
    nd_net_defaults(&c);
    assert(!c.device[0]);
    assert(c.scale_pps == 250UL && c.poll_ticks == 50UL);
    assert(nd_net_parse_line(&c, "DEVICE=DEVS:Networks/x.device \n"));
    assert(strcmp(c.device, "DEVS:Networks/x.device") == 0);
    assert(nd_net_parse_line(&c, "UNIT=2"));
    assert(c.unit == 2);
    assert(!nd_net_parse_line(&c, "UNIT=-1"));
    assert(!nd_net_parse_line(&c, "UNIT=999"));
    assert(nd_net_parse_line(&c, "SCALE_PPS=500"));
    assert(!nd_net_parse_line(&c, "SCALE_PPS=0"));
    assert(nd_net_parse_line(&c, "POLL_TICKS=25"));
    assert(!nd_net_parse_line(&c, "POLL_TICKS=0"));
    assert(!nd_net_parse_line(&c, "BOGUS=3"));
    assert(nd_net_load_config(&c, "file-does-not-exist.cfg") == -1);
    assert(nd_net_rates(100, 200, 125, 210, 50, &rx, &tx));
    assert(rx == 25 && tx == 10);
    assert(nd_net_rates(100, 200, 125, 210, 25, &rx, &tx));
    assert(rx == 50 && tx == 20);
    assert(nd_net_rates(0xfffffffeUL, 0xfffffffcUL, 2, 4, 50, &rx, &tx));
    assert(rx == 4 && tx == 8);
    assert(!nd_net_rates(1000, 1000, 0, 0, 50, &rx, &tx));
    assert(!nd_net_rates(100, 100, 200, 200, 0, &rx, &tx));
    assert(!nd_net_rates(100, 100, 200, 200, 5001, &rx, &tx));
    assert(nd_net_meter(25, 50, 250) == 20);
    assert(nd_net_meter(500, 5, 250) == 100);
    assert(nd_net_meter(0, 0, 250) == 0);
    assert(nd_net_channel_meter(25, 250) == 10);
    assert(nd_net_channel_meter(50, 250) == 20);
    assert(nd_net_channel_meter(250, 250) == 100);
    assert(nd_net_channel_meter(1000, 250) == 100);
    puts("Network config and packet-rate tests passed.");
    return 0;
}
