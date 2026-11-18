#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_alerts.h"
#include "nd_volume.h"
#include "nd_shelf.h"
#include "nd_i2c_cache.h"
#include "nd_config.h"
#include "nd_options.h"
static FILE *scratch(void){return fopen("feature-scratch.tmp","w+");}
static struct NDConfig config,restored;
static struct NDAlerts alerts;
static struct NDShelf shelf,copy;
static struct NDI2CCache sensors,reordered;
int main(void)
{
 FILE *f;unsigned int i;char text[128],key[16];struct NDVolumeSpace volume;int index;
 volume.total=1000;volume.used=910;volume.block_bytes=512;
 assert(nd_volume_valid(&volume) && nd_volume_low(&volume,10));assert(!nd_volume_low(&volume,0));volume.used=900;assert(!nd_volume_low(&volume,10));
 nd_volume_summary(&volume,text);assert(strstr(text,"Free ") && strstr(text,"Used ") && strstr(text,"Total "));
 nd_volume_size(8388608UL,512,text);assert(!strcmp(text,"4.0 GB"));nd_volume_size(0xffffffffUL,512,text);assert(strstr(text,"TB"));nd_volume_size(0xffffffffUL,0xffffffffUL,text);assert(strstr(text,"EB"));
 volume.used=1001;assert(!nd_volume_valid(&volume));nd_volume_summary(&volume,text);assert(!strcmp(text,"Capacity unavailable"));
 nd_alerts_add(&alerts,"TIMER","Finished","hello\nworld",0,100);assert(alerts.count==1 && alerts.unread==1 && !strcmp(alerts.entries[0].text,"helloworld"));
 nd_alerts_add(&alerts,"TIMER","Finished","hello\nworld",0,105);assert(alerts.count==1);nd_alerts_ack(&alerts,alerts.sequence);assert(!alerts.unread);
 for(i=0;i<80;++i){sprintf(text,"Result %u",i);nd_alerts_add(&alerts,"DOWNLOAD","Saved",text,0,200+i);}
 assert(alerts.count==64 && alerts.unread==64);nd_alerts_ack(&alerts,alerts.sequence-1);assert(alerts.unread==1);nd_alerts_clear(&alerts);assert(!alerts.count && !alerts.unread && alerts.sequence==81);
 assert(nd_shelf_id("SHELF_123") && !nd_shelf_id("SHELF_other") && !nd_shelf_id("OTHER"));
 assert(nd_shelf_add(&shelf,"Work:Documents/readme"));assert(nd_shelf_add(&shelf,"work:documents/README") && shelf.count==1);assert(!nd_shelf_add(&shelf,"relative-file"));assert(!nd_shelf_add(&shelf,"Work:bad\nname"));assert(nd_shelf_add(&shelf,"RAM:folder"));
 assert(nd_shelf_move(&shelf,1,-1) && !strcmp(shelf.paths[0],"RAM:folder"));assert(!nd_shelf_move(&shelf,0,-1));
 assert(nd_shelf_save("shelf-test.dat",&shelf));f=fopen("shelf-test.dat","r");assert(f && nd_shelf_read(f,&copy));fclose(f);assert(copy.count==2 && !strcmp(copy.paths[0],"RAM:folder"));
 assert(nd_shelf_remove(&copy,0) && copy.count==1);assert(nd_shelf_save("shelf-test.dat",&copy));f=fopen("shelf-test.dat.bak","r");assert(f && nd_shelf_read(f,&copy) && copy.count==2);fclose(f);
 f=scratch();assert(f);fputs("NDSHELF1\nWork:ok\ntruncated",f);rewind(f);assert(!nd_shelf_read(f,&copy) && copy.count==2);fclose(f);
 sensors.provider=1;sensors.stamp=1000;assert(nd_i2c_add(&sensors,123,0,"Zero sensor","C",1000));assert(nd_i2c_add(&sensors,456,-98304L,"Cold sensor","C",1000));assert(!nd_i2c_add(&sensors,789,42,NULL,NULL,1000));assert(sensors.count==2 && sensors.readings[0].available);
 nd_i2c_format(&sensors.readings[1],text);assert(!strcmp(text,"-1.5 C"));nd_i2c_option(456,key);assert(nd_i2c_key(key));assert(!nd_i2c_key("I2C_00000000") && !nd_i2c_key("I2C_0000000G"));assert(nd_i2c_fresh(&sensors,1060,60) && !nd_i2c_fresh(&sensors,1061,60) && !nd_i2c_fresh(&sensors,999,60));
 f=scratch();assert(f && nd_i2c_write(f,&sensors));rewind(f);assert(nd_i2c_read(f,&reordered));fclose(f);assert(reordered.readings[1].id==456 && reordered.readings[1].milli==-1500);
 f=scratch();assert(f);fputs("NDI2C1 1000 1 2\n123 1000 0 1|One|C\n123 1000 0 1|Duplicate|C\n",f);rewind(f);assert(!nd_i2c_read(f,&reordered) && reordered.count==2);fclose(f);
 nd_default_config(&config);assert(nd_add_launcher(&config,ND_ITEM_DRAWER,"Work","Work:",""));index=config.count-1;config.items[index].volume_warning=15;config.items[index].volume_show_free=1;
 assert(nd_parse_line(&config,"PLUGIN=SHELF|Plugins/Shelf|SHELF|METER|ON|YES"));assert(nd_option_set(&config,"SHELF","LABEL","Tools"));assert(nd_add_plugin_instance(&config,config.count-1));
 assert(!strcmp(nd_option_schema_id(&config,"SHELF_2"),"SHELF"));assert(nd_option_set(&config,"SHELF_2","LABEL","Documents"));assert(!strcmp(nd_option_get(&config,"SHELF","LABEL"),"Tools"));
 assert(nd_parse_line(&config,"PLUGIN=SENSOR|Plugins/Sensors|SENSORS|METER|ON|YES"));assert(nd_option_set(&config,"SENSORS",key,"NO"));assert(!nd_option_set(&config,"SENSORS","I2C_bad","YES"));
 assert(nd_write_config(&config,"features-test.cfg"));assert(!nd_load_config(&restored,"features-test.cfg"));assert(restored.items[index].volume_warning==15 && restored.items[index].volume_show_free);assert(!strcmp(nd_option_get(&restored,"SENSORS",key),"NO"));assert(!strcmp(nd_option_get(&restored,"SHELF_2","LABEL"),"Documents"));
 puts("Volume arithmetic/settings, bounded alerts, permanent shelves and stable I2C selection passed.");return 0;
}
