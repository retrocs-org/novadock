/* Metadata, scheduling and archive integrity regressions: no network or UI. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_update.h"
#include "nd_update_hash.h"
#include "nd_config.h"
#include "nd_layout_profiles.h"
static const char *hex="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
static void metadata(char *out,const char *tag,const char *version,const char *flags,const char *origin,const char *digest)
{
 sprintf(out,"{%s,\"body\":\"\\\"draft\\\": false fake assets\",\"tag_name\":\"%s\",\"assets\":["
  "{\"name\":\"NovaDock-%s.lha\",\"state\":\"uploaded\",\"size\":3,\"digest\":%s,\"browser_download_url\":\"%s/%s/NovaDock-%s.lha\"},"
  "{\"name\":\"NovaDock-%s.lha.sha256\",\"state\":\"uploaded\",\"browser_download_url\":\"%s/%s/NovaDock-%s.lha.sha256\"}]}",flags,tag,version,digest,origin,tag,version,version,origin,tag,version);
}
static void hash(const char *input,const char *expected)
{
 struct NDUpdateHash h;char out[65];nd_update_hash_init(&h);
 nd_update_hash_add(&h,(const unsigned char *)input,(unsigned int)strlen(input));
 nd_update_hash_end(&h,out);assert(!strcmp(out,expected));
}
int main(int argc,char **argv)
{
 const char *origin="https://github.com/retrocs-org/novadock/releases/download";
 const char *flags="\"draft\":false,\"prerelease\":false";
 struct NDUpdateRelease r,sentinel;struct NDUpdateState state;struct NDUpdateSchedule schedule;
 struct NDUpdateHash h;struct NDConfig cfg,other;char json[4000],copy[4000],digest[80],sum[170],out[65],a[1000];
 unsigned int i,j;unsigned long now=1000000UL;FILE *f;
 assert(nd_update_compare("0.10","0.4")==1);
 assert(nd_update_compare("v0.5","0.4")==1);
 assert(nd_update_compare("0.4.0","0.4")==0);
 assert(nd_update_compare("0.4.0.1","0.4")==1);
 assert(nd_update_compare("0.3","0.4")==-1);
 assert(nd_update_compare("0.4-rc1","0.3")==2);
 assert(nd_update_compare("1","0.4")==2);
 assert(nd_update_compare("0.65536","0.4")==2);
 assert(nd_update_compare("0.1.2.3.4","0.4")==2);
 assert(nd_update_compare(NULL,"0.4")==2);
 sprintf(digest,"\"sha256:%s\"",hex);
 metadata(json,"0.5","0.5",flags,origin,digest);
 assert(nd_update_release(json,&r));assert(!strcmp(r.version,"0.5") && r.size==3 && !strcmp(r.digest,hex));
 metadata(json,"v0.5","0.5",flags,origin,"null");assert(nd_update_release(json,&r));assert(!r.digest[0]);
 memset(&sentinel,42,sizeof(sentinel));
 metadata(json,"0.5","0.5","\"draft\":true,\"prerelease\":false",origin,"null");r=sentinel;assert(!nd_update_release(json,&r));assert(!memcmp(&r,&sentinel,sizeof(r)));
 metadata(json,"0.5","0.5","\"draft\":false,\"prerelease\":true",origin,"null");assert(!nd_update_release(json,&r));
 metadata(json,"0.5","0.5","\"draft\":false,\"draft\":false,\"prerelease\":false",origin,"null");assert(!nd_update_release(json,&r));
 metadata(json,"0.5","0.5","\"draft\":false",origin,"null");assert(!nd_update_release(json,&r));
 metadata(json,"0.5","0.5",flags,"https://evil.example/releases/download","null");assert(!nd_update_release(json,&r));
 metadata(json,"0.5-rc1","0.5-rc1",flags,origin,"null");assert(!nd_update_release(json,&r));
 metadata(json,"0.5","0.5",flags,origin,"\"sha256:1234\"");assert(!nd_update_release(json,&r));
 metadata(json,"0.5","0.5",flags,origin,digest);
 /* Every truncation and single-byte mutation must remain memory-safe. */
 for(i=0;i<strlen(json);++i){memcpy(copy,json,i);copy[i]=0;assert(!nd_update_release(copy,&r));}
 for(i=0;i<strlen(json);++i)for(j=0;j<3;++j){strcpy(copy,json);copy[i]=(char)(j ? (j==1 ? '\\':'"'):'[');(void)nd_update_release(copy,&r);}
 strcpy(copy,json);strcat(copy," junk");assert(!nd_update_release(copy,&r));
 assert(!nd_update_release("{\"draft\":false,\"prerelease\":false,\"assets\":[]}",&r));
 assert(!nd_update_release("{\"x\":1e+}",&r));
 sprintf(sum,"%s  NovaDock-0.5.lha\n",hex);assert(nd_update_checksum(sum,"NovaDock-0.5.lha",out));assert(!strcmp(out,hex));
 assert(!nd_update_checksum(sum,"NovaDock-0.6.lha",out));
 strcat(sum,"extra");assert(!nd_update_checksum(sum,"NovaDock-0.5.lha",out));
 for(i=0;i<66;++i){memset(sum,'a',i);sum[i]=0;assert(!nd_update_checksum(sum,"NovaDock-0.5.lha",out));}
 assert(nd_update_interval(2)==7200UL && nd_update_interval(4)==14400UL && nd_update_interval(6)==21600UL && nd_update_interval(24)==14400UL);
 memset(&schedule,0,sizeof(schedule));assert(!nd_update_schedule(&schedule,0,0,4,1,0,now,50));assert(!schedule.enabled);
 /* Startup only waits for network, runs once, and never becomes periodic. */
 assert(!nd_update_schedule(&schedule,1,0,4,0,0,now,50));assert(schedule.startup);
 assert(nd_update_schedule(&schedule,1,0,4,1,0,now,3000)==1);assert(!schedule.startup && schedule.waiting);
 schedule.waiting=0;assert(!nd_update_schedule(&schedule,1,0,4,1,0,now+ND_UPDATE_DAY,3000));
 assert(!nd_update_schedule(&schedule,0,0,4,1,0,now,3000));assert(!schedule.enabled && !schedule.next_due);
 /* Running only never performs an implicit startup check. */
 for(i=2;i<=6;i+=2){
  memset(&schedule,0,sizeof(schedule));
  assert(!nd_update_schedule(&schedule,0,1,i,1,0,now,0));
  assert(!nd_update_schedule(&schedule,0,1,i,1,0,now+nd_update_interval(i)-1,3000));
  assert(!nd_update_schedule(&schedule,0,1,i,0,0,now+nd_update_interval(i),3000));
  assert(!nd_update_schedule(&schedule,0,1,i,1,1,now+nd_update_interval(i)+60,3000));
  assert(nd_update_schedule(&schedule,0,1,i,1,0,now+nd_update_interval(i)+120,3000)==2);
 }
 memset(&schedule,0,sizeof(schedule));assert(nd_update_schedule(&schedule,1,1,2,1,0,now,0)==1);
 schedule.waiting=0;schedule.next_due=now+7200;
 assert(!nd_update_schedule(&schedule,1,1,2,1,0,now+7199,3000));
 assert(nd_update_schedule(&schedule,1,1,2,1,0,now+7200,3000)==2);
 /* Interval changes don't repeat the startup check; disabling stops all checks. */
 assert(!nd_update_schedule(&schedule,1,1,6,1,0,now+7201,0));assert(!schedule.startup && schedule.next_due==now+7201+21600);
 assert(!nd_update_schedule(&schedule,0,0,6,1,0,now+7202,0));
 memset(&state,0,sizeof(state));assert(nd_update_notice_due(&state,"0.5",now));
 strcpy(state.noticed,"0.5");state.last_notice=now;
 assert(!nd_update_notice_due(&state,"0.5",now+100));assert(nd_update_notice_due(&state,"0.6",now+100));
 assert(nd_update_notice_due(&state,"0.5",now+ND_UPDATE_DAY));assert(nd_update_notice_due(&state,"0.5",now-1));
 assert(nd_update_staging_dir("T:NovaDockUpdate-0012-abcd-0"));
 assert(nd_update_staging_dir("Work:Downloads/NovaDockUpdate-abcd-1"));
 assert(!nd_update_staging_dir("Work:Downloads"));assert(!nd_update_staging_dir("NovaDockUpdate-1"));
 assert(!nd_update_staging_dir("T:NovaDockUpdate-"));assert(!nd_update_staging_dir("T:NovaDockUpdate-../foo"));
 hash("","e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");hash("abc",hex);
 hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq","248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
 memset(a,'a',sizeof(a));nd_update_hash_init(&h);for(i=0;i<1000;++i)nd_update_hash_add(&h,(unsigned char *)a,sizeof(a));nd_update_hash_end(&h,out);
 assert(!strcmp(out,"cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));
 nd_default_config(&cfg);assert(!cfg.check_updates && !cfg.check_updates_running && cfg.update_interval_hours==4);other=cfg;
 assert(nd_parse_line(&cfg,"CHECK_UPDATES=YES")==1 && cfg.check_updates);
 assert(!nd_config_runtime_equal(&cfg,&other));assert(nd_config_can_update_live(&cfg,&other));
 assert(nd_parse_line(&cfg,"CHECK_UPDATES=MAYBE")==0 && cfg.check_updates);
 assert(cfg.check_updates_running); /* legacy combined YES migrates */
 assert(nd_parse_line(&cfg,"CHECK_UPDATES_STARTUP=NO") && !cfg.check_updates);
 assert(nd_parse_line(&cfg,"UPDATE_INTERVAL_HOURS=6") && cfg.update_interval_hours==6);
 assert(!nd_parse_line(&cfg,"UPDATE_INTERVAL_HOURS=3") && cfg.update_interval_hours==6);
 assert(!nd_parse_line(&cfg,"UPDATE_INTERVAL_HOURS=2junk"));
 assert(nd_parse_line(&cfg,"CHECK_UPDATES_STARTUP=YES"));
 assert(nd_write_config(&cfg,"update-test.cfg")==1);assert(nd_load_config(&other,"update-test.cfg")==0 && other.check_updates && other.check_updates_running && other.update_interval_hours==6);
 nd_default_config(&other);assert(nd_layout_apply(&cfg,&other) && cfg.check_updates && cfg.check_updates_running && cfg.update_interval_hours==6); /* layouts never opt in/out */
 assert(nd_parse_line(&cfg,"CHECK_UPDATES=NO")==1 && !cfg.check_updates);
 remove("update-test.cfg");remove("update-test.cfg.bak");remove("update-test.cfg.good");
 if(argc==2){long size;char *live;f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);size=ftell(f);rewind(f);live=malloc((size_t)size+1);assert(live);assert(fread(live,1,(size_t)size,f)==(size_t)size);live[size]=0;fclose(f);assert(nd_update_release(live,&r));printf("Live release %s: comparison with installed 0.4 = %d\n",r.version,nd_update_compare(r.version,"0.4"));free(live);}
 puts("Update metadata, opt-in, scheduling and SHA-256 tests passed.");return 0;
}
