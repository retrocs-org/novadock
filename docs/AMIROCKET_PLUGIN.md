# AmiRocket

Add **AmiRocket** using Preferences → Organize → Add → Plugin. This optional tile starts or restores your AmiRocket client and shows its connection state and unread count across configured servers. It is included with NovaDock 0.4 and has its own plugin version, initially 0.1.

## Settings

- **AmiRocket executable**: select the installed client. The default is `SYS:Utilities/AmiRocket/AmiRocket`.
- **Client port**: `AUTO` finds `AMIROCKET` or a numbered client such as `AMIROCKET.2`. Set the exact port to monitor a particular client. AUTO chooses the first name in lexical order and prefers the unnumbered client.
- **Show unread count**: enables the badge, capped at `999+`. Opening the client does not clear the badge: only AmiRocket's confirmed subscription counts do that.

The rocket changes between disconnected, connected, unread and waiting states. The tooltip shows how many servers are online. A stopped client shows **NOT RUNNING**; click to launch it. A disconnected running client can still be restored by clicking. Authentication and connection controls remain in AmiRocket.

## Requirements and behaviour

Requires AmiRocket's **dock protocol 2** (`DOCKSTATUS` and `SUBSCRIBE ... 2`) and `rexxsyslib.library`. Native ARexx packets are sent directly; no RexxMast script is launched. AmiRocket itself requires AmigaOS 3.2, a 68020 or better, a TCP/IP stack and AmiSSL for HTTPS. This plugin adds no networking or TLS dependency to NovaDock.

The tile receives local change notifications, coalesces them into fresh snapshots and also polls every ten seconds. Counts cover all configured servers in the selected client. AmiRocket can retain cached counts for an offline server while other servers remain online; these are subscription counts, not a complete message history or mention counter.

A stale or stopped client clears the visible badge. Restarting the client triggers rediscovery and subscription. Only one command may be outstanding at a time; unanswered packets are never freed while AmiRocket might still reply. Up to four retired requests are retained after host replacement, bounding failure recovery storage. After repeated nonreplying crashes exhaust that bound, restart the dock. A modal client may temporarily show **NOT RESPONDING** and recover when it replies.

No chat text is sent, executed or stored by this plugin. It never reads account tokens. Artwork ships in `Plugins/AmiRocket.assets/Artwork.ndart`, beside its executable and manifest, and is copied by install and update.
