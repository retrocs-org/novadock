/* Fault-injection helper for native_support_export.c. Build this as
 * QATests:FailLhA and QATests:RaceLhA; never included in the installer. */
#include <stdio.h>
#include <string.h>
long __stack=16384;
int main(int argc,char **argv){FILE*f;if(argc<4)return 20;f=fopen(argv[3],"wb");if(!f)return 20;fwrite("xx-lh5-",1,7,f);fclose(f);if(strstr(argv[0],"Race")){f=fopen("QATests:race.lha","w");if(f){fputs("other writer",f);fclose(f);}return 0;}return 20;}
