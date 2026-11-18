# AmiRocket plugin review

NovaDock 0.4; independent AmiRocket plugin 0.1.

## Verified

The native plugin was tested under AmigaOS Workbench 3.2.1 with the 68060 configuration of the local Workbench test app. The test uses fake local AmiRocket and NovaDock message ports, with the production plugin executable. No Rocket.Chat account, token, login or chat message is used.

The test verifies:

- Protocol 2 subscription, a confirmed unread count of 42 and connected artwork.
- A notification reconciles the unread count to zero without a read-marking command.
- A tile click sends SHOW, never SEND.
- An unanswered status query clears stale badges and reports the stalled client while the dock remains responsive.
- A late reply restores confirmed state.
- Removing and replacing the client port triggers rediscovery and subscription.
- The plugin exits and sends its goodbye packet after the dock requests shutdown.

Portable tests cover malformed, missing, duplicated and overflowing fields, invalid client names/settings, badge limits, modal waits, changed host owners, late replies and bounded retired-request storage. The production dock click-routing test covers AmiRocket with latched Shift and a stale graph flag.

## Limits

This verifies the local protocol and plugin lifecycle. It does not certify every AmiRocket version or a real Rocket.Chat server connection, and the test app measurements do not represent physical Amiga hardware. A visual check with the installed AmiRocket client remains useful: add the tile, select its executable, connect a client, receive an unread message, open it, and restart AmiRocket. Account setup remains exclusively in AmiRocket.

`tests/native_amirocket.c` refuses to run without the isolated-test marker. Its separate report records each native assertion. It must run in a disposable Workbench test profile; it stops only that guest's dock before creating its mock port.
