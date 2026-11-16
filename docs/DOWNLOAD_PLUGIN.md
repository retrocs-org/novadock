# DOWNLOAD plugin

Add **DOWNLOAD** from Add item > Plugins, then click the tile. Enter an HTTP or
HTTPS URL and a full **Save as** filename, such as `Work:Downloads/manual.pdf`.
Choose **Queue URL** to download. Up to eight files can be queued; one runs at
a time. The tile shows received bytes and a queue badge. Percentage progress
appears when curl reports it. Closing the window keeps the queue running.
**Clear waiting** removes waiting entries and leaves the active transfer alone.

Edit settings selects an HTTPS-capable curl executable (default `C:curl`) and
a default save drawer (default `RAM:`). curl, its required libraries and a
working TCP/IP stack must be installed separately. URLs with embedded
credentials are not accepted. Certificate verification stays enabled.

Each file is downloaded into a uniquely named temporary drawer on the
chosen destination volume, then moved to the requested filename after success.
Existing files are never intentionally overwritten; choose another name.
A failed transfer removes its partial file. Redirects are restricted to HTTP(S).
Connection setup is limited to 30 seconds and each transfer to ten minutes.

The queue belongs to the current plugin session. Quitting or restarting
NovaDock discards waiting entries; an outstanding helper may finish its
transfer and clean up its temporary file, without publishing a completed file.
Use a dedicated download manager for persistent queues or resumable transfers.
