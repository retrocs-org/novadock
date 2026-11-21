/* Run the installed-file validation used by Preferences on packaged plugins.
 * Portable for the release gate; also buildable for native Amiga QA. */
#include <stdio.h>
#include <string.h>
#include "nd_manifest_file.h"
#include "nd_artwork.h"
long __stack=65536;
static struct NDManifest manifest;
static char ids[128][20];
static int checked;
static int check(const char *path)
{
 char program[ND_ARTWORK_PATH],why[128];
 unsigned char magic[4];FILE *file;size_t length;int i;
 length=strlen(path);
 if(length<=strlen(ND_MANIFEST_SUFFIX) || length>=sizeof(program) ||
    strcmp(path+length-strlen(ND_MANIFEST_SUFFIX),ND_MANIFEST_SUFFIX)){
  printf("FAIL %s: invalid manifest filename\n",path);return 0;
 }
 strcpy(program,path);program[length-strlen(ND_MANIFEST_SUFFIX)]=0;
 nd_manifest_init(&manifest,program);
 if(!nd_manifest_read_text(path,&manifest) || !nd_manifest_finish(&manifest)){
  printf("FAIL %s: %s\n",path,manifest.error);return 0;
 }
 for(i=0;i<checked;++i)if(!strcmp(ids[i],manifest.id)){
  printf("FAIL %s: duplicate plugin ID %s\n",path,manifest.id);return 0;
 }
 if(checked>=128){puts("FAIL: too many plugin manifests");return 0;}
 strcpy(ids[checked++],manifest.id);
 file=fopen(program,"rb");
 if(!file){printf("FAIL %s: plugin program is missing or unreadable\n",program);return 0;}
 i=fread(magic,1,4,file)==4 && !memcmp(magic,"\0\0\3\363",4);
 fclose(file);
 if(!i){printf("FAIL %s: not an Amiga Hunk executable\n",program);return 0;}
 if(nd_artwork_package_problem(program,manifest.features,why,sizeof(why))){
  printf("FAIL %s: %s\n",program,why);return 0;
 }
 printf("PASS %s (%s)\n",manifest.name,manifest.id);return 1;
}
int main(int argc,char **argv)
{
 int i,ok=1;
 if(argc<2){puts("Usage: PluginHealth manifest.ndplugin [...]");return 10;}
 for(i=1;i<argc;++i)if(!check(argv[i]))ok=0;
 if(ok)printf("Verified %d plugins: none need repair.\n",checked);
 return ok ? 0:10;
}
