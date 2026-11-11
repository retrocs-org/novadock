#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_roadshow.h"

static void write_file(const char *path, const char *data)
{
    FILE *f = fopen(path, "w");
    assert(f != NULL);
    assert(fputs(data, f) >= 0);
    assert(fclose(f) == 0);
}
int main(void)
{
    struct RDRoadshowConfig c, saved;
    char cmd[192];
    const char *name = "tests/rd_valid.cfg";
    rd_config_defaults(&c);
    assert(c.poll_ticks == 250 && c.dhcp_timeout == 60);
    assert(c.ping_count == 4 && !strcmp(c.ping_host, "1.1.1.1"));
    assert(!c.allow_shutdown);
    assert(rd_valid_interface("eth0") && rd_valid_interface("3c589_1"));
    assert(!rd_valid_interface("") && !rd_valid_interface("../bad"));
    assert(!rd_valid_interface("a;NetShutdown"));
    assert(!rd_valid_interface("space name"));
    assert(!rd_valid_interface("loooooooooooooong"));
    assert(rd_interface_filename("Ariadne-II"));
    assert(!rd_interface_filename("Roadshow.info"));
    assert(!rd_interface_filename("../fake"));
    assert(rd_valid_host("amiga.example.org") && rd_valid_host("192.168.1.1"));
    assert(!rd_valid_host("") && !rd_valid_host("-a"));
    assert(!rd_valid_host("a;NetShutdown") && !rd_valid_host("a b"));
    assert(!rd_valid_host("a&&b") && !rd_valid_host("a\nb"));
    assert(!rd_valid_host(".evil") && !rd_valid_host("foo..bar"));
    assert(!rd_command(&c, RD_CMD_CONNECT, cmd, sizeof(cmd)));
    assert(rd_command(&c, RD_CMD_STATUS, cmd, sizeof(cmd)));
    assert(!strcmp(cmd, "C:GetNetStatus CHECK=INTERFACES QUIET"));
    assert(rd_command(&c, RD_CMD_DETAILS, cmd, sizeof(cmd)));
    assert(!strcmp(cmd, "C:ShowNetStatus INTERFACES ROUTES DNS"));
    assert(!rd_command(&c, RD_CMD_SHUTDOWN, cmd, sizeof(cmd)));
    strcpy(c.interface_name, "eth0");
    assert(rd_command(&c, RD_CMD_CONNECT, cmd, sizeof(cmd)));
    assert(!strcmp(cmd, "C:AddNetInterface eth0 QUIET TIMEOUT=60"));
    assert(rd_command(&c, RD_CMD_DISCONNECT, cmd, sizeof(cmd)));
    assert(!strcmp(cmd, "C:RemoveNetInterface eth0"));
    assert(rd_command(&c, RD_CMD_DHCP, cmd, sizeof(cmd)));
    assert(!strcmp(cmd, "C:ConfigureNetInterface eth0 CONFIGURE=DHCP TIMEOUT=60 QUIET"));
    assert(rd_command(&c, RD_CMD_PING, cmd, sizeof(cmd)));
    assert(!strcmp(cmd, "C:Ping -c 4 1.1.1.1"));
    assert(!rd_command(&c, RD_CMD_PING, cmd, 6));
    assert(!rd_command(&c, RD_CMD_CONNECT, cmd, 6));
    assert(!rd_command(&c, RD_CMD_SHUTDOWN, cmd, sizeof(cmd)));
    c.allow_shutdown = 1;
    assert(rd_command(&c, RD_CMD_SHUTDOWN, cmd, sizeof(cmd)));
    assert(!strcmp(cmd, "C:NetShutdown TIMEOUT=5 QUIET"));
    assert(rd_save_config(&c, name));
    assert(rd_load_config(&saved, name));
    assert(!memcmp(&saved, &c, sizeof(c)));
    assert(rd_save_config(&saved, name));
    assert(rd_load_config(&c, name));
    remove(name);
    remove("tests/rd_valid.cfg.bak");
    write_file(name, "configure=dhcp\n#ignore=anything\n");
    assert(rd_interface_uses_dhcp(name));
    write_file(name, "#configure=dhcp\naddress=192.168.1.20\n");
    assert(!rd_interface_uses_dhcp(name));
    write_file(name, "  CoNfIgUrE  =  DhCp   \n");
    assert(rd_interface_uses_dhcp(name));
    write_file(name, "INTERFACE=lan;NetShutdown\n");
    assert(!rd_load_config(&saved, name));
    write_file(name, "INTERFACE=lan\nPING_HOST=a;bad\n");
    assert(!rd_load_config(&saved, name));
    write_file(name, "INTERFACE=lan\nPING_COUNT=0\n");
    assert(!rd_load_config(&saved, name));
    write_file(name, "INTERFACE=lan\nALLOW_SHUTDOWN=MAYBE\n");
    assert(!rd_load_config(&saved, name));
    write_file(name, "INTERFACE=lan\nDHCP_TIMEOUT=0\n");
    assert(!rd_load_config(&saved, name));
    write_file(name, "INTERFACE=lan\nWHAT=bad\n");
    assert(!rd_load_config(&saved, name));
    remove(name);
    assert(!rd_load_config(&saved, "tests/roadshow-missing.cfg"));
    puts("Roadshow command validation, DHCP guard and prefs round-trip tests passed.");
    return 0;
}
