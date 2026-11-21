#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_agenda.h"
#include "nd_pictures.h"
static int probe(unsigned char *bytes,unsigned int n,unsigned int w,unsigned int h)
{FILE *f=tmpfile();unsigned int x=0,y=0;int ok;assert(f);assert(fwrite(bytes,1,n,f)==n);ok=nd_picture_probe(f,&x,&y);fclose(f);return ok&&x==w&&y==h;}
int main(void)
{struct NDAgenda a,b;struct NDThumbnail thumb;struct NDPictureImage image;unsigned char bytes[64],rgb[3]={255,255,255};long day,next;unsigned int minute;FILE *f;memset(&a,0,sizeof(a));
 assert(nd_agenda_date("2026-11-21",&day));assert(!nd_agenda_date("2027-02-29",&next));assert(nd_agenda_date("2028-02-29",&next));assert(!nd_agenda_date("2026-11- 1",&next));assert(!nd_agenda_time("24:00",&minute));assert(!nd_agenda_time(" 1:00",&minute));
 assert(nd_agenda_put(&a,-1,"Daily task","2026-11-21","12:30","DAILY"));assert(nd_agenda_due(&a.items[0],day));assert(!nd_agenda_remind(&a.items[0],day,749));assert(nd_agenda_remind(&a.items[0],day,750));a.items[0].notified=day;assert(!nd_agenda_remind(&a.items[0],day+1,900));assert(nd_agenda_complete(&a,0,day)&&a.items[0].day==day+1&&a.items[0].notified==-1);
 assert(nd_agenda_put(&a,-1,"Month end","2027-01-31","08:00","MONTHLY"));assert(nd_agenda_date("2027-01-31",&day));assert(nd_agenda_complete(&a,1,day));assert(nd_agenda_date("2027-02-28",&day)&&a.items[1].day==day);assert(nd_agenda_complete(&a,1,day));assert(nd_agenda_date("2027-03-31",&day)&&a.items[1].day==day);
 assert(nd_agenda_put(&a,-1,"Weekly","2026-11-01","00:00","WEEKLY"));assert(nd_agenda_date("2026-11-21",&day));assert(nd_agenda_complete(&a,2,day));assert(nd_agenda_date("2026-11-22",&next)&&a.items[2].day==next);
 assert(nd_agenda_put(&a,-1,"Undated","","","NONE"));assert(!nd_agenda_due(&a.items[3],day)&&nd_agenda_complete(&a,3,day)&&a.items[3].done);assert(!nd_agenda_put(&a,-1,"bad|title","","","NONE"));assert(!nd_agenda_put(&a,-1,"No date repeating","","","DAILY"));
 assert(nd_agenda_save("agenda",&a));f=fopen("agenda","rb");assert(f&&nd_agenda_read(f,&b));fclose(f);assert(b.count==4&&b.items[1].anchor==31);assert(nd_agenda_remove(&a,0)&&nd_agenda_save("agenda",&a));f=fopen("agenda.bak","rb");assert(f&&nd_agenda_read(f,&b)&&b.count==4);fclose(f);
 f=fopen("agenda","wb");assert(f);fputs("Damaged original\n",f);fclose(f);assert(!nd_agenda_save("agenda",&a));f=fopen("agenda","rb");assert(f);assert(fgets((char *)bytes,sizeof(bytes),f)&&!strcmp((char *)bytes,"Damaged original\n"));fclose(f);
 f=tmpfile();assert(f);fputs("NDAGENDA1|1\n1|0|0|0|0|1|-1|First\n1|0|0|0|0|1|-1|Duplicate\n",f);rewind(f);assert(!nd_agenda_read(f,&b));fclose(f);
 assert(nd_picture_name("Family.JPG")&&nd_picture_name("image.ilbm")&&!nd_picture_name("C:bad.jpg")&&!nd_picture_name("../bad.png")&&!nd_picture_name("run.exe"));
 memset(&image,0,sizeof(image));nd_picture_palette(image.rgb);assert(nd_picture_color(rgb,image.rgb)==6);image.pixels[0]=6;nd_picture_thumbnail(&image,&thumb);assert(nd_thumbnail_valid(&thumb)&&thumb.pixels[4*32]==6);thumb.pixels[0]=16;assert(!nd_thumbnail_valid(&thumb));memset(&thumb,0,sizeof(thumb));assert(nd_thumbnail_valid(&thumb));thumb.width=31;assert(!nd_thumbnail_valid(&thumb));
 memset(bytes,0,sizeof(bytes));memcpy(bytes,"\x89PNG\r\n\x1a\n",8);bytes[11]=13;memcpy(bytes+12,"IHDR",4);bytes[18]=2;bytes[22]=1;assert(probe(bytes,64,512,256));bytes[16]=1;assert(!probe(bytes,64,512,256));
 memset(bytes,0,64);memcpy(bytes,"GIF89a",6);bytes[6]=64;bytes[8]=32;assert(probe(bytes,64,64,32));bytes[7]=16;assert(!probe(bytes,64,64,32));
 memset(bytes,0,64);memcpy(bytes,"FORM",4);memcpy(bytes+8,"ILBMBMHD",8);bytes[19]=20;bytes[20]=1;bytes[21]=64;bytes[22]=0;bytes[23]=200;assert(probe(bytes,64,320,200));
 memset(bytes,0,64);bytes[0]=0xff;bytes[1]=0xd8;bytes[2]=0xff;bytes[3]=0xc0;bytes[5]=11;bytes[6]=8;bytes[7]=0;bytes[8]=240;bytes[9]=1;bytes[10]=64;assert(probe(bytes,64,320,240));bytes[9]=5;assert(!probe(bytes,64,320,240));
 puts("AGENDA: recurrence, calendar/time validation, reminder deduplication, persistence, backups and corrupt-file preservation passed.");puts("PICTURES: safe filenames, indexed previews, thumbnail validation and bounded PNG/GIF/ILBM/JPEG header preflight passed.");return 0;}
