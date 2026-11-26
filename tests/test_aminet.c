#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_aminet.h"
static FILE *contents(const char *s)
{ FILE *f=tmpfile();assert(f);assert(fputs(s,f)>=0);rewind(f);return f; }
static void row(FILE *f,int n,const char *package)
{
    fprintf(f,"<tr class=\"lightrow pkg_row\"><td class=\"name_col\"><a href=\"/%s\">package%d</a></td><td><a href=\"/package/util/misc/pkg%d\">A &amp; B &#60;test&#62; &#27; %d</a></td></tr>\n",package,n,n,n);
}
int main(void)
{
    char out[400],package[100];long length;FILE *f;struct NDAminetResults r;struct NDAminetDetails d;int i;
    assert(nd_aminet_percent(0,0)==-1);
    assert(nd_aminet_percent(0,100)==0 && nd_aminet_percent(1,3)==33);
    assert(nd_aminet_percent(2,3)==66 && nd_aminet_percent(3,3)==100);
    assert(nd_aminet_percent(67108863UL,67108864UL)==99);
    assert(nd_aminet_percent(2147483647UL,4294967295UL)==49);
    assert(nd_aminet_percent(4294967295UL,4294967295UL)==100);
    assert(nd_aminet_path_valid("util/wb/Beam68-0.1.lha"));
    assert(nd_aminet_path_valid("mods/pro/a+b_1992.mod"));
    assert(!nd_aminet_path_valid("../wb/package.lha"));
    assert(!nd_aminet_path_valid("util/wb/../evil.lha"));
    assert(!nd_aminet_path_valid("/util/wb/package.lha"));
    assert(!nd_aminet_path_valid("util//package.lha"));
    assert(!nd_aminet_path_valid("util/wb/"));
    assert(!nd_aminet_path_valid("util/wb/package.lha\n"));
    assert(!nd_aminet_path_valid("util/wb/a*\".lha"));
    assert(!nd_aminet_path_valid("util/wb/a%2f..lha"));
    memset(out,'a',sizeof(out));out[sizeof(out)-1]=0;assert(!nd_aminet_path_valid(out));
    assert(nd_aminet_encode_query("demo & game+1992",out,sizeof(out)));
    assert(!strcmp(out,"demo%20%26%20game%2B1992"));
    assert(!nd_aminet_encode_query("a",out,sizeof(out)));
    assert(!nd_aminet_encode_query("ab\r\nInjected",out,sizeof(out)));
    assert(!nd_aminet_encode_query("hello",out,3));
    assert(nd_aminet_readme_path("util/misc/package.tar.gz",out,sizeof(out)));
    assert(!strcmp(out,"/util/misc/package.readme"));
    assert(nd_aminet_readme_path("util/wb/Beam68-0.1.lha",out,sizeof(out)));
    assert(!strcmp(out,"/util/wb/Beam68-0.1.readme"));
    assert(!nd_aminet_readme_path("../wb/evil.lha",out,sizeof(out)));
    nd_aminet_plain(" <b>Hello</b> &amp; &#10;there \033",out,sizeof(out));
    assert(!strcmp(out,"Hello & ?there"));
    f=tmpfile();assert(f);
    fputs("<table>\n<tr><a class=\"sort_link_name\">Name</a></tr>\n",f);
    row(f,0,"util/wb/../../evil.lha");
    for(i=0;i<50;i++) { snprintf(package,sizeof(package),"util/misc/package%d.lha",i);row(f,i,package); }
    fputs("</table>",f);rewind(f);
    assert(nd_aminet_search(f,0,&r));assert(r.count==50 && r.seen==50);
    assert(!strcmp(r.rows[0].path,"util/misc/package0.lha"));
    assert(strstr(r.rows[0].title,"A & B <test> ? 0"));
    rewind(f);assert(nd_aminet_search(f,48,&r));assert(r.count==2 && r.seen==50);
    assert(!strcmp(r.rows[0].path,"util/misc/package48.lha"));fclose(f);
    f=contents("Found 0 matching packages<br>");assert(nd_aminet_search(f,0,&r));assert(!r.count);fclose(f);
    f=contents("<html>Server failure</html>");assert(!nd_aminet_search(f,0,&r));fclose(f);
    f=contents("<tr class=\"pkg_row\"><td class=\"name_col\"><a href=\"/util/misc/x.lha\">x");assert(!nd_aminet_search(f,0,&r));fclose(f);
    f=contents("Short: Test package\nVersion: 1.2\nAuthor: Example\nArchitecture: m68k-amigaos >= 3.0\nRequires: 68020\n\nLong body\n");
    assert(nd_aminet_details(f,&d));assert(!strcmp(d.summary,"Test package"));assert(!strcmp(d.version,"1.2"));assert(!strcmp(d.requires,"68020"));fclose(f);
    f=contents("<html>not a readme</html>\n");assert(!nd_aminet_details(f,&d));fclose(f);
    assert(nd_aminet_http_header("HTTP/1.0 200 OK\r\nContent-Length: 123\r\n\r\n",1000,&length) && length==123);
    assert(nd_aminet_http_header("HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n",1000,&length) && length==-1);
    assert(nd_aminet_http_header("HTTP/1.1 200 OK\r\ncontent-length: 1000\r\nContent-Encoding: identity\r\n\r\n",1000,&length) && length==1000);
    /* Header names and the identity token are case-insensitive (RFC 9110). */
    assert(nd_aminet_http_header("HTTP/1.1 200 OK\r\nCONTENT-LENGTH: 7\r\nTRANSFER-ENCODING: Identity\r\n\r\n",1000,&length) && length==7);
    assert(!nd_aminet_http_header("HTTP/1.1 200 OK\r\ntransfer-encoding: CHUNKED\r\n\r\n",1000,&length));
    assert(!nd_aminet_http_header("HTTP/1.1 200 OK\r\ncontent-encoding: gzip\r\n\r\n",1000,&length));
    assert(!nd_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Length: 5\r\ncontent-length: 5\r\n\r\n",1000,&length));
    assert(!nd_aminet_http_header("HTTP/1.1 404 Missing\r\n\r\n",1000,&length));
    assert(!nd_aminet_http_header("HTTP/1.1 301 Moved\r\nLocation: http://evil/\r\n\r\n",1000,&length));
    assert(!nd_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Length: 1001\r\n\r\n",1000,&length));
    assert(!nd_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Length: 9999999999999999999\r\n\r\n",1000,&length));
    assert(!nd_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Length: -1\r\n\r\n",1000,&length));
    assert(!nd_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Length: 5abc\r\n\r\n",1000,&length));
    assert(!nd_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Length: 5\r\n\r\n",1000,&length));
    assert(!nd_aminet_http_header("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n",1000,&length));
    assert(!nd_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Encoding: gzip\r\n\r\n",1000,&length));
    assert(!nd_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Length: 5\r\n",1000,&length));
    assert(nd_aminet_archive_header("util/misc/a.zip",(const unsigned char *)"PK\003\004",4));
    assert(!nd_aminet_archive_header("util/misc/a.zip",(const unsigned char *)"<html>",6));
    assert(nd_aminet_archive_header("util/misc/a.lha",(const unsigned char *)"xx-lh5-",7));
    assert(!nd_aminet_archive_header("util/misc/a.lha",(const unsigned char *)"xx-lh5-",6));
    assert(nd_aminet_archive_header("util/misc/a.tgz",(const unsigned char *)"\037\213\010",3));
    assert(!nd_aminet_archive_header("util/misc/a.run",(const unsigned char *)"xx-lh5-",7));
    assert(nd_aminet_category_valid("util/wb") && nd_aminet_category_valid("mods"));
    assert(!nd_aminet_category_valid("util/../x") && !nd_aminet_category_valid("/util") && !nd_aminet_category_valid("util/wb/extra"));
    f=contents("<a href=\"/tree/util/wb\">Workbench</a><a href=\"/util/boot\">Boot</a><a href=\"/util/wb\">duplicate</a><a href=\"/game/misc\">other</a>");
    assert(nd_aminet_categories(f,"util",&r) && r.count==2);fclose(f);
    puts("Aminet paths, URL encoding, bounded HTML/readme parsing, paging and HTTP headers: PASS");return 0;
}
