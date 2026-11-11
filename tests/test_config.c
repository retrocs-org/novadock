/* Portable tests. Run on host: cc -std=c89 -Wall -Wextra -pedantic ... */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"

int main(void)
{
    struct NDConfig cfg;
    int n,i;
    FILE *legacy;
    struct NDConfig recovered;
    nd_default_config(&cfg);
    /* Editing items and layout does not require replacing window resources. */
    recovered=cfg;
    assert(nd_move_visible(&recovered,0,1));
    assert(!nd_config_runtime_equal(&cfg,&recovered));
    assert(nd_config_can_update_live(&cfg,&recovered));
    recovered=cfg;
    assert(nd_add_launcher(&recovered,ND_ITEM_LAUNCH,"Editor","C:Ed",""));
    assert(nd_config_can_update_live(&cfg,&recovered));
    assert(nd_remove_visible_item(&recovered,0));
    recovered.rows=2;recovered.position=ND_POSITION_END;
    assert(nd_config_can_update_live(&cfg,&recovered));
    recovered.font_mode=ND_FONT_TOPAZ;
    assert(!nd_config_can_update_live(&cfg,&recovered));
    recovered=cfg;recovered.refresh_mode=ND_REFRESH_SMOOTH;
    assert(!nd_config_can_update_live(&cfg,&recovered));
    recovered=cfg;recovered.item_size=ND_ITEM_SIZE_TINY;
    assert(!nd_config_can_update_live(&cfg,&recovered));
    recovered=cfg;nd_config_set_theme(&recovered,"Classic");
    assert(!nd_config_can_update_live(&cfg,&recovered));
    assert(!nd_config_can_update_live(NULL,&cfg));
    assert(cfg.count == 11);
    assert(cfg.side == ND_SIDE_BOTTOM);
    assert(cfg.position == ND_POSITION_CENTER);
    assert(cfg.font_mode == ND_FONT_WORKBENCH);
    assert(cfg.running_apps == 1);
    assert(cfg.refresh_mode == ND_REFRESH_LOW_MEMORY);
    assert(cfg.rows == 1);
    assert(cfg.label_mode == ND_LABEL_ALWAYS);
    assert(cfg.item_size == ND_ITEM_SIZE_NORMAL);
    assert(cfg.shape == ND_SHAPE_ROUNDED);
    assert(cfg.tile_height == 76);
    assert(cfg.theme==ND_THEME_NORD && !strcmp(nd_config_theme_id(&cfg),"Nord"));
    assert(cfg.items[0].kind==ND_ITEM_DRAWER && !strcmp(cfg.items[0].command,"SYS:"));
    assert(cfg.items[0].show_volume_meter);
    assert(!strcmp(nd_item_text(&cfg,&cfg.items[0],ND_TEXT_ICON),"SYS:Disk"));
    assert(!strcmp(cfg.items[1].command,"NewShell"));
    assert(!strcmp(cfg.items[3].id,"CPU"));
    assert(!strcmp(cfg.items[4].id,"RAM"));
    assert(!strcmp(cfg.items[5].id,"CLOCK"));
    for (i=0;i<cfg.count;++i) assert(cfg.items[i].enabled==(i<6));

    assert(cfg.tooltips == 1);
    /* Keyboard-control hot key: optional, kept verbatim, bounded, and it
     * counts as a runtime change (a reload must rebuild the hot keys). */
    {
        struct NDConfig other;
        char too_long[ND_HOTKEY_LEN+8];
        assert(cfg.dock_hotkey[0]=='\0');
        memcpy(&other,&cfg,sizeof(other));
        assert(nd_parse_line(&other,"DOCK_HOTKEY=ctrl alt d")==1);
        assert(!strcmp(other.dock_hotkey,"ctrl alt d"));
        assert(!nd_config_runtime_equal(&cfg,&other));
        memset(too_long,'x',sizeof(too_long)-1);
        too_long[sizeof(too_long)-1]='\0';
        {
            char line[ND_HOTKEY_LEN+32];
            sprintf(line,"DOCK_HOTKEY=%s",too_long);
            assert(nd_parse_line(&other,line)==0);
            assert(!strcmp(other.dock_hotkey,"ctrl alt d"));
        }
        /* Written only when set; read back unchanged. */
        {
            struct NDConfig back;
            FILE *f;
            char text[256];
            int seen=0;
            assert(nd_write_config(&other,"tests/hotkey.cfg"));
            f=fopen("tests/hotkey.cfg","r");
            assert(f);
            while (fgets(text,sizeof(text),f))
                if (!strcmp(text,"DOCK_HOTKEY=ctrl alt d\n")) ++seen;
            fclose(f);
            assert(seen==1);
            assert(nd_load_config(&back,"tests/hotkey.cfg")==0);
            assert(!strcmp(back.dock_hotkey,"ctrl alt d"));
            assert(nd_write_config(&cfg,"tests/hotkey.cfg"));
            f=fopen("tests/hotkey.cfg","r");
            assert(f);
            while (fgets(text,sizeof(text),f))
                assert(strncmp(text,"DOCK_HOTKEY",11));
            fclose(f);
            remove("tests/hotkey.cfg");
        }
    }
    assert(nd_parse_line(&cfg, "TOOLTIPS=NO") == 1);
    assert(cfg.tooltips == 0);
    assert(nd_parse_line(&cfg, "TOOLTIPS=YES") == 1);
    assert(cfg.tooltips == 1);
    assert(nd_parse_line(&cfg, "TOOLTIPS=MAYBE") == 0);
    assert(cfg.always_on_top == 0);
    assert(nd_parse_line(&cfg, "ALWAYS_ON_TOP=YES") == 1);
    assert(cfg.always_on_top == 1);
    assert(nd_parse_line(&cfg, "ALWAYS_ON_TOP=NO") == 1);
    assert(cfg.always_on_top == 0);
    assert(nd_parse_line(&cfg, "ALWAYS_ON_TOP=MAYBE") == 0);
    assert(nd_parse_line(&cfg, "RUNNING_APPS=NO") == 1 && cfg.running_apps == 0);
    assert(nd_parse_line(&cfg, "RUNNING_APPS=YES") == 1 && cfg.running_apps == 1);
    assert(nd_parse_line(&cfg, "RUNNING_APPS=MAYBE") == 0);
    assert(nd_parse_line(&cfg,"REFRESH_MODE=SMOOTH") == 1);
    assert(cfg.refresh_mode==ND_REFRESH_SMOOTH);
    assert(!strcmp(nd_refresh_mode_name(cfg.refresh_mode),"SMOOTH"));
    assert(nd_parse_line(&cfg,"REFRESH_MODE=LOW_MEMORY") == 1);
    assert(cfg.refresh_mode==ND_REFRESH_LOW_MEMORY);
    assert(!strcmp(nd_refresh_mode_name(cfg.refresh_mode),"LOW_MEMORY"));
    assert(nd_parse_line(&cfg,"REFRESH_MODE=FAST") == 0);
    assert(nd_parse_line(&cfg,"ROWS=3") == 1 && cfg.rows==3);
    assert(nd_parse_line(&cfg,"ROWS=0") == 0);
    assert(nd_parse_line(&cfg,"ROWS=4") == 0);
    assert(cfg.items[3].kind == ND_ITEM_PLUGIN);
    assert(cfg.items[3].graph == 0);
    assert(!cfg.items[4].graph && !cfg.items[5].graph && !cfg.items[6].graph);

    assert(strcmp(cfg.items[4].id, "RAM") == 0);
    assert(strcmp(cfg.items[6].id, "NET") == 0);
    assert(strcmp(cfg.items[2].id, "MUSIC") == 0);
    assert(strcmp(cfg.items[7].id, "EYES") == 0);
    assert(nd_plugin_allows_multiple(&cfg.items[7]));
    n=cfg.count;
    assert(nd_add_plugin_instance(&cfg,7));
    assert(cfg.count==n+1 && !strcmp(cfg.items[n].id,"EYES"));
    assert(cfg.items[n].enabled && cfg.items[n].multiple_instances);
    /* The installed-plugin catalogue and active tiles share this array.  A
     * populated dock must still have room for another multi-instance Eyes
     * tile and for late catalogue entries such as Running Apps. */
    nd_default_config(&cfg);
    for (i=0;i<13;++i) {
        n=cfg.count;
        cfg.items[n]=cfg.items[0];
        cfg.items[n].kind=ND_ITEM_SPACER;
        cfg.items[n].enabled=1;
        ++cfg.count;
    }
    assert(cfg.count==24);
    assert(nd_add_plugin_instance(&cfg,7));
    assert(cfg.count==25 && !strcmp(cfg.items[24].id,"EYES"));
    nd_default_config(&cfg); /* keep the fixed catalogue indexes below */
    assert(strcmp(cfg.items[8].id, "DOPUS4") == 0 && !cfg.items[8].enabled);
    assert(strcmp(cfg.items[9].id,"ARCHIVE")==0 && !cfg.items[9].enabled);
    assert(strcmp(cfg.items[10].id,"RUNNINGAPPS")==0 && !cfg.items[10].enabled);
    assert(strcmp(cfg.items[5].id, "CLOCK") == 0);
    assert(nd_parse_line(&cfg, " SIDE = LEFT \r\n") == 1);
    assert(cfg.side == ND_SIDE_LEFT);
    assert(nd_parse_line(&cfg,"POSITION=START") && cfg.position==ND_POSITION_START);
    assert(nd_parse_line(&cfg,"POSITION=END") && cfg.position==ND_POSITION_END);
    assert(nd_parse_line(&cfg,"POSITION=CENTER") && cfg.position==ND_POSITION_CENTER);
    assert(!nd_parse_line(&cfg,"POSITION=FLOAT"));
    assert(nd_parse_line(&cfg,"FONT=TOPAZ") && cfg.font_mode==ND_FONT_TOPAZ);
    assert(nd_parse_line(&cfg,"FONT=BOLD") && cfg.font_mode==ND_FONT_BOLD);
    assert(nd_parse_line(&cfg,"FONT=WORKBENCH") && cfg.font_mode==ND_FONT_WORKBENCH);
    assert(!nd_parse_line(&cfg,"FONT=FUTURA"));
    assert(nd_parse_line(&cfg,"LABELS=HOVER") && cfg.label_mode==ND_LABEL_HOVER);
    assert(nd_parse_line(&cfg,"LABELS=HIDDEN") && cfg.label_mode==ND_LABEL_HIDDEN);
    assert(nd_parse_line(&cfg,"LABELS=ALWAYS") && cfg.label_mode==ND_LABEL_ALWAYS);
    assert(!nd_parse_line(&cfg,"LABELS=SOMETIMES"));
    assert(nd_parse_line(&cfg,"ITEM_SIZE=TINY") &&
           cfg.item_size==ND_ITEM_SIZE_TINY);
    assert(!strcmp(nd_item_size_name(cfg.item_size),"TINY"));
    assert(nd_parse_line(&cfg,"ITEM_SIZE=NORMAL") &&
           cfg.item_size==ND_ITEM_SIZE_NORMAL);
    assert(!nd_parse_line(&cfg,"ITEM_SIZE=HUGE"));
    assert(!strcmp(nd_position_name(ND_POSITION_END),"END"));
    assert(!strcmp(nd_font_name(ND_FONT_BOLD),"BOLD"));
    assert(!strcmp(nd_label_mode_name(ND_LABEL_HOVER),"HOVER"));
    assert(nd_parse_line(&cfg, "THEME=LIGHT") == 1);
    assert(cfg.theme == ND_THEME_LIGHT);
    assert(nd_parse_line(&cfg, "THEME=WORKBENCH") == 1);
    assert(cfg.theme == ND_THEME_WORKBENCH);
    assert(nd_parse_line(&cfg, "THEME=GRAPHITE") == 1);
    assert(cfg.theme == ND_THEME_GRAPHITE);
    assert(nd_parse_line(&cfg, "THEME=HIGH_CONTRAST") == 1);
    assert(cfg.theme == ND_THEME_HIGH_CONTRAST);
    assert(nd_parse_line(&cfg, "THEME=COPPER") == 1);
    assert(cfg.theme == ND_THEME_COPPER);
    assert(nd_parse_line(&cfg, "THEME=OCEAN") == 1);
    assert(cfg.theme == ND_THEME_OCEAN);
    assert(nd_parse_line(&cfg, "THEME=EMERALD") == 1);
    assert(cfg.theme == ND_THEME_EMERALD);
    assert(nd_parse_line(&cfg, "THEME=NEXTSTEP") == 1);
    assert(cfg.theme == ND_THEME_NEXTSTEP);
    assert(nd_parse_line(&cfg, "THEME=BEOS") == 1);
    assert(cfg.theme == ND_THEME_BEOS);
    assert(nd_parse_line(&cfg, "THEME=NORD") == 1);
    assert(cfg.theme == ND_THEME_NORD);
    assert(nd_parse_line(&cfg, "THEME=VAPOR") == 1);
    assert(cfg.theme == ND_THEME_VAPOR);
    assert(nd_parse_line(&cfg,"THEME=BlueCopper") == 1);
    assert(!strcmp(nd_config_theme_id(&cfg),"BlueCopper"));
    assert(nd_parse_line(&cfg,"THEME=../BlueCopper") == 0);
    assert(!strcmp(nd_config_theme_id(&cfg),"BlueCopper"));
    assert(nd_parse_line(&cfg,"THEME=MOCHA") == 1);
    assert(!strcmp(nd_theme_name(ND_THEME_MOCHA), "MOCHA"));
    assert(!strcmp(nd_theme_name(ND_THEME_HIGH_CONTRAST), "HIGH_CONTRAST"));
    assert(!strcmp(nd_theme_name(ND_THEME_COPPER), "COPPER"));
    assert(!strcmp(nd_theme_name(ND_THEME_OCEAN), "OCEAN"));
    assert(!strcmp(nd_theme_name(ND_THEME_EMERALD), "EMERALD"));
    assert(!strcmp(nd_theme_name(ND_THEME_NEXTSTEP), "NEXTSTEP"));
    assert(!strcmp(nd_theme_name(ND_THEME_BEOS), "BEOS"));
    assert(!strcmp(nd_theme_name(ND_THEME_NORD), "NORD"));
    assert(!strcmp(nd_theme_name(ND_THEME_VAPOR), "VAPOR"));
    assert(nd_parse_line(&cfg,"SHAPE=CHAMFER")==1);
    assert(cfg.shape==ND_SHAPE_CHAMFER);
    assert(!strcmp(nd_shape_name(cfg.shape),"CHAMFER"));
    assert(nd_parse_line(&cfg,"SHAPE=ROUNDISH")==0);
    assert(nd_parse_line(&cfg,"BACKGROUND=Backgrounds/BrushedMetal.iff")==1);
    assert(nd_parse_line(&cfg,"BACKGROUND=Backgrounds/Circuit.iff")==1);
    assert(nd_parse_line(&cfg,"BACKGROUND=")==1);
    assert(nd_parse_line(&cfg,"BACKGROUND=bad|path")==0);
    assert(nd_parse_line(&cfg, "TILE_WIDTH=55") == 1);
    assert(nd_parse_line(&cfg, "TILE_WIDTH=1") == 0);
    assert(nd_parse_line(&cfg, "GAP=900") == 0);
    assert(nd_parse_line(&cfg, "PLUGIN=Duplicate|Plugins/Cpu|CPU") == 0);
    assert(nd_parse_line(&cfg, "PLUGIN=Unsafe|Plugins/Cpu|bad id") == 0);
    n = cfg.count;
    assert(nd_parse_line(&cfg, "PLUGIN=Test|Plugins/Test|TEST_1") == 1);
    assert(cfg.count == n + 1);
    assert(cfg.items[n].graph == 0);
    assert(nd_parse_line(&cfg, "PLUGIN=Graph|Plugins/Graph|G123|GRAPH") == 1);
    assert(cfg.items[cfg.count - 1].graph == 1);
    nd_default_config(&cfg);
    cfg.count=0;
    assert(nd_parse_line(&cfg,"PLUGIN=BitSkim|Plugins/BitSkim|BITSKIM|GRAPH")==1);
    assert(cfg.items[0].graph==0); /* old graph settings migrate to progress */
    assert(nd_parse_line(&cfg,"OPTION.BITSKIM.PROGRAM=Bin:BitSkim/BitSkim"));
    assert(!strcmp(nd_option_get(&cfg,"BITSKIM","PROGRAM"),"SYS:Utilities/BitSkim/BitSkim"));
    assert(nd_parse_line(&cfg,"OPTION.BITSKIM.PROGRAM=Work:Custom/BitSkim"));
    assert(!strcmp(nd_option_get(&cfg,"BITSKIM","PROGRAM"),"Work:Custom/BitSkim"));
    cfg.count=0;
    assert(nd_parse_line(&cfg,"PLUGIN=Mixer|SYS:Utilities/NovaDock/Plugins/Mixer|MIXER|METER|OFF"));
    assert(nd_parse_line(&cfg,"PLUGIN=Custom|Work:Extra/Plugins/Custom|CUSTOM|METER|OFF"));
    nd_upgrade_network_config(&cfg);
    assert(!strcmp(cfg.items[0].command,"Plugins/Mixer"));
    assert(!strcmp(cfg.items[1].command,"Work:Extra/Plugins/Custom"));
    assert(nd_parse_line(&cfg,"OPTION.SCRIPT.SCRIPT=SYS:Utilities/NovaDock/Rexx/ScriptTile.rexx"));
    assert(!strcmp(nd_option_get(&cfg,"SCRIPT","SCRIPT"),"NovaDock:Rexx/ScriptTile.rexx"));
    nd_default_config(&cfg); /* leave room to exercise the METER parser */
    assert(nd_parse_line(&cfg, "PLUGIN=Meter|Plugins/Meter|M123|METER") == 1);
    assert(cfg.items[cfg.count - 1].graph == 0);
    assert(nd_parse_line(&cfg, "PLUGIN=Unknown|Plugins/Test|BAD1|SPARK") == 0);
    assert(nd_parse_line(&cfg, "PLUGIN=Invalid|Plugins/Test|BAD2|GRAPH|PIPE") == 0);
    assert(nd_parse_line(&cfg, "PLUGIN=Invalid|Plugins/Test|BAD3|") == 0);

    nd_default_config(&cfg); /* leave space for launcher case */
    assert(nd_parse_line(&cfg, "LAUNCH=Foo|SYS:Tools/Foo|SYS:Tools/Foo") == 1);
    assert(strcmp(nd_item_text(&cfg,&cfg.items[cfg.count - 1],ND_TEXT_ICON), "SYS:Tools/Foo") == 0);
    assert(nd_parse_line(&cfg,"LAUNCH=Spare|SYS:Tools/MultiView||OFF")==1);
    assert(!cfg.items[cfg.count-1].enabled);
    assert(!strcmp(nd_item_text(&cfg,&cfg.items[cfg.count-1],ND_TEXT_ICON),""));
    assert(nd_parse_line(&cfg,"LAUNCH=Editor|C:Ed||OFF|CLI|S:Startup-Sequence|SYS:|control alt e|YES")==1);
    assert(cfg.items[cfg.count-1].launch_mode==ND_LAUNCH_CLI);
    assert(!strcmp(nd_item_text(&cfg,&cfg.items[cfg.count-1],ND_TEXT_ARGS),"S:Startup-Sequence"));
    assert(!strcmp(nd_item_text(&cfg,&cfg.items[cfg.count-1],ND_TEXT_WORKDIR),"SYS:"));
    assert(!strcmp(nd_item_text(&cfg,&cfg.items[cfg.count-1],ND_TEXT_HOTKEY),"control alt e"));
    assert(cfg.items[cfg.count-1].tools_menu==1);
    assert(nd_parse_line(&cfg,"LAUNCH=Player|SYS:Tools/Player||ON|AUTO||||NO|YES|PlayerTask|Player *|PLAYER.1")==1);
    assert(!strcmp(nd_item_text(&cfg,&cfg.items[cfg.count-1],ND_TEXT_MATCH_TASK),"PlayerTask"));
    assert(!strcmp(nd_item_text(&cfg,&cfg.items[cfg.count-1],ND_TEXT_MATCH_WINDOW),"Player *"));
    assert(!strcmp(nd_item_text(&cfg,&cfg.items[cfg.count-1],ND_TEXT_MATCH_PORT),"PLAYER.1"));
    assert(nd_set_launcher_match(&cfg,cfg.count-1,"OtherTask","* Player","")==1);
    assert(!strcmp(nd_item_text(&cfg,&cfg.items[cfg.count-1],ND_TEXT_MATCH_TASK),"OtherTask"));
    assert(!nd_set_launcher_match(&cfg,cfg.count-1,"bad|task","", ""));
    assert(nd_parse_line(&cfg,"LAUNCH=BadMode|C:Ed||ON|MAGIC||||NO")==0);
    assert(nd_parse_line(&cfg, "LAUNCH=Foo|with|icon|pipe") == 0);
    assert(nd_parse_line(&cfg,"DRAWER=Games|Games:|Icons/Games.iff")==1);
    assert(cfg.items[cfg.count-1].kind==ND_ITEM_DRAWER);
    assert(!strcmp(nd_item_text(&cfg,&cfg.items[cfg.count-1],ND_TEXT_ICON),"Icons/Games.iff"));
    assert(cfg.items[cfg.count-1].show_volume_meter==1);
    assert(nd_parse_line(&cfg,"DRAWER=Utilities|SYS:Utilities|Icons/Drawer.iff")==1);
    assert(cfg.items[cfg.count-1].show_volume_meter==0);
    assert(nd_item_supports_space_meter(&cfg.items[cfg.count-1]));
    assert(nd_parse_line(&cfg,"DRAWER=Quiet|Quiet:||ON|AUTO||||NO|NO")==1);
    assert(cfg.items[cfg.count-1].show_volume_meter==0);
    assert(nd_parse_line(&cfg,"DRAWER=Bad|Bad:||ON|AUTO||||NO|MAYBE")==0);
    assert(nd_parse_line(&cfg,"GROUP=Work|SYS:Utilities|Icons/Drawer.iff|ON|AUTO||||NO|NO")==1);
    assert(cfg.items[cfg.count-1].kind==ND_ITEM_GROUP);
    assert(nd_item_is_group(&cfg.items[cfg.count-1]));
    assert(!strcmp(cfg.items[cfg.count-1].command,"SYS:Utilities"));
    assert(nd_remove_launcher(&cfg,cfg.count-1));
    assert(nd_edit_launcher(&cfg,cfg.count-1,ND_ITEM_DRAWER,
                            "Workbench","SYS:","Icons/Drawer.iff"));
    assert(!strcmp(cfg.items[cfg.count-1].command,"SYS:"));
    assert(!nd_add_launcher(&cfg,ND_ITEM_DRAWER,"Bad|Label","SYS:",""));
    assert(!nd_edit_launcher(&cfg,cfg.count-1,ND_ITEM_DRAWER,
                             "Bad", "SYS:|evil", ""));
    assert(nd_set_launcher_action(&cfg,cfg.count-1,ND_LAUNCH_WORKBENCH,
                                  "","SYS:","control alt w",1));
    assert(!nd_set_launcher_action(&cfg,cfg.count-1,99,"","","",0));
    assert(nd_remove_launcher(&cfg,cfg.count-1));
    assert(nd_add_spacer(&cfg,"Work"));
    assert(cfg.items[cfg.count-1].kind==ND_ITEM_SPACER);
    assert(cfg.items[cfg.count-1].spacer_width==0);
    assert(cfg.items[cfg.count-1].spacer_line==1);
    assert(cfg.items[cfg.count-1].spacer_line_width==2);
    assert(cfg.items[cfg.count-1].spacer_line_color==ND_SPACER_COLOR_SUBTLE);
    assert(nd_remove_launcher(&cfg,cfg.count-1));
    assert(nd_parse_line(&cfg,"SPACER=Wide|ON|72")==1);
    assert(cfg.items[cfg.count-1].spacer_width==72);
    assert(nd_remove_launcher(&cfg,cfg.count-1));
    assert(nd_parse_line(&cfg,"SPACER=Default|ON|DEFAULT")==1);
    assert(cfg.items[cfg.count-1].spacer_width==0);
    assert(nd_parse_line(&cfg,"SPACER=Styled|ON|48|NO|ACCENT|4")==1);
    assert(cfg.items[cfg.count-1].spacer_width==48);
    assert(cfg.items[cfg.count-1].spacer_line==0);
    assert(cfg.items[cfg.count-1].spacer_line_width==4);
    assert(cfg.items[cfg.count-1].spacer_line_color==ND_SPACER_COLOR_ACCENT);
    assert(nd_parse_line(&cfg,"SPACER=BadColor|ON|40|YES|PURPLE|2")==0);
    assert(nd_remove_launcher(&cfg,cfg.count-1));
    assert(nd_parse_line(&cfg,"SPACER=Bad|ON|8")==0);
    assert(nd_parse_line(&cfg, "# sample") == 1);
    /* Unknown keys from newer versions are preserved, not fatal. */
    assert(nd_parse_line(&cfg, "unrecognised=x") == 1);
    assert(cfg.unknown_count == 1 && !strcmp(cfg.unknown[0], "unrecognised=x"));
    assert(nd_parse_line(&cfg, "unrecognised=y") == 1 && cfg.unknown_count == 1);
    assert(!strcmp(cfg.unknown[0], "unrecognised=y"));
    assert(nd_parse_line(&cfg, "bad key=x") == 0);
    assert(nd_parse_line(&cfg, "no equals sign") == 0);
    /* Enumerated values and keys are case-insensitive. */
    assert(nd_parse_line(&cfg, "theme=copper") == 1 && cfg.theme == ND_THEME_COPPER);
    assert(nd_parse_line(&cfg, "Side=Left") == 1 && cfg.side == ND_SIDE_LEFT);
    assert(nd_parse_line(&cfg, "TOOLTIPS=no") == 1 && cfg.tooltips == 0);
    assert(nd_load_config(&cfg, "novadock.cfg") == 0);
    assert(cfg.count == 17); /* six active tiles plus optional definitions */
    assert(!cfg.items[16].enabled && !strcmp(cfg.items[16].id,"BITSKIM"));
    assert(!cfg.items[14].enabled && !strcmp(cfg.items[14].id,"AMIRC"));
    assert(!cfg.items[15].enabled && !strcmp(cfg.items[15].id,"CLIPBOARD"));
    for (i=0;i<cfg.count;++i) assert(cfg.items[i].enabled==(i<6));
    assert(cfg.theme==ND_THEME_NORD && cfg.shape==ND_SHAPE_ROUNDED);
    assert(!strcmp(cfg.items[0].command,"SYS:"));
    assert(!strcmp(cfg.items[1].command,"NewShell"));

    assert(!cfg.items[13].enabled && !strcmp(cfg.items[13].id,"KICKSTART"));
    assert(!cfg.items[10].enabled && !strcmp(cfg.items[10].id,"RUNNINGAPPS"));
    assert(cfg.items[3].graph == 0); /* default CPU meter */
    assert(!cfg.items[4].graph && !cfg.items[5].graph && !cfg.items[6].graph);
    legacy=fopen("tests/legacy-ram.cfg","w");
    assert(legacy!=NULL);
    fputs("PLUGIN=Chip RAM|Plugins/Memory|CHIPRAM|GRAPH|ON\n",legacy);
    fputs("PLUGIN=Fast RAM|Plugins/Memory|FASTRAM|GRAPH|OFF\n",legacy);
    fputs("OPTION.CHIPRAM.POLL_TICKS=60\n",legacy);
    fclose(legacy);
    assert(nd_load_config(&cfg,"tests/legacy-ram.cfg")==0);
    assert(cfg.count==9 && !strcmp(cfg.items[0].id,"RAM"));
    /* Every bundled plugin remains available to Add Item after loading a
     * configuration written before that plugin shipped. */
    assert(!strcmp(cfg.items[1].id,"CLOCK") && !cfg.items[1].enabled);
    assert(!strcmp(cfg.items[2].id,"CPU") && !cfg.items[2].enabled);
    assert(!strcmp(cfg.items[5].id,"EYES") &&
           cfg.items[5].multiple_instances && !cfg.items[5].enabled);
    assert(!strcmp(cfg.items[6].id,"DOPUS4") && !cfg.items[6].enabled);
    assert(!strcmp(cfg.items[7].id,"ARCHIVE") && !cfg.items[7].enabled);
    assert(!strcmp(cfg.items[8].id,"RUNNINGAPPS") && !cfg.items[8].enabled);
    assert(cfg.items[0].enabled && !cfg.items[0].graph);
    assert(!strcmp(nd_option_get(&cfg,"RAM","POLL_TICKS"),"60"));
    remove("tests/legacy-ram.cfg");

    /* A successful atomic save publishes a separately validated recovery
     * generation.  It must remain usable even if the live file is later
     * damaged by an interrupted external edit. */
    nd_default_config(&cfg);
    assert(nd_parse_line(&cfg,"GROUP=Work|SYS:Utilities||ON|AUTO||||NO|NO")==1);
    assert(nd_write_config(&cfg,"tests/known-good.cfg"));
    assert(nd_load_config(&recovered,"tests/known-good.cfg.good")==0);
    assert(recovered.count==cfg.count);
    assert(recovered.items[recovered.count-1].kind==ND_ITEM_GROUP);
    legacy=fopen("tests/known-good.cfg","w");
    assert(legacy!=NULL);
    fputs("THEME=../NOT_A_THEME\n",legacy);
    fclose(legacy);
    assert(nd_load_config(&recovered,"tests/known-good.cfg")>0);
    assert(nd_load_config(&recovered,"tests/known-good.cfg.good")==0);
    remove("tests/known-good.cfg");
    remove("tests/known-good.cfg.good");
    remove("tests/known-good.cfg.bak");
    assert(nd_load_config(&cfg, "this-file-does-not-exist.cfg") == -1);
    assert(cfg.count == 11);
    puts("Config tests passed.");
    return 0;
}
