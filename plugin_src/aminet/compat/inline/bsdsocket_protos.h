#ifndef ND_AMINET_BSDSOCKET_INLINE_H
#define ND_AMINET_BSDSOCKET_INLINE_H
/* Classic bsdsocket.library register calls, matching the NDK ABI. */
LONG __nd_socket(__reg("a6") struct Library *,__reg("d0") LONG,__reg("d1") LONG,__reg("d2") LONG)="\tjsr\t-30(a6)";
#define socket(a,b,c) __nd_socket(SocketBase,(a),(b),(c))
LONG __nd_connect(__reg("a6") struct Library *,__reg("d0") LONG,__reg("a0") const struct sockaddr *,__reg("d1") LONG)="\tjsr\t-54(a6)";
#define connect(a,b,c) __nd_connect(SocketBase,(a),(b),(c))
LONG __nd_send(__reg("a6") struct Library *,__reg("d0") LONG,__reg("a0") const void *,__reg("d1") ULONG,__reg("d2") LONG)="\tjsr\t-66(a6)";
#define send(a,b,c,d) __nd_send(SocketBase,(a),(b),(c),(d))
LONG __nd_recv(__reg("a6") struct Library *,__reg("d0") LONG,__reg("a0") void *,__reg("d1") ULONG,__reg("d2") LONG)="\tjsr\t-78(a6)";
#define recv(a,b,c,d) __nd_recv(SocketBase,(a),(b),(c),(d))
LONG __nd_getsockopt(__reg("a6") struct Library *,__reg("d0") LONG,__reg("d1") LONG,__reg("d2") LONG,__reg("a0") void *,__reg("a1") socklen_t *)="\tjsr\t-96(a6)";
#define getsockopt(a,b,c,d,e) __nd_getsockopt(SocketBase,(a),(b),(c),(d),(e))
LONG __nd_ioctl(__reg("a6") struct Library *,__reg("d0") LONG,__reg("d1") ULONG,__reg("a0") void *)="\tjsr\t-114(a6)";
#define IoctlSocket(a,b,c) __nd_ioctl(SocketBase,(a),(b),(c))
LONG __nd_closesocket(__reg("a6") struct Library *,__reg("d0") LONG)="\tjsr\t-120(a6)";
#define CloseSocket(a) __nd_closesocket(SocketBase,(a))
LONG __nd_select(__reg("a6") struct Library *,__reg("d0") LONG,__reg("a0") fd_set *,__reg("a1") fd_set *,__reg("a2") fd_set *,__reg("a3") struct timeval *,__reg("d1") ULONG *)="\tjsr\t-126(a6)";
#define WaitSelect(a,b,c,d,e,f) __nd_select(SocketBase,(a),(b),(c),(d),(e),(f))
LONG __nd_errno(__reg("a6") struct Library *)="\tjsr\t-162(a6)";
#define Errno() __nd_errno(SocketBase)
struct hostent *__nd_gethostbyname(__reg("a6") struct Library *,__reg("a0") const char *)="\tjsr\t-210(a6)";
#define gethostbyname(a) __nd_gethostbyname(SocketBase,(a))
#endif
