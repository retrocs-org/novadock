#include "nd_interface_list.h"
#include <assert.h>
static struct NDInterfaceChoices list;
int main(void)
{
    struct NDInterfaceChoice a,b;FILE *f;int i;
    f=fopen("tests/interface-choice.tmp","w");assert(f);
    fputs("# profile\ndevice = \"DEVS:Networks/a2065.device\"\nunit=2 # comment\nconfigure=dhcp\n",f);fclose(f);
    assert(nd_interface_choice_read(&a,"A2065","tests/interface-choice.tmp",1));
    assert(!strcmp(a.device,"DEVS:Networks/a2065.device") && a.unit==2 && a.storage);
    assert(!nd_interface_choice_read(&b,"a2065.device","tests/interface-choice.tmp",0));
    assert(!nd_interface_choice_read(&b,"A2065.info","tests/interface-choice.tmp",0));
    assert(!nd_interface_name_valid("bad;command") && !nd_interface_name_valid("AUTO"));
    assert(nd_interface_choice_add(&list,&a));
    b=a;strcpy(b.name,"a2065");b.storage=0;strcpy(b.device,"preferred.device");
    assert(nd_interface_choice_add(&list,&b) && list.count==1 && !list.entry[0].storage);
    assert(nd_interface_choice_add(&list,&a) && !strcmp(list.entry[0].device,"preferred.device"));
    b=a;strcpy(b.name,"ZZ9000Net");assert(nd_interface_choice_add(&list,&b));
    assert(!strcmp(list.entry[0].name,"a2065") && !strcmp(list.entry[1].name,"ZZ9000Net"));
    f=fopen("tests/interface-choice.tmp","w");assert(f);fputs("unit=0\n",f);fclose(f);
    assert(!nd_interface_choice_read(&b,"BadProfile","tests/interface-choice.tmp",0));
    f=fopen("tests/interface-choice.tmp","w");assert(f);fputs("device=test.device\nunit=-1\n",f);fclose(f);
    assert(!nd_interface_choice_read(&b,"BadProfile","tests/interface-choice.tmp",0));
    memset(&list,0,sizeof(list));b=a;
    for(i=0;i<ND_INTERFACE_CHOICES;++i){sprintf(b.name,"P%03d",i);assert(nd_interface_choice_add(&list,&b));}
    strcpy(b.name,"ZZZ");assert(!nd_interface_choice_add(&list,&b) && list.truncated);
    remove("tests/interface-choice.tmp");
    puts("Interface profiles: metadata, safe names, sorted deduplication, DEVS priority and bounds passed.");
    return 0;
}
