#include <stdio.h>
#include <string.h>
#include "nd_musicplayer.h"
static int failed = 0;
#define CHECK(c) do { if (!(c)) { ++failed; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
int main(void)
{
    struct NDMusicConfig cfg, saved;
    FILE *f;
    const char *path = "tests/music-config-test.tmp";
    nd_music_defaults(&cfg);
    CHECK(cfg.mode == ND_MUSIC_AUTO);
    CHECK(!strcmp(cfg.eagle.port, "rexx_EP"));
    CHECK(!strcmp(cfg.eagle.alt_port, "EAGLEPLAYER"));
    CHECK(!strcmp(cfg.eagle.commands[ND_MUSIC_PLAY], "PLAY 1"));
    CHECK(!strcmp(cfg.eagle.commands[ND_MUSIC_PAUSE], "PLAY 0"));
    CHECK(!strcmp(cfg.eagle.commands[ND_MUSIC_PREVIOUS], "PREVMODULE"));
    CHECK(!strcmp(cfg.eagle.commands[ND_MUSIC_NEXT], "NEXTMODULE"));
    CHECK(!strcmp(cfg.eagle.extras[ND_MUSIC_QUERY_TITLE], "STATUS M FIL"));
    CHECK(!strcmp(cfg.eagle.extras[ND_MUSIC_QUERY_VOLUME], "STATUS G VOL"));
    CHECK(!strcmp(cfg.eagle.extras[ND_MUSIC_SET_VOLUME], "VOLUME"));
    CHECK(cfg.eagle.volume_max == 255U);
    CHECK(!strcmp(cfg.amp.port, "AMIGAAMP"));
    CHECK(!strcmp(cfg.hippo.port,"HIPPOPLAYER"));
    CHECK(!strcmp(cfg.hippo.commands[ND_MUSIC_PLAY],"CONT"));
    CHECK(!strcmp(cfg.hippo.commands[ND_MUSIC_PAUSE],"STOP"));
    CHECK(!strcmp(cfg.hippo.commands[ND_MUSIC_PREVIOUS],"CHOOSEPREV"));
    CHECK(!strcmp(cfg.hippo.commands[ND_MUSIC_NEXT],"CHOOSENEXT"));
    CHECK(!strcmp(cfg.hippo.extras[ND_MUSIC_QUERY_TITLE],"GET NAME"));
    CHECK(!strcmp(cfg.hippo.extras[ND_MUSIC_QUERY_VOLUME],"GET VOLU"));
    CHECK(!strcmp(cfg.hippo.extras[ND_MUSIC_SET_VOLUME],"VOLUME"));
    CHECK(!strcmp(cfg.hippo.extras[ND_MUSIC_QUERY_PLAYING],"GET PLAY"));
    CHECK(cfg.hippo.volume_max==64U);
    CHECK(nd_music_valid_port("AMIGAAMP.1"));
    CHECK(!nd_music_valid_port("AMIGAAMP;STOP"));
    CHECK(!nd_music_valid_port(""));
    CHECK(nd_music_valid_command("NEXT TRACK"));
    CHECK(!nd_music_valid_command("PLAY;QUIT"));
    CHECK(!nd_music_valid_command("SAY 'oops'"));
    CHECK(!nd_music_valid_command("`CMD`"));
    CHECK(!nd_music_valid_command(""));
    CHECK(nd_music_select(0,ND_MUSIC_EAGLE,0,0,0,0)==ND_MUSIC_NONE);
    CHECK(nd_music_select(0,ND_MUSIC_EAGLE,1,0,0,0)==ND_MUSIC_EAGLE);
    CHECK(nd_music_select(0,ND_MUSIC_EAGLE,0,1,0,0)==ND_MUSIC_AMP);
    CHECK(nd_music_select(0,ND_MUSIC_EAGLE,0,0,1,0)==ND_MUSIC_HIPPO);
    CHECK(nd_music_select(0,ND_MUSIC_AMP,1,1,1,0)==ND_MUSIC_AMP);
    CHECK(nd_music_select(0,ND_MUSIC_HIPPO,1,1,1,0)==ND_MUSIC_HIPPO);
    CHECK(nd_music_select(0,ND_MUSIC_EAGLE,1,1,1,0)==ND_MUSIC_EAGLE);
    CHECK(nd_music_select(ND_MUSIC_EAGLE,ND_MUSIC_AMP,0,1,1,0)==ND_MUSIC_NONE);
    CHECK(nd_music_select(ND_MUSIC_AMP,ND_MUSIC_EAGLE,1,1,1,0)==ND_MUSIC_AMP);
    CHECK(nd_music_select(ND_MUSIC_HIPPO,ND_MUSIC_EAGLE,1,1,1,0)==ND_MUSIC_HIPPO);
    CHECK(!strcmp(nd_music_name(ND_MUSIC_AMP), "AmigaAMP"));
    CHECK(!strcmp(nd_music_name(ND_MUSIC_HIPPO),"HippoPlayer"));
    CHECK(!strcmp(nd_music_followup(ND_MUSIC_HIPPO,ND_MUSIC_NEXT),"PLAY"));
    CHECK(nd_music_followup(ND_MUSIC_EAGLE,ND_MUSIC_NEXT)==NULL);
    CHECK(nd_music_profile(&cfg, ND_MUSIC_NONE) == NULL);
    CHECK(!strcmp(cfg.lumi.port,"LUMIAMP"));
    CHECK(!strcmp(cfg.lumi.extras[ND_MUSIC_QUERY_TITLE],"GETSTATE TITLE"));
    CHECK(!strcmp(cfg.lumi.extras[ND_MUSIC_QUERY_TIME],"GETSTATE TIME"));
    CHECK(!strcmp(cfg.lumi.extras[ND_MUSIC_QUERY_PLAYING],"STATUS"));
    CHECK(!strcmp(nd_music_play_command(&cfg.lumi,1),"RESUME"));
    CHECK(!strcmp(nd_music_play_command(&cfg.lumi,0),"PLAY"));
    CHECK(!strcmp(nd_music_play_command(&cfg.hippo,1),"CONT"));
    CHECK(nd_music_profile(&cfg,ND_MUSIC_LUMI)==&cfg.lumi);
    CHECK(nd_music_select(ND_MUSIC_AUTO,ND_MUSIC_LUMI,1,1,1,1)==ND_MUSIC_LUMI);
    CHECK(nd_music_select(ND_MUSIC_AUTO,ND_MUSIC_EAGLE,0,0,0,1)==ND_MUSIC_LUMI);
    CHECK(nd_music_select(ND_MUSIC_AUTO,ND_MUSIC_EAGLE,1,0,0,1)==ND_MUSIC_EAGLE);
    CHECK(nd_music_select(ND_MUSIC_LUMI,ND_MUSIC_EAGLE,1,1,1,0)==ND_MUSIC_NONE);
    CHECK(nd_music_select(ND_MUSIC_LUMI,ND_MUSIC_EAGLE,1,1,1,1)==ND_MUSIC_LUMI);
    CHECK(nd_music_apply_option(&cfg,"MODE","LUMIAMP") && cfg.mode==ND_MUSIC_LUMI);
    CHECK(nd_music_apply_option(&cfg,"PREFER","LUMIAMP") && cfg.prefer==ND_MUSIC_LUMI);
    {unsigned int percent;char command[80];
     CHECK(nd_music_parse_volume(&cfg.lumi,"72",&percent) && percent==72);
     CHECK(nd_music_volume_command(&cfg.lumi,72,command,sizeof(command)) && !strcmp(command,"VOLUME 72"));}
    CHECK(nd_music_valid_command(cfg.lumi.extras[ND_MUSIC_VOL_UP]));
    CHECK(nd_music_valid_command(cfg.lumi.extras[ND_MUSIC_VOL_DOWN]));
    CHECK(!nd_music_apply_option(&cfg,"LUMI_PLAY","PLAY;QUIT"));
    nd_music_defaults(&cfg);
    saved = cfg;
    CHECK(!nd_music_load(&cfg, "tests/no-such-music-config"));
    CHECK(!strcmp(cfg.eagle.port, saved.eagle.port));
    f = fopen(path, "w"); CHECK(f != NULL); if (!f) return 1;
    fputs("# portable parser\nMODE=AMIGAAMP\nPREFER=AMIGAAMP\n"
          "AMIGAAMP_PORT=AMIGAAMP.2\nAMIGAAMP_PLAY=RESUME\n"
          "EAGLE_PORT=EAGLEPLAYER.3\nEAGLE_NEXT=NEXT SUBSONG\n"
          "HIPPO_PORT=HIPPOPLAYER\nHIPPO_QUERY_TITLE=GET NAME\n"
          "POLL_TICKS=250\n", f);
    fclose(f);
    CHECK(nd_music_load(&cfg, path));
    CHECK(cfg.mode == ND_MUSIC_AMP && cfg.prefer == ND_MUSIC_AMP);
    CHECK(!strcmp(cfg.amp.port, "AMIGAAMP.2"));
    CHECK(!strcmp(cfg.amp.commands[ND_MUSIC_PLAY], "RESUME"));
    CHECK(!strcmp(cfg.eagle.commands[ND_MUSIC_NEXT], "NEXT SUBSONG"));
    CHECK(!strcmp(cfg.hippo.extras[ND_MUSIC_QUERY_TITLE],"GET NAME"));
    CHECK(cfg.poll_ticks == 250);
    CHECK(cfg.auto_query == 1);
    CHECK(cfg.scroll_ticks == 25);
    CHECK(cfg.title_width == 24);
    CHECK(!strcmp(cfg.eagle.extras[ND_MUSIC_QUERY_TITLE], "STATUS M FIL"));
    saved = cfg;
    f = fopen(path, "w"); CHECK(f != NULL); if (!f) return 1;
    fputs("MODE=AUTO\nAMIGAAMP_PLAY=PLAY;QUIT\n", f); fclose(f);
    CHECK(!nd_music_load(&cfg, path));
    CHECK(!strcmp(cfg.amp.port, saved.amp.port));
    CHECK(cfg.mode == saved.mode);
    f = fopen(path, "w"); CHECK(f != NULL); if (!f) return 1;
    fputs("MODE=UNKNOWN\n", f); fclose(f);
    CHECK(!nd_music_load(&cfg, path));
    f = fopen(path, "w"); CHECK(f != NULL); if (!f) return 1;
    fputs("POLL_TICKS=999999\n", f); fclose(f);
    CHECK(!nd_music_load(&cfg, path));
    f = fopen(path, "w"); CHECK(f != NULL); if (!f) return 1;
    fputs("EAGLE_PORT=WHAT PORT\n", f); fclose(f);
    CHECK(!nd_music_load(&cfg, path));
    f = fopen(path, "w"); CHECK(f != NULL); if (!f) return 1;
    fputs("UNKNOWN_KEY=SOMETHING\n", f); fclose(f);
    CHECK(!nd_music_load(&cfg, path));
    f = fopen(path, "w"); CHECK(f != NULL); if (!f) return 1;
    fputs("MODE=AUTO\nAUTO_QUERY=YES\nQUERY_TICKS=600\n"
          "SCROLL_TITLE=NO\nSCROLL_TICKS=10\nTITLE_WIDTH=30\n"
          "EAGLE_QUERY_TITLE=GET NAME\nEAGLE_QUERY_TIME=GET POSITION\n"
          "EAGLE_VOL_UP=VOLUME UP\nEAGLE_VOL_DOWN=VOLUME DOWN\n"
          "AMIGAAMP_QUERY_TITLE=\nAMIGAAMP_QUERY_TIME=\n"
          "AMIGAAMP_VOL_UP=\nAMIGAAMP_VOL_DOWN=\n", f);
    fclose(f);
    CHECK(nd_music_load(&cfg, path));
    CHECK(cfg.auto_query == 1 && cfg.scroll_title == 0);
    CHECK(cfg.query_ticks == 600 && cfg.scroll_ticks == 10);
    CHECK(cfg.title_width == 30);
    CHECK(!strcmp(cfg.eagle.extras[ND_MUSIC_QUERY_TITLE], "GET NAME"));
    CHECK(!strcmp(cfg.eagle.extras[ND_MUSIC_VOL_UP], "VOLUME UP"));
    CHECK(cfg.amp.extras[ND_MUSIC_VOL_UP][0] == 0);
    {
        char command[64];
        unsigned int volume;
        int playing;
        CHECK(nd_music_parse_volume(&cfg.eagle, "128", &volume));
        CHECK(volume == 50U);
        CHECK(!nd_music_parse_volume(&cfg.eagle, "256", &volume));
        CHECK(nd_music_volume_command(&cfg.eagle, 50U, command,
                                      sizeof(command)));
        CHECK(!strcmp(command, "VOLUME 128"));
        CHECK(!nd_music_volume_command(&cfg.eagle, 101U, command,
                                       sizeof(command)));
        CHECK(!nd_music_volume_command(&cfg.amp, 50U, command,
                                       sizeof(command)));
        CHECK(nd_music_volume_command(&cfg.hippo,50U,command,sizeof(command)));
        CHECK(!strcmp(command,"VOLUME 32"));
        CHECK(nd_music_parse_playing("1",&playing) && playing==1);
        CHECK(nd_music_parse_playing("0 ",&playing) && playing==0);
        CHECK(nd_music_parse_playing("PLAYING",&playing) && playing==1);
        CHECK(nd_music_parse_playing("PAUSED",&playing) && playing==0);
        CHECK(nd_music_parse_playing("READY",&playing) && playing==0);
        CHECK(nd_music_parse_playing("EMPTY",&playing) && playing==0);
        CHECK(nd_music_parse_playing("ERROR",&playing) && playing==0);
        CHECK(!nd_music_parse_playing("NOT_PLAYING",&playing));
        CHECK(!nd_music_parse_playing("1 unknown",&playing));
    }
    saved = cfg;
    f = fopen(path, "w"); CHECK(f != NULL); if (!f) return 1;
    fputs("AUTO_QUERY=MAYBE\n", f); fclose(f);
    CHECK(!nd_music_load(&cfg, path));
    CHECK(cfg.auto_query == saved.auto_query);
    f = fopen(path, "w"); CHECK(f != NULL); if (!f) return 1;
    fputs("AMIGAAMP_QUERY_TITLE=GET TITLE;QUIT\n", f); fclose(f);
    CHECK(!nd_music_load(&cfg, path));
    f = fopen(path, "w"); CHECK(f != NULL); if (!f) return 1;
    fputs("SCROLL_TICKS=0\n", f); fclose(f);
    CHECK(!nd_music_load(&cfg, path));
    f = fopen(path, "w"); CHECK(f != NULL); if (!f) return 1;
    fputs("TITLE_WIDTH=500\n", f); fclose(f);
    CHECK(!nd_music_load(&cfg, path));
    {
        char cleaned[32], marquee[16], shortbuf[5];
        nd_music_clean_text(cleaned, sizeof(cleaned), "  Intro\r\nPart\tTwo\001");
        CHECK(!strcmp(cleaned, "  Intro  Part Two"));
        nd_music_clean_text(cleaned, sizeof(cleaned), NULL);
        CHECK(cleaned[0] == 0);
        nd_music_clean_text(shortbuf, sizeof(shortbuf), "Long result");
        CHECK(!strcmp(shortbuf, "Long"));
        nd_music_marquee(marquee, sizeof(marquee), "Hello World", 0, 5, 1);
        CHECK(!strcmp(marquee, "Hello"));
        nd_music_marquee(marquee, sizeof(marquee), "Hello World", 6, 5, 1);
        CHECK(!strcmp(marquee, "World"));
        nd_music_marquee(marquee, sizeof(marquee), "Hello World", 11, 5, 1);
        CHECK(!strcmp(marquee, "   He"));
        nd_music_marquee(marquee, sizeof(marquee), "Hi", 50, 8, 1);
        CHECK(!strcmp(marquee, "Hi"));
        nd_music_marquee(marquee, sizeof(marquee), "Hello World", 6, 5, 0);
        CHECK(!strcmp(marquee, "Hello"));
        nd_music_marquee(shortbuf, sizeof(shortbuf), "Long title", 0, 20, 1);
        CHECK(strlen(shortbuf) == 4);
    }
    remove(path);
    printf("MusicPlayer tests: %s\n", failed ? "FAIL" : "PASS");
    return failed != 0;
}
