/* nd_config_get(): every global key is readable, formats exactly as the
 * configuration file stores it, and parses back to the same value (the
 * contract ARexx GET/SET relies on). */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_config.h"
int main(void)
{
    static struct NDConfig a,b;
    char value[ND_CONFIG_VALUE_LEN],again[ND_CONFIG_VALUE_LEN],line[400],tiny[2];
    int i,keys=0;
    nd_default_config(&a);
    a.auto_hide=1;a.hide_delay=4;a.row_layout=1;a.row_counts[0]=3;a.row_counts[1]=2;
    strcpy(a.dock_hotkey,"ctrl alt d");
    for (i=0;nd_config_keys[i];++i,++keys) {
        assert(nd_config_get(&a,nd_config_keys[i],value,sizeof(value)));
        if (!strcmp(nd_config_keys[i],"RUNNING_SECTION") || !strcmp(nd_config_keys[i],"PROFILE"))
            assert(!nd_config_settable(&a,nd_config_keys[i]));
        else assert(nd_config_settable(&a,nd_config_keys[i]));
        if (!strcmp(nd_config_keys[i],"RUNNING_SECTION")) continue;
        b=a;
        sprintf(line,"%s=%s",nd_config_keys[i],value);
        assert(nd_parse_line(&b,line));
        assert(nd_config_get(&b,nd_config_keys[i],again,sizeof(again)));
        assert(!strcmp(value,again));
    }
    assert(keys>=30);
    /* Unknown keys, item keywords and short buffers are refused. */
    assert(!nd_config_get(&a,"LAUNCH",value,sizeof(value)));
    assert(!nd_config_get(&a,"NOPE",value,sizeof(value)));
    assert(!nd_config_settable(&a,"PLUGIN"));
    assert(!nd_config_get(&a,"SIDE",tiny,sizeof(tiny)));
    assert(!nd_config_get(&a,"OPTION.",value,sizeof(value)));
    assert(!nd_config_get(&a,"OPTION.CLOCK",value,sizeof(value)));
    /* Keys are matched without regard to case and returned as stored. */
    assert(nd_config_canonical_key(&a,"side",value,sizeof(value)) && !strcmp(value,"SIDE"));
    assert(nd_config_canonical_key(&a,"Hover_Magnification",value,sizeof(value)) &&
           !strcmp(value,"HOVER_MAGNIFICATION"));
    b=a;(void)nd_parse_line(&b,"PLUGIN=Clock|Plugins/Clock|CLOCK|METER|ON|NO"); /* the default may have it */
    assert(nd_parse_line(&b,"OPTION.CLOCK.FORMAT=ANALOG"));
    assert(nd_config_canonical_key(&b,"option.clock.format",value,sizeof(value)) &&
           !strcmp(value,"OPTION.CLOCK.FORMAT"));
    assert(nd_config_get(&b,value,again,sizeof(again)) && !strcmp(again,"ANALOG"));
    assert(!nd_config_canonical_key(&b,"option.nosuch.format",value,sizeof(value)));
    assert(!nd_config_canonical_key(&b,"option.clock",value,sizeof(value)));
    assert(!nd_config_canonical_key(&b,"OPTION.",value,sizeof(value)));
    assert(!nd_config_canonical_key(&b,"side",tiny,sizeof(tiny)));
    /* Profile names and paths. */
    assert(nd_profile_name_valid("Work") && nd_profile_name_valid("Late Night-2_x"));
    assert(!nd_profile_name_valid("") && !nd_profile_name_valid(" Work") && !nd_profile_name_valid("Work "));
    assert(!nd_profile_name_valid("a/b") && !nd_profile_name_valid("a:b") && !nd_profile_name_valid("a.cfg"));
    assert(nd_profile_name_valid("123456789012345678901234") && !nd_profile_name_valid("1234567890123456789012345"));
    assert(nd_profile_path("Games",line,sizeof(line)) && !strcmp(line,"ENVARC:NovaDock/Profiles/Games.cfg"));
    assert(!nd_profile_path("x/y",line,sizeof(line)) && !nd_profile_path("Games",tiny,sizeof(tiny)));
    b=a;assert(nd_parse_line(&b,"PROFILE=Games") && !strcmp(b.profile,"Games"));
    assert(!nd_parse_line(&b,"PROFILE=../x") && !strcmp(b.profile,"Games"));
    assert(nd_parse_line(&b,"PROFILE=") && !b.profile[0]);
    /* Invalid values are rejected by the parser SET uses. */
    b=a;assert(!nd_parse_line(&b,"SIDE=SIDEWAYS"));
    assert(nd_config_get(&b,"SIDE",value,sizeof(value)) && strcmp(value,"SIDEWAYS"));
    assert(!nd_parse_line(&b,"HIDE_DELAY=999"));
    /* The writer and the getter agree: the file lists the same values. */
    assert(nd_write_config(&a,"config-get.cfg"));
    {FILE *f=fopen("config-get.cfg","r");int found=0;assert(f);
     while (fgets(line,sizeof(line),f)) if (!strcmp(line,"DOCK_HOTKEY=ctrl alt d\n") || !strcmp(line,"ROW_COUNTS=3,2,0\n")) ++found;
     fclose(f);assert(found==2);}
    remove("config-get.cfg");remove("config-get.cfg.bak");remove("config-get.cfg.old");remove("config-get.cfg.good");
    printf("Config GET: %d global keys read, written and parsed back identically; case-insensitive keys; invalid keys, values and buffers refused.\n",keys);
    return 0;
}
