/* Adapted from Beam68. Copyright (c) 2026 Beam68 contributors.
 * MIT licence: see Aminet-parser-LICENSE.txt in the package. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bsdsocket.h>
#include <sys/filio.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "nd_aminet.h"
#include "nd_aminet_http.h"
struct Library *SocketBase;
static char header[8192],buffer[4096],request[640];
static unsigned long ticks(void)
{
    struct DateStamp d;DateStamp(&d);
    return (unsigned long)d.ds_Days*4320000UL+(unsigned long)d.ds_Minute*3000UL+(unsigned long)d.ds_Tick;
}
static int ready(int fd,int writing,unsigned long start,unsigned long timeout,
                  int (*progress)(unsigned long,unsigned long),unsigned long done,unsigned long total)
{
    fd_set set;struct timeval tv;LONG rc;
    for(;;) {
        if((unsigned long)(ticks()-start)>=timeout || (SetSignal(0,0)&SIGBREAKF_CTRL_C) || (progress && !progress(done,total))) return 0;
        FD_ZERO(&set);FD_SET(fd,&set);tv.tv_secs=0;tv.tv_micro=100000;
        rc=WaitSelect(fd+1,writing?NULL:&set,writing?&set:NULL,NULL,&tv,NULL);
        if(rc>0) return 1;
        if(rc<0 && Errno()!=EINTR) return 0;
    }
}
static void set_error(char *out,unsigned int cap,const char *s)
{ if(cap) { strncpy(out,s,cap-1);out[cap-1]=0; } }
int nd_aminet_fetch(const char *path,const char *destination,unsigned long limit,
                      unsigned long seconds,int require_length,int (*progress)(unsigned long,unsigned long),
                      char *error,unsigned int error_cap)
{
    struct hostent *host;struct sockaddr_in address;BPTR file=0;
    int fd=-1,ok=0,header_done=0;LONG nonblock=1,rc,socket_error=0;
    socklen_t optlen=sizeof(socket_error);
    unsigned int used=0,sent=0,request_len;unsigned long done=0,start=ticks();long length=-1;
    set_error(error,error_cap,"Aminet transfer failed, timed out or was cancelled.");
    if(!path || path[0]!='/' || strlen(path)>400 || strchr(path,'\r') || strchr(path,'\n')) return 0;
    SocketBase=OpenLibrary("bsdsocket.library",4);
    if(!SocketBase) { set_error(error,error_cap,"Start your TCP/IP stack (bsdsocket.library is required).");return 0; }
    /* Resolver latency is controlled by the installed TCP/IP stack. */
    host=gethostbyname((STRPTR)"m68k.aminet.net");
    if(!host || host->h_addrtype!=AF_INET || host->h_length!=4 || !host->h_addr_list[0]) {
        set_error(error,error_cap,"Cannot resolve m68k.aminet.net.");goto cleanup;
    }
    if(progress && !progress(0,0)) goto cleanup;
    fd=(int)socket(AF_INET,SOCK_STREAM,0);
    if(fd<0 || IoctlSocket(fd,FIONBIO,(char *)&nonblock)<0) goto cleanup;
    memset(&address,0,sizeof(address));address.sin_family=AF_INET;address.sin_port=htons(80);
    memcpy(&address.sin_addr,host->h_addr_list[0],4);
    rc=connect(fd,(struct sockaddr *)&address,sizeof(address));
    if(rc<0 && Errno()!=EINPROGRESS && Errno()!=EWOULDBLOCK) goto cleanup;
    if(!ready(fd,1,start,seconds*50UL,progress,done,length>=0?(unsigned long)length:0UL) ||
       getsockopt(fd,SOL_SOCKET,SO_ERROR,&socket_error,&optlen)<0 || socket_error) goto cleanup;
    sprintf(request,"GET %s HTTP/1.0\r\nHost: m68k.aminet.net\r\nUser-Agent: NovaDock-Aminet/0.3\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n",path);
    request_len=(unsigned int)strlen(request);
    while(sent<request_len) {
        if(!ready(fd,1,start,seconds*50UL,progress,done,length>=0?(unsigned long)length:0UL)) goto cleanup;
        rc=send(fd,request+sent,request_len-sent,0);
        if(rc<0 && (Errno()==EWOULDBLOCK || Errno()==EINTR)) continue;
        if(rc<=0) goto cleanup;
        sent+=(unsigned int)rc;
    }
    file=Open((STRPTR)destination,MODE_NEWFILE);
    if(!file) { set_error(error,error_cap,"Cannot create the download file.");goto cleanup; }
    for(;;) {
        unsigned int i=0;
        if(!ready(fd,0,start,seconds*50UL,progress,done,length>=0?(unsigned long)length:0UL)) goto cleanup;
        rc=recv(fd,buffer,sizeof(buffer),0);
        if(rc<0 && (Errno()==EWOULDBLOCK || Errno()==EINTR)) continue;
        if(rc<0) goto cleanup;
        if(!rc) break;
        if(!header_done) {
            while(i<(unsigned int)rc) {
                if(used+1>=sizeof(header)) { set_error(error,error_cap,"Aminet response header is too large.");goto cleanup; }
                if(!buffer[i]) { set_error(error,error_cap,"Invalid NUL in Aminet HTTP header.");goto cleanup; }
                header[used++]=buffer[i++];header[used]=0;
                if(used>=4 && !memcmp(header+used-4,"\r\n\r\n",4)) {
                    if(!nd_aminet_http_header(header,limit,&length)) {
                        set_error(error,error_cap,"Aminet returned an unsupported HTTP response (or the package is missing).");goto cleanup;
                    }
                    if(require_length && length<0) {
                        set_error(error,error_cap,"Aminet did not provide an archive size; download was not committed.");goto cleanup;
                    }
                    header_done=1;break;
                }
            }
        }
        if(header_done && i<(unsigned int)rc) {
            unsigned long n=(unsigned long)rc-i;
            if(n>limit-done || (length>=0 && n>(unsigned long)length-done)) {
                set_error(error,error_cap,"Aminet response exceeds the transfer size limit.");goto cleanup;
            }
            if(Write(file,buffer+i,n)!=(LONG)n) { set_error(error,error_cap,"Download write failed (check free space).");goto cleanup; }
            done+=n;
            if(progress && !progress(done,length>=0?(unsigned long)length:0UL))goto cleanup;
        }
        if(header_done && length>=0 && done==(unsigned long)length) break;
    }
    if(!header_done || !done || (length>=0 && done!=(unsigned long)length)) {
        set_error(error,error_cap,"Aminet transfer was empty or incomplete.");goto cleanup;
    }
    if(!Flush(file)) { set_error(error,error_cap,"Cannot flush the download file.");goto cleanup; }
    ok=1;
cleanup:
    if(file) { if(!Close(file)) ok=0; }
    if(fd>=0) CloseSocket(fd);
    CloseLibrary(SocketBase);SocketBase=NULL;
    if(!ok && file) DeleteFile((STRPTR)destination);
    return ok;
}
