#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_monitor.h"
#include "nd_sensor_status.h"
static struct NDMonitorHistory h;
static struct NDWatchSnapshot a,b;
static struct NDLogParser parser;
static struct NDSensors sensors;
static struct NDI2CCache i2c;
int main(void)
{
 unsigned int i;struct NDLogCursor c;unsigned char prefix[32];
 memset(&a,0,sizeof(a));memset(&b,0,sizeof(b));b.valid=1;b.count=1;strcpy(b.entries[0].name,"one");b.entries[0].size=4;
 assert(!nd_watch_changes(&h,&a,&b,"RAM:","01:00:00"));nd_watch_snapshot_copy(&a,&b);assert(a.count==1 && a.valid && a.entries[0].size==4);
 assert(!nd_watch_changes(&h,&a,&b,"RAM:","01:00:00"));
 strcpy(b.entries[0].name,"ONE");assert(!nd_watch_changes(&h,&a,&b,"RAM:","01:00:00"));
 b.entries[0].size=5;assert(nd_watch_changes(&h,&a,&b,"RAM:","01:00:00")==1);assert(!strcmp(h.rows[0].path,"RAM:ONE"));assert(strstr(h.rows[0].text,"changed"));
 b.count=2;strcpy(b.entries[1].name,"two");a=b;a.count=1;
 assert(nd_watch_changes(&h,&a,&b,"RAM:test","01:00:00")==1);assert(!strcmp(h.rows[1].path,"RAM:test/two"));
 b.valid=0;assert(!nd_watch_changes(&h,&a,&b,"RAM:",""));
 for(i=0;i<100;++i){char text[20];sprintf(text,"row %u",i);nd_monitor_add(&h,text,NULL,0,1);}
 assert(!strcmp(nd_monitor_row(&h,0)->text,"row 36") && !strcmp(nd_monitor_row(&h,63)->text,"row 99"));
 assert(h.count==64 && h.unseen==102);nd_monitor_read(&h);assert(!h.unseen);for(i=0;i<h.count;++i)assert(!h.rows[i].unread);
 memset(&h,0,sizeof(h));nd_log_reset(&parser,0);
 assert(!nd_log_feed(&h,&parser,(const unsigned char *)"normal\nER",9));assert(h.count==1);
 assert(nd_log_feed(&h,&parser,(const unsigned char *)"ROR test\r\n\033[33mWarning\033[0m\n",27)==2);assert(h.unseen==2);assert(strstr(h.rows[1].text,"[ERROR]"));assert(!strcmp(h.rows[2].text,"[WARN] Warning"));
 nd_log_reset(&parser,1);nd_log_feed(&h,&parser,(const unsigned char *)"error skipped\nOK\n",17);assert(h.count==4 && h.unseen==2);
 nd_log_reset(&parser,0);for(i=0;i<10000;++i)nd_log_feed(&h,&parser,(const unsigned char *)"a",1);nd_log_feed(&h,&parser,(const unsigned char *)"\n",1);assert(strlen(h.rows[4].text)<=159);
 memset(&c,0,sizeof(c));memset(prefix,1,sizeof(prefix));c.valid=1;c.offset=100;c.key=12;c.prefix_length=32;memcpy(c.prefix,prefix,32);
 assert(!nd_log_replaced(&c,12,120,prefix,32));assert(nd_log_replaced(&c,12,99,prefix,32));assert(nd_log_replaced(&c,13,120,prefix,32));prefix[2]=2;assert(nd_log_replaced(&c,12,120,prefix,32));
 memset(&c,0,sizeof(c));c.valid=1;c.key=12;c.prefix_length=2;memcpy(c.prefix,prefix,2);assert(!nd_log_replaced(&c,12,32,prefix,32));
 assert(!strcmp(nd_sensor_summary(&sensors,&i2c,0,0,0,100),"No channels selected"));
 i2c.stamp=100;assert(!strcmp(nd_sensor_summary(&sensors,&i2c,ND_SENSOR_ALL,~0UL,1,100),"I2C library missing"));
 sensors.available=1;sensors.stamp[0]=100;assert(!strcmp(nd_sensor_summary(&sensors,&i2c,1,0,0,100),"Ready"));
 assert(!strcmp(nd_sensor_summary(&sensors,&i2c,0,0,0,100),"No channels selected"));
 sensors.available=0;assert(!strcmp(nd_sensor_summary(&sensors,&i2c,1,0,0,101),"Device unavailable"));
 assert(!strcmp(nd_sensor_summary(&sensors,&i2c,1,0,0,200),"Stale readings"));
 assert(!strcmp(nd_sensor_reading_status(1,100,200,60),"Stale reading"));
 i2c.provider=1;i2c.count=1;i2c.stamp=100;i2c.readings[0].available=1;i2c.readings[0].stamp=100;
 assert(!strcmp(nd_sensor_summary(&sensors,&i2c,0,1,1,100),"Ready"));
 assert(!strcmp(nd_sensor_summary(&sensors,&i2c,0,1,1,200),"Stale readings"));
 puts("WATCH change detection, bounded LOGS parser/cursor and SENSOR states passed.");return 0;
}
