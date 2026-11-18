# HASS / current AmiHASS SDK review — 18 November 2026

NovaDock remains 0.4; HASS and AmiHASS remain 0.1, with local message ABI 1.

## Coverage

| Current bridge feature | NovaDock support |
| --- | --- |
| Room + type + search | Combined server-side filters in both discovery steps; room name or ID, empty means all rooms. Unknown rooms return no matches. |
| Devices and scoped entities | Type retained across paging and device/entity navigation; explicit Entities route includes targets without physical devices. |
| Selected-entity subscriptions | SUBSCRIBE for configured tiles, bounded renewal, SDK unsubscribe on close/settings/target changes. Explicit unsupported response falls back to legacy QUERY. |
| Per-client refresh | Default or 5/10/30/60/300/900/3600 seconds; the shared bridge combines clients selecting the same entity. |
| Read-only clients | Per-tile option disables Click Action; state popup creates no action widgets. Adapter independently rejects controls. |
| Display preferences | State, name and state, or name; optional updated timestamp in details. |
| Source and connection state | Sample/server label, bridge failure hints, provider-change invalidation and existing connection notifications. |
| Capability metadata | Explicit popup summary plus capability-gated brightness/RGB, climate targets, fan speed and cover movement. |
| Confirmed actions | Sending/Waiting/Confirmed/failure; state/parameter readback, nine-second bounded confirmation, no duplicate sends or reconnect replay. |
| On-demand fresh read | Refresh requests one fresh read then ordinary follow-up snapshots. |
| Shared sample provider | Native sample tests, independent-client readback and zero HTTP/control traffic to real devices. |
| Client/memory diagnostics | Available in AmiHASS's own diagnostics and used for native lease validation; no duplicate diagnostics application added to the tile. |
| Dashboard pages/layouts/undo/backups | Remain in AmiHASS Dashboard; dock tiles retain their own settings and native dock organization. |
| Push/WebSocket transport | Not provided by the current SDK; subscriptions use shared polling rather than unsolicited events. |

The vendored public headers, core policy and C client were refreshed together,
with SHA-256 provenance in `plugin_src/hass/service/sdk.json`. No direct HTTP,
credential handling, ARexx calls, service reconfiguration or physical controls
were added to the dock.

## Checks

- Full sanitized NovaDock host suite passed, followed by full refreshed website/archive checks.
- Native build, local LHA extraction (48 programs / 22 artwork banks), source/SDK/website packaging and staging passed.
- Staged dock, Preferences, HASS binary/manifest/artwork, guide and installer match canonical files. Existing dock settings were retained.

- Portable adapter tests cover area changes during pending requests, scene/type retention, paging, invalid/unknown rooms, read-only enforcement, subscription intervals/renewal, explicit legacy fallback, one-shot fresh reads, delayed confirmation, timeout, source changes and connection hints.
- Independent duplicate tiles preserve room/refresh settings.
- Native picker rectangles and all plugin settings fit 640×256 and 640×480. Additional settings use the existing Preferences scrolling.
- Workbench 3.2.1 sample test passed: `/private/tmp/nd-hass-room-native-03/junit.xml`.
- Native results include combined room/type/search/device filtering, empty unknown-room results, scene-only rows, new room picker, light/climate/fan/cover/read-only windows, no actions from read-only tiles, and a lease count returning from one to zero after close.
- On/off/toggle and brightness 25/100/0 were confirmed through the real sample service. A second client changed brightness and the tile followed it. Three service stop/start cycles recovered without replaying offline actions. HTTP diagnostics remained zero.
- An early native test failure was a test-harness output-handle leak between synchronous child runs; it was corrected before the passing run.

Real device behavior and physical Amiga performance remain separate validation.
No commits, uploads or publishing were performed.
