#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_prism.h"
static void hex(const unsigned char *p,char out[65])
{unsigned int i;for(i=0;i<32;++i)sprintf(out+2*i,"%02x",p[i]);}
int main(void)
{
 unsigned char key[32];char digest[65],profile[2048],again[2048];
 struct NDPrismScan scan;char longname[34];int i;
 assert(nd_prism_psk("IEEE","password",key,NULL));hex(key,digest);
 assert(!strcmp(digest,"f42c6fc52df0ebef9ebb4b90b38a5f902e83fe1b135a70e23aed762e9710a12e"));
 assert(nd_prism_psk("ThisIsASSID","ThisIsAPassword",key,NULL));hex(key,digest);
 assert(!strcmp(digest,"0dc0d6eb90555ed6419756b9a15ec3e3209b63df707dd508d14581f8982721af"));
 assert(!nd_prism_credentials("wifi","short",0));assert(nd_prism_credentials("wifi","",1));
 assert(!nd_prism_credentials("\n","password",0));memset(longname,'a',33);longname[33]=0;assert(!nd_prism_credentials(longname,"password",0));
 assert(nd_prism_profile("# retained\nnetwork={\n ssid=\"old\"\n psk=\"old-pass\"\n}\n","new\"\\$",key,0,profile,sizeof(profile)));
 assert(strstr(profile,"# retained\nnetwork={\n ssid=\"old\"\n psk=\"old-pass\"\n}\n"));
 assert(strstr(profile,"ssid=6e6577225c24\n"));assert(strstr(profile,digest));assert(!strstr(profile,"ThisIsAPassword"));
 assert(nd_prism_profile(profile,"replacement",key,1,again,sizeof(again)));
 assert(strstr(again,"key_mgmt=NONE\n"));assert(!strstr(again,"ssid=6e6577225c24"));
 assert(strstr(again,"# retained"));assert(!nd_prism_profile("# NovaDock Wi-Fi profile begin\nbroken","wifi",key,0,again,sizeof(again)));
 assert(!nd_prism_profile("# NovaDock Wi-Fi profile end\n","wifi",key,0,again,sizeof(again)));
 assert(!nd_prism_profile("","wifi",key,0,again,20));
 memset(&scan,0,sizeof(scan));nd_prism_add_network(&scan,"weak",-80,3);nd_prism_add_network(&scan,"strong",-30,1);
 assert(scan.count==2 && !strcmp(scan.networks[0].ssid,"strong"));nd_prism_add_network(&scan,"weak",-20,2);
 assert(scan.count==2 && !strcmp(scan.networks[0].ssid,"weak") && scan.networks[0].channel==2);
 nd_prism_add_network(&scan,"weak",-90,4);assert(scan.networks[0].signal==-20);
 nd_prism_add_network(&scan,"bad\nname",-50,3);assert(scan.count==2);
 for(i=0;i<40;++i){sprintf(longname,"network-%d",i);nd_prism_add_network(&scan,longname,-90,1);}
 assert(scan.count==32);nd_prism_add_network(&scan,"late-strongest",-10,4);
 assert(!strcmp(scan.networks[0].ssid,"late-strongest") && scan.count==32);
 return 0;
}
