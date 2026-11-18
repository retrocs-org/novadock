#include <assert.h>
#include <string.h>
#include "nd_warp_control.h"
int main(void)
{
 unsigned char d[428],packet[10];char secret[64]="",work[64]="a",longpass[66];int i;
 assert(nd_warp_supported(30,74));assert(!nd_warp_supported(30,73));assert(!nd_warp_supported(31,74));
 memset(d,0,sizeof(d));strcpy((char *)d,"Warp1260");assert(nd_warp_1260(d,428));
 strcpy((char *)d,"warp1260");assert(nd_warp_1260(d,428));
 strcpy((char *)d,"Warp1240");assert(!nd_warp_1260(d,428));strcpy((char *)d,"Warp1260X");assert(!nd_warp_1260(d,428));
 assert(!nd_warp_1260(d,427));
 assert(nd_warp_wifi_valid("mywifi","pass \"$*;word"));assert(!nd_warp_wifi_valid("","password"));
 memset(longpass,'a',65);longpass[65]=0;assert(!nd_warp_wifi_valid("wifi",longpass));longpass[63]=0;assert(nd_warp_wifi_valid("wifi",longpass));
 assert(!nd_warp_wifi_valid("wi\nfi","password"));assert(!nd_warp_wifi_valid("wifi",""));
 for(i=0;i<8;++i){assert(nd_warp_turbo_packet(i,packet));assert(packet[4]==0 && packet[8]==0 && packet[9]==1);
  memcpy(d+382,packet,4);assert(nd_warp_frequency_khz(d,428)>=50000 && nd_warp_frequency_khz(d,428)<=105000);}
 assert(nd_warp_turbo_packet(1,packet));memcpy(d+382,packet,4);assert(nd_warp_frequency_khz(d,428)==66600);
 assert(!nd_warp_turbo_packet(-1,packet));assert(!nd_warp_turbo_packet(8,packet));
 memset(d+382,255,4);assert(!nd_warp_frequency_khz(d,428));
 assert(nd_warp_password_edit(secret,work,0,1,0,1,8));assert(!strcmp(secret,"a") && !strcmp(work,"*"));
 strcpy(work,"*b");assert(nd_warp_password_edit(secret,work,1,2,1,2,8));assert(!strcmp(secret,"ab") && !strcmp(work,"**"));
 strcpy(work,"c**");assert(nd_warp_password_edit(secret,work,0,1,2,3,8));assert(!strcmp(secret,"cab") && !strcmp(work,"***"));
 strcpy(work,"*d*");assert(nd_warp_password_edit(secret,work,1,2,3,3,7));assert(!strcmp(secret,"cdb"));
 strcpy(work,"**");assert(nd_warp_password_edit(secret,work,2,1,3,2,2));assert(!strcmp(secret,"cb"));
 strcpy(work,"*");assert(nd_warp_password_edit(secret,work,1,1,2,1,3));assert(!strcmp(secret,"c"));
 assert(nd_warp_password_edit(secret,work,1,0,1,1,4));assert(!strcmp(secret,"c"));
 strcpy(work,"\n*");assert(!nd_warp_password_edit(secret,work,0,1,1,2,8));assert(!strcmp(secret,"c"));
 assert(nd_warp_password_edit(secret,work,1,0,1,0,12));assert(!secret[0] && !work[0]);
 memset(secret,'p',63);secret[63]=0;memset(work,'*',63);work[63]=0;
 assert(nd_warp_password_edit(secret,work,63,63,63,63,1));
 assert(!nd_warp_password_edit(secret,work,63,64,63,64,8));
 assert(nd_warp_password_edit(secret,work,63,0,63,0,6));
 for(i=0;i<64;++i)assert(secret[i]==0);
 return 0;
}
