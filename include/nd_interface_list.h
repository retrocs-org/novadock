#ifndef ND_INTERFACE_LIST_H
#define ND_INTERFACE_LIST_H
/* Read-only, bounded profile list shared by native Preferences and host checks. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#define ND_INTERFACE_CHOICES 64
struct NDInterfaceChoice {char name[16],device[96];unsigned long unit;int storage;};
struct NDInterfaceChoices {struct NDInterfaceChoice entry[ND_INTERFACE_CHOICES];int count,truncated;};
static int nd_interface_compare(const char *a,const char *b)
{
    unsigned char x,y;
    do{x=(unsigned char)*a++;y=(unsigned char)*b++;
        if(x>='a' && x<='z')x=(unsigned char)(x-'a'+'A');
        if(y>='a' && y<='z')y=(unsigned char)(y-'a'+'A');
        if(x!=y)return (int)x-(int)y;
    }while(x);
    return 0;
}
static int nd_interface_name_valid(const char *name)
{
    unsigned int i,n=(unsigned int)strlen(name);
    if(!n || n>15 || !nd_interface_compare(name,"AUTO"))return 0;
    for(i=0;i<n;++i){unsigned char c=(unsigned char)name[i];
        if(!((c>='A' && c<='Z') || (c>='a' && c<='z') ||
             (c>='0' && c<='9') || c=='_' || c=='-'))return 0;
    }
    return 1;
}
static int nd_interface_choice_read(struct NDInterfaceChoice *out,const char *name,
                                    const char *path,int storage)
{
    FILE *f;char line[384],*key,*value,*end,*tail;int ch;
    if(!nd_interface_name_valid(name))return 0;
    f=fopen(path,"r");if(!f)return 0;
    memset(out,0,sizeof(*out));strcpy(out->name,name);out->storage=storage;
    while(fgets(line,sizeof(line),f)){
        if(!strchr(line,'\n') && !feof(f)){while((ch=fgetc(f))!='\n' && ch!=EOF){}continue;}
        key=line;while(isspace((unsigned char)*key))++key;
        if(!*key || *key=='#' || *key==';')continue;
        value=strchr(key,'=');if(!value)continue;*value++=0;
        end=key+strlen(key);while(end>key && isspace((unsigned char)end[-1]))*--end=0;
        while(isspace((unsigned char)*value))++value;
        if(*value=='"'){++value;end=strchr(value,'"');if(!end){fclose(f);return 0;}*end=0;}
        else{end=value;while(*end && !isspace((unsigned char)*end) && *end!='#' && *end!=';')++end;*end=0;}
        if(!nd_interface_compare(key,"DEVICE")){
            if(!*value || strlen(value)>=sizeof(out->device)){fclose(f);return 0;}
            for(end=value;*end;++end)if((unsigned char)*end<32 || (unsigned char)*end==127){fclose(f);return 0;}
            strcpy(out->device,value);
        }else if(!nd_interface_compare(key,"UNIT")){
            out->unit=strtoul(value,&tail,10);
            if(!isdigit((unsigned char)*value) || *tail || out->unit>255){fclose(f);return 0;}
        }
    }
    ch=ferror(f);fclose(f);return !ch && out->device[0];
}
static int nd_interface_choice_add(struct NDInterfaceChoices *choices,
                                   const struct NDInterfaceChoice *entry)
{
    int i,j;
    for(i=0;i<choices->count;++i){
        int order=nd_interface_compare(choices->entry[i].name,entry->name);
        if(!order){if(choices->entry[i].storage && !entry->storage)choices->entry[i]=*entry;return 1;}
        if(order>0)break;
    }
    if(choices->count>=ND_INTERFACE_CHOICES){choices->truncated=1;return 0;}
    for(j=choices->count;j>i;--j)choices->entry[j]=choices->entry[j-1];
    choices->entry[i]=*entry;++choices->count;return 1;
}
#endif
