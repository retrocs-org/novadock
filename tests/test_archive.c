#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_archive.h"

int main(void)
{
    char destination[260],command[820],output[300];
    assert(nd_archive_kind("Work:Download/Demo.ZIP")==ND_ARCHIVE_ZIP);
    assert(nd_archive_kind("Work:Download/Demo.lha")==ND_ARCHIVE_LHA);
    assert(nd_archive_kind("Work:Download/Demo.LZH")==ND_ARCHIVE_LHA);
    assert(nd_archive_kind("Work:Download/Demo.rar")==ND_ARCHIVE_RAR);
    assert(nd_archive_kind("Work:Download/Demo.arj")==ND_ARCHIVE_ARJ);
    assert(nd_archive_kind("Work:Download/Demo.LZX")==ND_ARCHIVE_LZX);
    assert(nd_archive_kind("Work:Download/Demo.tar.gz")==ND_ARCHIVE_TGZ);
    assert(nd_archive_kind("Work:Download/Demo.TGZ")==ND_ARCHIVE_TGZ);
    assert(nd_archive_kind("Work:Download/Demo.tar")==ND_ARCHIVE_TAR);
    assert(nd_archive_kind("Work:Download/readme.txt.gz")==ND_ARCHIVE_GZ);
    assert(nd_archive_kind("Work:Download/Demo.gz/")==ND_ARCHIVE_NONE);
    assert(nd_archive_kind("Work:Download/.lzx")==ND_ARCHIVE_NONE);
    assert(nd_archive_kind("Work:Download/Demo.adf")==ND_ARCHIVE_NONE);
    assert(nd_archive_destination("Work:Download/My Demo.zip",destination,
                                  sizeof(destination)));
    assert(!strcmp(destination,"Work:Download/My Demo"));
    assert(nd_archive_destination("RAM:demo.lha",destination,sizeof(destination)));
    assert(!strcmp(destination,"RAM:demo"));
    assert(nd_archive_destination("RAM:src-1.0.tar.gz",destination,sizeof(destination)));
    assert(!strcmp(destination,"RAM:src-1.0"));
    assert(!nd_archive_destination("RAM:.zip",destination,sizeof(destination)));
    assert(!nd_archive_safe_path("RAM:bad*name.zip"));
    assert(!nd_archive_safe_path("RAM:bad`name.zip"));
    assert(nd_archive_command(ND_ARCHIVE_ZIP,"C:UnZip","RAM:demo.zip",
                              "RAM:demo",command,sizeof(command)));
    assert(!strcmp(command,"\"C:UnZip\" -o \"RAM:demo.zip\" -d \"RAM:demo\""));
    assert(nd_archive_command(ND_ARCHIVE_LHA,"C:LhA","RAM:demo.lha",
                              "RAM:demo",command,sizeof(command)));
    assert(!strcmp(command,"\"C:LhA\" x \"RAM:demo.lha\" \"RAM:demo/\""));
    assert(nd_archive_command(ND_ARCHIVE_RAR,"C:UnRAR","RAM:demo.rar",
                              "RAM:demo",command,sizeof(command)));
    assert(strstr(command," x -o+ ")!=NULL);
    assert(nd_archive_command(ND_ARCHIVE_ARJ,"C:ARJ","RAM:demo.arj",
                              "RAM:demo",command,sizeof(command)));
    assert(strstr(command," x -y ")!=NULL);
    assert(nd_archive_command(ND_ARCHIVE_ARJ,"C:UnARJ","RAM:demo.arj",
                              "RAM:demo",command,sizeof(command)));
    assert(!strcmp(command,"CD \"RAM:demo\"\n\"C:UnARJ\" x \"RAM:demo.arj\""));
    assert(nd_archive_command(ND_ARCHIVE_LZX,"C:LZX","RAM:demo.lzx",
                              "RAM:demo",command,sizeof(command)));
    assert(!strcmp(command,"\"C:LZX\" x \"RAM:demo.lzx\" \"RAM:demo/\""));
    assert(nd_archive_command(ND_ARCHIVE_LZX,"C:unlzx","RAM:demo.lzx",
                              "RAM:demo",command,sizeof(command)));
    assert(!strcmp(command,"CD \"RAM:demo\"\n\"C:unlzx\" -x \"RAM:demo.lzx\""));
    assert(nd_archive_command(ND_ARCHIVE_TGZ,"C:tar","RAM:src.tgz",
                              "RAM:src",command,sizeof(command)));
    assert(!strcmp(command,"CD \"RAM:src\"\n\"C:tar\" -xzvf \"RAM:src.tgz\""));
    assert(nd_archive_command(ND_ARCHIVE_TAR,"C:tar","RAM:src.tar",
                              "RAM:src",command,sizeof(command)));
    assert(strstr(command,"-xvf")!=NULL);
    assert(nd_archive_gz_output("RAM:notes.txt.gz","RAM:notes.txt",output,
                                sizeof(output)));
    assert(!strcmp(output,"RAM:notes.txt/notes.txt"));
    assert(nd_archive_command(ND_ARCHIVE_GZ,"C:gzip","RAM:notes.txt.gz",
                              "RAM:notes.txt",command,sizeof(command)));
    assert(!strcmp(command,
        "\"C:gzip\" >\"RAM:notes.txt/notes.txt\" -dc \"RAM:notes.txt.gz\""));
    assert(!nd_archive_command(ND_ARCHIVE_LAST+1,"C:x","RAM:a.zip","RAM:a",
                               command,sizeof(command)));
    assert(nd_archive_status_name(0x1234abcdUL,command,sizeof(command)));
    assert(!strcmp(command,"T:NovaDockArchive-1234abcd.status"));
    puts("Archive recognition, paths, and extractor commands passed.");

    assert(nd_archive_output_name("My Folder",ND_ARCHIVE_ZIP,output,sizeof(output)));
    assert(!strcmp(output,"My Folder.zip"));
    assert(nd_archive_output_name("My Folder.ZIP",ND_ARCHIVE_LHA,output,sizeof(output)));
    assert(!strcmp(output,"My Folder.lha"));
    assert(nd_archive_output_name("notes.txt",ND_ARCHIVE_ZIP,output,sizeof(output)));
    assert(!strcmp(output,"notes.txt.zip"));
    assert(!nd_archive_output_name("../bad",ND_ARCHIVE_ZIP,output,sizeof(output)));
    assert(!nd_archive_output_name("$name",ND_ARCHIVE_ZIP,output,sizeof(output)));
    assert(!nd_archive_output_name("-option",ND_ARCHIVE_ZIP,output,sizeof(output)));
    assert(!nd_archive_output_name("foo",ND_ARCHIVE_RAR,output,sizeof(output)));
    assert(!nd_archive_output_name("foo",ND_ARCHIVE_ZIP,output,3));
    assert(nd_archive_output_allowed("Work:Folder",1,"Work:Folder backup.zip"));
    assert(!nd_archive_output_allowed("Work:Folder",1,"work:folder/inside.zip"));
    assert(!nd_archive_output_allowed("Work:foo.txt",0,"WORK:FOO.TXT"));
    assert(!nd_archive_output_allowed("Work:",1,"Work:inside.zip"));
    assert(nd_archive_compress_command(ND_ARCHIVE_ZIP,"C:Zip","Work:My Files/notes.txt",
                                       "RAM:notes.zip",0,command,sizeof(command)));
    assert(!strcmp(command,"CD \"Work:My Files\"\n\"C:Zip\" -r \"RAM:notes.zip\" \"notes.txt\""));
    assert(nd_archive_compress_command(ND_ARCHIVE_LHA,"C:LhA","Work:My Folder",
                                       "RAM:folder.lha",0,command,sizeof(command)));
    assert(!strcmp(command,"CD \"Work:\"\n\"C:LhA\" -r a \"RAM:folder.lha\" \"My Folder\""));
    assert(!nd_archive_compress_command(ND_ARCHIVE_ZIP,"C:Zip","Work:-r","RAM:x.zip",0,command,sizeof(command)));
    assert(!nd_archive_compress_command(ND_ARCHIVE_ZIP,"C:Zip","Work:","RAM:x.zip",0,command,sizeof(command)));
    assert(!nd_archive_compress_command(ND_ARCHIVE_ZIP,"C:Zip","Work:File","RAM:foo$bar.zip",0,command,sizeof(command)));
    assert(!nd_archive_compress_command(ND_ARCHIVE_ZIP,"C:Zip","Work:File","RAM:x.zip",0,command,4));
    assert(nd_archive_compress_command(ND_ARCHIVE_ZIP,"C:Zip","Work:My Files/notes.txt",
                                       "RAM:notes.zip",1,command,sizeof(command)));
    assert(!strcmp(command,"CD \"Work:My Files\"\n\"C:Zip\" -r \"RAM:notes.zip\" \"notes.txt\" \"notes.txt.info\""));
    assert(nd_archive_compress_command(ND_ARCHIVE_LHA,"C:LhA","Work:My Folder",
                                       "RAM:folder.lha",1,command,sizeof(command)));
    assert(!strcmp(command,"CD \"Work:\"\n\"C:LhA\" -r a \"RAM:folder.lha\" \"My Folder\" \"My Folder.info\""));
    assert(!nd_archive_compress_command(ND_ARCHIVE_ZIP,"C:Zip","Work:File",
                                        "RAM:x.zip",1,command,60));
    assert(nd_archive_detect("RAM:unknown",(const unsigned char *)"PK\003\004",4)==ND_ARCHIVE_ZIP);
    assert(nd_archive_detect("RAM:unknown",(const unsigned char *)"xx-lh5-",7)==ND_ARCHIVE_LHA);
    assert(nd_archive_detect("RAM:unknown",(const unsigned char *)"\037\213",2)==ND_ARCHIVE_GZ);
    assert(nd_archive_detect("RAM:unknown",NULL,0)==ND_ARCHIVE_NONE);
    assert(nd_archive_detect("RAM:demo.lha",NULL,0)==ND_ARCHIVE_LHA);
    assert(nd_archive_gz_output("RAM:unknown","RAM:unknown.unpacked",output,sizeof(output)));
    assert(!strcmp(output,"RAM:unknown.unpacked/unknown"));
    assert(nd_archive_extract_destination("Work:Download",1,"demo",output,sizeof(output)));
    assert(!strcmp(output,"Work:Download/demo"));
    assert(nd_archive_extract_destination("RAM:",1,"demo",output,sizeof(output)));
    assert(!strcmp(output,"RAM:demo"));
    assert(nd_archive_extract_destination("RAM:",0,NULL,output,sizeof(output)));
    assert(!strcmp(output,"RAM:"));
    assert(!nd_archive_extract_destination("RAM:",1,"..",output,sizeof(output)));
    assert(!nd_archive_extract_destination("RAM:",1,"a/b",output,sizeof(output)));
    assert(!nd_archive_extract_destination("RAM:",1,"bad#?",output,sizeof(output)));
    assert(!nd_archive_extract_destination("relative",0,NULL,output,sizeof(output)));
    assert(!nd_archive_extract_destination("RAM:",1,"demo",output,5));
    {
        struct NDArchiveSource items[4];
        memset(items,0,sizeof(items));
        strcpy(items[0].path,"Work:Folder");items[0].drawer=1;items[0].include_info=1;
        strcpy(items[1].path,"Work:Folder/child.txt");
        strcpy(items[2].path,"work:folder.info");
        strcpy(items[3].path,"Work:Other.txt");
        assert(nd_archive_plan_sources(items,4)==2);
        assert(!strcmp(items[1].path,"Work:Other.txt"));
        strcpy(items[0].path,"Work:File.txt");items[0].drawer=0;items[0].include_info=0;
        strcpy(items[1].path,"work:file.txt");assert(nd_archive_plan_sources(items,2)==1);
        strcpy(items[1].path,"Other:File.txt");assert(!nd_archive_plan_sources(items,2));
        strcpy(items[1].path,"Other:File.txt.info");items[0].include_info=1;
        assert(!nd_archive_plan_sources(items,2));
        strcpy(items[1].path,"Work:-r");assert(!nd_archive_plan_sources(items,2));
        assert(!nd_archive_plan_sources(items,ND_ARCHIVE_MAX_SOURCES+1));
        assert(!nd_archive_output_name("",ND_ARCHIVE_ZIP,output,sizeof(output)));
    }
    {
        struct NDArchiveCloseInput input;
        const unsigned char *event;
        int closed;
        memset(&input,0,sizeof(input));
        assert(nd_archive_close_input(&input,'\r'));
        memset(&input,0,sizeof(input));
        event=(const unsigned char *)"\2332;0;104;0;10;20;123;456|";
        closed=0;while(*event)closed|=nd_archive_close_input(&input,*event++);
        assert(closed);
        memset(&input,0,sizeof(input));
        event=(const unsigned char *)"\033[2;0;232;0;10;20;123;456|\2332;0;255;0;0;0;0;0|";
        closed=0;while(*event)closed|=nd_archive_close_input(&input,*event++);
        assert(!closed); /* Button release and mouse movement are not clicks. */
        event=(const unsigned char *)"\23311;0;0;0;0;0;0;0|";
        while(*event)closed|=nd_archive_close_input(&input,*event++);
        assert(closed);
        memset(&input,0,sizeof(input));
        event=(const unsigned char *)"\2332;0;99999999999999999999999999;0;0;0;0;0|";
        closed=0;while(*event)closed|=nd_archive_close_input(&input,*event++);
        assert(!closed);
        assert(nd_archive_close_input(&input,'\n'));
    }
    return 0;
}
