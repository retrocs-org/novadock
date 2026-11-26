# Plugin diagnostics policy

Use the supplied `nd_client` for every bundled or third-party client. The host
owns capture policy, retention, disk output and support-report collection.
Do not create a private always-on trace file or log a title, path, search query,
message body, setting value, contact detail, credential or arbitrary text.

`nd_client.c` includes the private resource implementation. Follow the sample
Makefile and do not also link `nd_plugin_resources.c` as a second object.

The normal SDK Makefile enables `ND_PLUGIN_RESOURCES`. Include
`nd_plugin_resources.h` **after** native and C library headers in each source
file that allocates memory or performs file I/O. It preserves native inline
library stubs and wraps AllocVec/AllocMem, malloc/calloc/realloc and their frees,
and Open/Read/Write/ExNext. Use the matching allocation/free families. Foreign
library-owned pointers pass through safely. Library buffers, code, stacks and
graphics allocations are not process-total RAM; UI and reports call this
**tracked RAM**. Tracking has 128 buckets and at most 4096 records per process;
allocation/metadata failure marks results partial without failing a successful
application allocation. Metadata is released when the last NDClient closes.
Call `nd_resources_forget(pointer)` when ownership passes to another process.
Internal workers share the owner's accounting only while that owner is alive;
external launched programs are not attributed to the plugin.

`nd_client` negotiates `ND_EVENT2_HOST_SUPPORT` before sending `ND_MSG_SUPPORT`.
Old hosts never see that command. Off sends no support-only packets. Logging
sends numeric snapshots no more often than five seconds; Detailed trace allows
one per second. Normal status heartbeats may make a snapshot arrive later.
A temporary resource sample requests snapshots without enabling logging. Support
metadata never consumes queued click/media/settings events. Existing v1-v5
message prefixes and artwork/badge negotiation remain intact.

Describe meaningful work with fixed operations and numeric fields:

```c
nd_client_activity(&client, ND_PLUGIN_OP_SCAN, 1, 0, 0);
/* scan without blocking the host */
nd_client_activity(&client, ND_PLUGIN_OP_SCAN, errors ? 3 : 2,
                   entries, errors);
```

Phases are 1 begin, 2 complete and 3 failed. Use the supplied operation constants;
there is no arbitrary-string logging field. Automatic wrappers record numeric
allocation/I/O counters and the latest operation. Add domain counters such as
index entries, cache hits, queue lengths or transferred bytes through the two
numeric detail fields. Do not encode private data in those fields.

Create report automatically collects repeated five-second statistical CPU
samples. Preferences > Support > Advanced > Activity also offers a manual
sample. It compares exact plugin task pointers during jittered timer interrupts;
there are no Exec patches, CIA ownership changes or busy-wait sampler tasks.
CPU values are estimates with a sample count, not scheduler accounting. Short
bursts and internal/external child tasks may be missed. RAM and last-operation
values are bounded snapshots and carry age/partial/availability information.
Old clients remain usable and show unavailable resource data.

Inside an SDK project, run `python3 tools/check_plugin.py .` or `make check`.
In the full NovaDock tree, run `python3 tools/check_plugin_policy.py`, the host regression suite,
and the native plugin-support fixture. Third-party authors must use these APIs
and follow the policy; the host cannot prevent independently written programs
from opening their own files outside the SDK.
