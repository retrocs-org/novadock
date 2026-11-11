#define ND_INSTALLER_TEST
#include "../tools/startinstaller.c"
#include <assert.h>
int main(void)
{
    char out[100],small[4];
    assert(quoted(out,sizeof(out),"Work:Install Folder/Install-NovaDock"));
    assert(!strcmp(out,"\"Work:Install Folder/Install-NovaDock\""));
    assert(quoted(out,sizeof(out),"Work:Star*And\"Quote"));
    assert(!strcmp(out,"\"Work:Star**And*\"Quote\""));
    assert(!quoted(out,sizeof(out),"Work:bad\ncommand"));
    assert(!quoted(small,sizeof(small),"long"));
    assert(script_in_drawer(out,sizeof(out),"Bin:NovaDock/"));
    assert(!strcmp(out,"Bin:NovaDock/Install-NovaDock"));
    assert(script_in_drawer(out,sizeof(out),"Bin:NovaDock"));
    assert(!strcmp(out,"Bin:NovaDock/Install-NovaDock"));
    assert(script_in_drawer(out,sizeof(out),"Work:"));
    assert(!strcmp(out,"Work:Install-NovaDock"));
    assert(!script_in_drawer(out,sizeof(out),""));
    assert(!script_in_drawer(small,sizeof(small),"Bin:NovaDock"));
    puts("Installer launcher path quoting and bounds passed.");
    return 0;
}
