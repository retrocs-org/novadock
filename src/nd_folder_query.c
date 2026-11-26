/* Bounded directory results live inside the task packet, never in a shared
 * heap allocation. The receiver either retains that packet or discards it. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include "nd_async_io.h"
int nd_query_folder(struct NDFolderBatch *batch)
{
 struct FileInfoBlock *fib;BPTR lock;LONG error=0;
 batch->count=batch->more=0;batch->error=0;lock=Lock(batch->path,ACCESS_READ);
 if(!lock){batch->error=IoErr();if(!batch->error)batch->error=ERROR_OBJECT_NOT_FOUND;return 0;}
 fib=AllocDosObject(DOS_FIB,NULL);
 if(!fib){UnLock(lock);batch->error=ERROR_NO_FREE_STORE;return 0;}
 if(!Examine(lock,fib)){error=IoErr();if(!error)error=ERROR_OBJECT_NOT_FOUND;}
 else if(fib->fib_DirEntryType<=0)error=ERROR_OBJECT_WRONG_TYPE;
 else {
  while(ExNext(lock,fib)){
   struct NDFolderEntry *entry;if(!nd_folder_visible_name(fib->fib_FileName))continue;
   if(batch->count==ND_FOLDER_MAX){batch->more=1;break;}
   entry=&batch->entries[batch->count++];memset(entry,0,sizeof(*entry));strncpy(entry->name,fib->fib_FileName,sizeof(entry->name)-1);
   entry->drawer=fib->fib_DirEntryType>0;entry->size=fib->fib_Size;entry->days=fib->fib_Date.ds_Days;entry->minutes=fib->fib_Date.ds_Minute;entry->ticks=fib->fib_Date.ds_Tick;
  }
  if(!batch->more){LONG last=IoErr();if(last && last!=ERROR_NO_MORE_ENTRIES)error=last;}
 }
 FreeDosObject(DOS_FIB,fib);UnLock(lock);batch->error=error;return error ? 0:1;
}
