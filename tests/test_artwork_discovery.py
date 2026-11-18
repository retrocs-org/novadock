#!/usr/bin/env python3
"""Exercise real catalogue health checks with valid, absent and damaged banks."""
import os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def function(text,signature):
 a=text.index(signature);b=text.index('{',a);n=1;i=b+1
 while n:n+=(text[i]=='{')-(text[i]=='}');i+=1
 return text[a:i]
s=(ROOT/'prefs/novadockdiscovery.c').read_text()
h=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "nd_plugin_discovery.h"
#include "nd_artwork.h"
static int is_file(const char *p){struct stat s;return !stat(p,&s)&&S_ISREG(s.st_mode);}
static int read_icon_manifest(const char *p,struct NDManifest *m){(void)p;(void)m;return -1;}
'''+function(s,'static int read_text_manifest(')+'\n'+s[s.index('typedef int (*NDCatalogSink)'):s.index('static void catalog_drawer(')]+r'''
static struct NDPluginCatalogEntry result;
static int sink(void *ctx,const struct NDPluginCatalogEntry *e){(void)ctx;result=*e;return 1;}
static void manifest(const char *features){FILE *f=fopen("Plugins/Foo.ndplugin","w");assert(f);fprintf(f,"NOVADOCK_ID=FOO\nNOVADOCK_PROGRAM=Foo\nNOVADOCK_NAME=Example\nNOVADOCK_VERSION=1.0\nNOVADOCK_FEATURES=%s\n",features);assert(!fclose(f));}
static int scan(int diagnostics){static struct CatalogWalk w;memset(&w,0,sizeof(w));memset(&result,0,sizeof(result));w.maximum=ND_PLUGIN_CATALOG_MAX;w.diagnostics=diagnostics;w.sink=sink;catalog_consider(&w,"Plugins","Foo",1);return w.count;}
static void bank(const unsigned char *d,unsigned long n){FILE *f=fopen("Plugins/Foo.assets/Artwork.ndart","wb");assert(f);assert(fwrite(d,1,n,f)==n);assert(!fclose(f));}
int main(int argc,char **argv){FILE *f;struct NDArtwork a;unsigned char *d;unsigned long n,offset;unsigned int flags,count,i;char why[96];
 assert(argc==2);memset(&a,0,sizeof(a));assert(nd_artwork_load(&a,argv[1]));n=a.size;d=malloc(n);assert(d);memcpy(d,a.data,n);
 assert(a.data[5]==2&&nd_artwork_header(a.data)==72);assert(nd_artwork_key_frame(&a,nd_artwork_key("weather"),0,&count,&flags)&&count&&flags==ND_ARTWORK_VALUE);
 for(i=0;i<16;++i)assert(a.data[56+i]<=5);
 d[56]=1;assert(!nd_artwork_validate(d,n));d[56]=0;d[72+25]=128;assert(!nd_artwork_validate(d,n));memcpy(d,a.data,n);
 offset=nd_artwork_u32(d+72+28);d[offset+4]=16;assert(!nd_artwork_validate(d,n));memcpy(d,a.data,n);
 assert(nd_artwork_feature("click, artwork, badge"));assert(!nd_artwork_feature("notartwork,artworkish"));assert(!nd_artwork_feature("click, badge  "));
 assert(!mkdir("Plugins",0700));assert(!mkdir("Plugins/Foo.assets",0700));f=fopen("Plugins/Foo","w");assert(f);fputs("program",f);fclose(f);
 manifest("click,artwork");assert(scan(1)==1&&strstr(result.problem,"missing"));assert(scan(0)==0);
 bank(d,n);assert(scan(1)==1&&!result.problem[0]);assert(scan(0)==1);
 d[0]='X';bank(d,n);assert(scan(1)==1&&strstr(result.problem,"invalid"));assert(scan(0)==0);d[0]='N';
 d[5]=3;bank(d,n);assert(scan(1)==1&&strstr(result.problem,"update NovaDock"));assert(scan(0)==0);d[5]=2;
 bank(d,n);assert(scan(1)==1&&!result.problem[0]); /* Restoring package repairs catalogue state. */
 assert(!remove("Plugins/Foo.assets/Artwork.ndart"));manifest("click");assert(scan(0)==1);
 assert(!nd_artwork_package_problem("Plugins/Foo","notartwork",why,sizeof(why)));
 manifest("artwork");assert(scan(0)==0);free(d);nd_artwork_close(&a);return 0;}
'''
with tempfile.TemporaryDirectory(prefix='novadock-art-health-') as tmp:
 p=Path(tmp);(p/'health.c').write_text(h)
 cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'health.c'),str(ROOT/'src/nd_manifest.c'),str(ROOT/'src/config.c'),str(ROOT/'src/nd_options.c'),'-o',str(p/'health')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'health'),str(ROOT/'Plugins/Weather.assets/Artwork.ndart')],cwd=p,check=True)
print('Artwork catalogue: missing/invalid/newer banks need repair; healthy/restored/optional banks and v2 theme/layout validation pass.')
