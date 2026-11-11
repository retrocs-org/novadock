#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_theme.h"

int main(void)
{
    struct NDThemeSpec spec;
    struct NDConfig cfg;
    int i;
    assert(nd_theme_id_valid("BlueCopper"));
    assert(nd_theme_id_valid("blue-copper_2"));
    assert(!nd_theme_id_valid("Blue Copper"));
    assert(!nd_theme_id_valid("../BlueCopper"));
    assert(nd_theme_index_from_id("Ocean")==ND_THEME_OCEAN);
    assert(nd_theme_index_from_id("BlueCopper")==-1);
    nd_theme_defaults_id("BlueCopper",&spec);
    assert(!strcmp(spec.name,"BlueCopper"));
    for (i=0;i<ND_THEME_COUNT;++i) {
        int separation;
        assert(nd_theme_load(".",i,&spec)==1);
        assert(spec.name[0]!='\0');
        assert(spec.shape>=ND_SHAPE_ROUNDED && spec.shape<ND_SHAPE_COUNT);
        assert((spec.binding_mask&ND_THEME_BIND_ALL)==ND_THEME_BIND_ALL);
        assert(spec.side>=ND_SIDE_BOTTOM && spec.side<=ND_SIDE_RIGHT);
        assert(spec.position>=ND_POSITION_START && spec.position<ND_POSITION_COUNT);
        if (i!=ND_THEME_NEXTSTEP) {
            assert(spec.side==ND_SIDE_BOTTOM);
            assert(spec.position==ND_POSITION_CENTER);
        }
        assert(spec.font_mode>=ND_FONT_WORKBENCH && spec.font_mode<ND_FONT_COUNT);
        assert(spec.label_mode>=ND_LABEL_ALWAYS && spec.label_mode<ND_LABEL_COUNT);
        assert(spec.spacer_width>=20 && spec.spacer_width<=140);
        assert(spec.spacer_line==0 || spec.spacer_line==1);
        assert(spec.spacer_line_width>=1 && spec.spacer_line_width<=8);
        assert(spec.spacer_line_color>=ND_SPACER_COLOR_SUBTLE &&
               spec.spacer_line_color<ND_SPACER_COLOR_COUNT);
        separation=(spec.rgb[0][0]>spec.rgb[1][0] ?
                    spec.rgb[0][0]-spec.rgb[1][0] :
                    spec.rgb[1][0]-spec.rgb[0][0])+
                   (spec.rgb[0][1]>spec.rgb[1][1] ?
                    spec.rgb[0][1]-spec.rgb[1][1] :
                    spec.rgb[1][1]-spec.rgb[0][1])+
                   (spec.rgb[0][2]>spec.rgb[1][2] ?
                    spec.rgb[0][2]-spec.rgb[1][2] :
                    spec.rgb[1][2]-spec.rgb[0][2]);
        if (spec.shape!=ND_SHAPE_MINIMAL) assert(separation>=30);
    }
    assert(nd_theme_load(".",ND_THEME_MOCHA,&spec)==1);
    assert(spec.shape==ND_SHAPE_CHAMFER && spec.spacer_line==0);
    assert(spec.spacer_width==48 && spec.spacer_line_width==2);
    memset(&cfg,0,sizeof(cfg));
    cfg.side=ND_SIDE_RIGHT;cfg.position=ND_POSITION_END;
    cfg.font_mode=ND_FONT_BOLD;cfg.label_mode=ND_LABEL_HIDDEN;
    nd_theme_apply_preferences(&cfg,&spec);
    assert(cfg.side==ND_SIDE_BOTTOM && cfg.position==ND_POSITION_CENTER);
    assert(cfg.font_mode==ND_FONT_WORKBENCH && cfg.label_mode==ND_LABEL_ALWAYS);
    memset(&cfg,0,sizeof(cfg));cfg.count=2;
    cfg.items[0].kind=ND_ITEM_LAUNCH;cfg.items[0].spacer_width=99;
    cfg.items[1].kind=ND_ITEM_SPACER;
    nd_theme_apply_spacers(&cfg,&spec);
    assert(cfg.items[0].spacer_width==99);
    assert(cfg.items[1].spacer_width==48 && cfg.items[1].spacer_line==0);
    assert(nd_theme_load(".",ND_THEME_COPPER,&spec)==1);
    assert(!strcmp(spec.name,"Copper"));
    assert(spec.rgb[0][0]==43 && spec.rgb[5][0]==225);
    assert(spec.shape==ND_SHAPE_CHAMFER && spec.spacer_line==1);
    assert(nd_theme_load(".",ND_THEME_OCEAN,&spec)==1);
    assert(spec.rgb[5][2]==235);
    assert(spec.shape==ND_SHAPE_ROUNDED && spec.spacer_line==0);
    assert(nd_theme_load(".",ND_THEME_EMERALD,&spec)==1);
    assert(spec.rgb[7][0]==240);
    nd_theme_defaults(ND_THEME_WORKBENCH,&spec);
    assert(spec.shape==ND_SHAPE_SQUARE && spec.side==ND_SIDE_BOTTOM);
    assert(spec.position==ND_POSITION_CENTER && spec.font_mode==ND_FONT_TOPAZ);
    assert(nd_theme_load(".",ND_THEME_NEXTSTEP,&spec)==1);
    assert(!strcmp(spec.name,"NeXTSTEP") && spec.shape==ND_SHAPE_SQUARE);
    assert(spec.side==ND_SIDE_RIGHT && spec.position==ND_POSITION_CENTER);
    assert(spec.label_mode==ND_LABEL_ALWAYS);
    assert(nd_theme_load(".",ND_THEME_BEOS,&spec)==1);
    assert(!strcmp(spec.name,"BeOS") && spec.shape==ND_SHAPE_CHAMFER);
    assert(spec.rgb[5][0]==250 && spec.label_mode==ND_LABEL_ALWAYS);
    assert(nd_theme_load(".",ND_THEME_NORD,&spec)==1);
    assert(!strcmp(spec.name,"Nord") && spec.shape==ND_SHAPE_ROUNDED);
    assert(spec.rgb[7][1]==190);
    assert(nd_theme_load(".",ND_THEME_VAPOR,&spec)==1);
    assert(!strcmp(spec.name,"Vapor") && spec.shape==ND_SHAPE_MINIMAL);
    assert(spec.spacer_line_width==2 && spec.font_mode==ND_FONT_BOLD);
    /* BEVEL is a theme key: on for Workbench, off by default, and a
     * third-party theme can turn it on. */
    assert(nd_theme_load(".",ND_THEME_WORKBENCH,&spec)==1 && spec.bevel==1);
    assert(nd_theme_load(".",ND_THEME_OCEAN,&spec)==1 && spec.bevel==0);
    nd_theme_defaults_id("BevelTest",&spec);
    assert(spec.bevel==0);
    {
        FILE *f;
        assert(system("mkdir -p tests/nd_theme_tmp/Themes/BevelTest")==0);
        f=fopen("tests/nd_theme_tmp/Themes/BevelTest/theme.cfg","w");
        assert(f);
        fputs("NAME=Bevel Test\nBEVEL=YES\nBG=10,20,30\n",f);
        fclose(f);
        assert(nd_theme_load_id("tests/nd_theme_tmp","BevelTest",&spec)==1);
        assert(spec.bevel==1 && !strcmp(spec.name,"Bevel Test") && spec.rgb[0][2]==30);
        assert(system("rm -rf tests/nd_theme_tmp")==0);
    }
    puts("Theme pack configuration passed.");
    return 0;
}
