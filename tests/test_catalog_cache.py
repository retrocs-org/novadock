"""Real cache format: fingerprints, invalidation, checksum and fallback."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
code=r'''
#include <assert.h>
#include <unistd.h>
#include "nd_catalog_cache.h"
static void put(const char *path,const char *text){FILE *f=fopen(path,"wb");assert(f);fputs(text,f);fclose(f);}
int main(void){unsigned char raw[1000],packed[1100],copy[1000];size_t n;unsigned int i;struct NDPluginCatalogEntry e,out;struct NDCacheStamp stamps[4];char path[80];FILE *f;memset(raw,0,sizeof(raw));n=nd_cache_pack(raw,sizeof(raw),packed,sizeof(packed));assert(n<16&&nd_cache_unpack(packed,n,copy,sizeof(copy))&&!memcmp(raw,copy,sizeof(raw)));for(i=0;i<sizeof(raw);++i)raw[i]=(unsigned char)(i*17);n=nd_cache_pack(raw,sizeof(raw),packed,sizeof(packed));assert(nd_cache_unpack(packed,n,copy,sizeof(copy))&&!memcmp(raw,copy,sizeof(raw)));assert(!nd_cache_unpack(packed,n-1,copy,sizeof(copy)));assert(!nd_cache_unpack(packed,n,copy,10));assert(!nd_cache_pack(raw,sizeof(raw),packed,2));{unsigned char *buffer=malloc(sizeof(e)+sizeof(e)/128+6);assert(buffer);memset(&e,0,sizeof(e));e.manifest.option_count=1;strcpy(e.manifest.options[0].key,"ACTIVE");memset(&e.manifest.options[1],0x5a,sizeof(e.manifest.options[1]));strcpy(e.command,"kept-tail");n=nd_cache_pack_entry(&e,buffer,sizeof(e)+sizeof(e)/128+6);assert(n&&nd_cache_unpack(buffer,n,&out,sizeof(out)));assert(!strcmp(out.manifest.options[0].key,"ACTIVE")&&!out.manifest.options[1].id[0]&&!strcmp(out.command,"kept-tail"));e.manifest.option_count=ND_MANIFEST_MAX_OPTIONS;strcpy(e.manifest.options[ND_MANIFEST_MAX_OPTIONS-1].key,"LAST");n=nd_cache_pack_entry(&e,buffer,sizeof(e)+sizeof(e)/128+6);assert(n&&nd_cache_unpack(buffer,n,&out,sizeof(out))&&!strcmp(out.manifest.options[ND_MANIFEST_MAX_OPTIONS-1].key,"LAST"));e.manifest.option_count=ND_MANIFEST_MAX_OPTIONS+1;assert(!nd_cache_pack_entry(&e,buffer,sizeof(e)+sizeof(e)/128+6));free(buffer);}memset(&e,0,sizeof(e));strcpy(e.command,"Plugin");strcpy(e.manifest.id,"TEST");strcpy(e.manifest.name,"Example");put("Plugin","program");put("Plugin.ndplugin","manifest");assert(nd_cache_sources("Plugin",stamps));assert(!nd_catalog_cached("Plugin",&out,stamps));nd_catalog_remember("Plugin",&e,stamps);assert(nd_catalog_cached("Plugin",&out,stamps)&&!strcmp(out.manifest.id,"TEST"));
 put("Plugin.ndplugin","changed manifest");assert(nd_cache_sources("Plugin",stamps));assert(!nd_catalog_cached("Plugin",&out,stamps));nd_catalog_remember("Plugin",&e,stamps);assert(nd_catalog_cached("Plugin",&out,stamps));
 mkdir("Plugin.assets",0700);put("Plugin.assets/Artwork.ndart","artwork");assert(nd_cache_sources("Plugin",stamps)&&!nd_catalog_cached("Plugin",&out,stamps));nd_catalog_remember("Plugin",&e,stamps);assert(nd_catalog_cached("Plugin",&out,stamps));
 put("Plugin.info","icon");assert(nd_cache_sources("Plugin",stamps)&&!nd_catalog_cached("Plugin",&out,stamps));nd_catalog_remember("Plugin",&e,stamps);remove("Plugin");assert(nd_cache_sources("Plugin",stamps)&&!nd_catalog_cached("Plugin",&out,stamps));put("Plugin","new executable");assert(nd_cache_sources("Plugin",stamps));nd_catalog_remember("Plugin",&e,stamps);
 nd_cache_name("Plugin",path);f=fopen(path,"r+b");assert(f);fseek(f,-1,SEEK_END);fputc(7,f);fclose(f);assert(!nd_catalog_cached("Plugin",&out,stamps));put(path,"NDCAT02");assert(!nd_catalog_cached("Plugin",&out,stamps));remove(path);assert(!nd_catalog_cached("Plugin",&out,stamps));puts("Catalogue cache: warm hits, manifest/program/icon/artwork invalidation, corruption and missing-cache fallbacks passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(code)
 cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'test.c'),'-o',str(p/'test')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'test')],cwd=p,check=True)
