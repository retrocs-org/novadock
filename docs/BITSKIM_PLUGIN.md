# BitSkim plugin

An optional NovaDock tile for the separately installed Amiga BitSkim client.
The updated BitSkim 0.1 source exposes ARexx protocol 2; version discovery uses
CAPABILITIES, not the application's unchanged 0.1 VERSION string.

## Add and use

Right-click the dock background, choose Add item, then Plugins and BitSkim.
Set **BitSkim executable** if it is not at `SYS:Utilities/BitSkim/BitSkim`.
The existing default dock layout remains unchanged. Restart NovaDock once
after updating to load its enhanced context menu and file-drop handling.

Click to start BitSkim or restore its window, and acknowledge the latest result
alert. Idle tiles use BitSkim's icon. Transfers show SEND/RECV, percentage and
recent payload speed (K/s uses 1024 bytes). A zero rate means no measured payload
activity, not proof of a failed transfer. Pending incoming offers show
INCOMING FILE and a `!` badge; outgoing offers show WAIT ACCEPT.

## Send a file

Choose **Send file...** from the tile's context menu, or drop one regular file
onto the tile. The helper opens BitSkim, then a centered recipient list showing
computer name, platform and address. Select a computer and press **Send**.
Refresh can collect devices that appear after discovery resumes. No file is
sent until Send is pressed. Cancel closes the picker without sending.

BitSkim must be running for Send and Details. Click its tile first if it is
offline. SHOW resumes discovery because iconified BitSkim clears its peer list
and pauses searches. The helper uses stable peer IDs and SENDID; it never falls
back to another peer, an old list index or a manual address. A disappeared or
ambiguous recipient is rejected by BitSkim. Incoming acceptance remains in
BitSkim's normal requester; the dock never accepts automatically.

One regular file, at most 2,147,483,647 bytes, is supported. Drawers, multi-file
drops and batch queues are rejected. Filenames must also satisfy BitSkim's
portable filename rules. Paths with spaces and literal quotes are encoded for
BitSkim's command parser. A busy transfer rejects a second send.

## Results and details

Protocol-2 clients retain the last terminal result. The tile shows SENT,
RECEIVED, TRANSFER FAILED, CANCELLED or DECLINED until acknowledged by clicking
it, or until a new transfer becomes active. A new successful result gets a `1`
badge; failure, cancellation and decline get `!`. These represent the latest
unacknowledged result, not a multi-transfer unread queue. Incoming offers take
priority while active. Badge OFF suppresses badges.

Open BitSkim to inspect filenames, computer names and error details, or to
change its application preferences. These controls stay in BitSkim itself.

The plugin negotiates capabilities and polls numeric STATUS plus session-aware
EVENTDETAILS. It rereads LAST when events advance or history is lost and
deduplicates results by qualified transfer ID. On application restart or event
generation change it resynchronizes; an already-retained result at initial
attachment is not announced as new. Rapid events can exceed the 32-entry
history: LAST is a single retained result, so intermediate outcomes cannot be
reconstructed and are not invented.

## Right-click controls

- Open BitSkim: start or restore the application.
- Send file: choose one file and a recipient.
- Toggle receiving: enable or disable receiving in the running application.
- Cancel transfer: ask BitSkim to cancel the current transfer.
- Edit settings: configure this dock plugin.
- Remove: remove the tile.

Other controls require BitSkim to be running. Errors from rejected actions use
BitSkim's ERROR diagnostic when available. That diagnostic slot is host-global,
so the return code and transfer ID/result remain authoritative.

## Settings and compatibility

`PROGRAM` is the executable. `PORT=AUTO` discovers BITSKIM or BITSKIM.n;
an explicit port selects one copy. `POLL_TICKS=50` is the initial refresh
interval (50 ticks per second), with NovaDock's normal idle backoff.
`BADGE=PEERS` shows nearby-device counts while idle; `STATUS` shows receiving
status instead; `OFF` hides badges. A running, receiving client with no peers
shows a dot. Offline and unresponsive clients show no dot.

Older clients returning RC 10 to CAPABILITIES retain the original numeric
status, progress and launch/restore/management controls. The new send
helper explains that protocol 2 is required; no legacy SEND fallback is used.

On the tested rexxsyslib 47.2 setup, CreateRexxMsg omitted its REXX marker,
causing BitSkim to return messages without handling them. The plugin now
supplies that marker only when missing and retains it until the reply (or
with an abandoned request). This fixes STATUS FAILED without replacing
BitSkim or the system library.

Each requester keeps at most one asynchronous ARexx message in flight. After
five seconds without a reply, the tile shows WAIT BITSKIM and keeps answering
dock IPC. It resumes automatically when the pending reply returns; a valid
status also clears a previous STATUS FAILED notice immediately. A separate helper process owns pickers/requesters, so it does not block
the dock. A timed-out live message is not freed beneath BitSkim. Native BitSkim
accept/save/error requesters may delay ARexx replies until dismissed: a timeout
does not mean a transfer failed. The plugin opens no network sockets and does
not hold bsdsocket.library open during Roadshow shutdown.

## Artwork

`Plugins/BitSkim.info` is from the supplied BitSkim source, copyright 2026
BitSkim contributors, under MIT. See `BitSkim-icon-LICENSE.txt` for its notice.

## Tile layout

Active transfers use the same centered heading row as CPU/RAM, with SEND or
RECV and a percentage above the meter. Hover over the tile for transfer speed
and its full current status. Idle tiles keep the BitSkim icon and bottom title.

BitSkim does not support graph mode. Clicking (including Shift-click) opens
or restores BitSkim; old saved GRAPH settings load as progress-meter mode.

If a pending request loses its registered destination (or that port changes
owner), the resident can reconnect using a fresh reply port. It never resends
an already-issued action. Old messages and reply targets remain valid for late
replies, which are discarded and reclaimed. At most four lost-host requests
are retained; further recovery waits for one to return or for a manual plugin
restart. Merely waiting in an accept/save dialog does not trigger reconnection.
