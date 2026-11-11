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
    return 0;
}
