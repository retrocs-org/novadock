/* Plugin-owned artwork example. Clicking toggles a local demo state only. */
#include <string.h>
#include "nd_client.h"
const char nd_version[]="$VER: ArtHello 1.0 (17.11.2026)";
long __stack=16384;
int main(int argc,char **argv)
{
 struct NDClient c;unsigned int event,elapsed=0;int active=0;
 if(argc<2 || !nd_client_init(&c,argv[1]))return 20;
 for(;;){
  event=nd_client_update_art_quiet(&c,"ART DEMO",active ? "On":"Off","lamp",active,elapsed,500U);
  if(event&ND_EVENT_QUIT)break;
  if(event&ND_EVENT_CLICK)active=!active;
  elapsed=nd_client_wait(&c,50U);
 }
 nd_client_close(&c);return 0;
}
