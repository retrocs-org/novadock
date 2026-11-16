#include <assert.h>
#include <stdio.h>
#include "nd_music_formats.h"
#include "nd_games_filter.h"
int main(void)
{
 unsigned long all=(1UL<<ND_MUSIC_FORMAT_COUNT)-1;int i;
 const char *names[]={"tune.MP3","mod.Enigma","Enigma.mod","song.XM","s3m.intro","song.it","med.demo","song.MMD2","Monty.sid","SID.Monty","tune.psid","tune.RSID","audio.wav","audio.AIFF","audio.aif","audio.ogg","audio.oga","audio.flac"};
 assert(ND_MUSIC_FORMAT_COUNT==11 && nd_music_format_index("SID")==6);
 assert(nd_music_file_matches("tune.mp3",nd_music_default_mask()));
 assert(nd_music_default_mask()==all && nd_music_file_matches("tune.sid",nd_music_default_mask()));
 for(i=0;i<(int)(sizeof(names)/sizeof(names[0]));++i){assert(nd_music_file_matches(names[i],all));assert(!nd_music_file_matches(names[i],0));}
 assert(nd_music_file_matches("sid.Tune",1UL<<6));assert(!nd_music_file_matches("tune.mp3",1UL<<6));
 assert(!nd_music_file_matches("sid.Tune.info",all) && !nd_music_file_matches("song.mp3.backup",all));
 assert(!nd_music_file_matches("README",all) && !nd_music_file_matches("mod.",all));
 assert(nd_games_matches("A Great Demo 1992","demo great"));assert(nd_games_matches("A Great Demo 1992","  GREAT \t199"));
 assert(nd_games_matches("Title","") && nd_games_matches("Title","  \t"));
 assert(!nd_games_matches("A Great Demo","demo music") && !nd_games_matches("Short","longer"));
 puts("Music formats including SID, format selection and live title filtering passed.");return 0;
}
