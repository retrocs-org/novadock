# Notification service transition

ALERTS, AMIRC and AMIROCKET are no longer shipped or offered by NovaDock.
The notification-focused integrations are moving to the independent AmiNotis
service. AMINOTIS, the AmiNotis inbox tile, brings the bundled SDK-client
inventory to 47 plugins. See [AMINOTIS](AMINOTIS_PLUGIN.md).

WATCH and LOGS remain file/log monitors. AGENDA and TIMER remain reminders.
DOWNLOAD, AMINET, ARCHIVE and HASS retain their own functions and the existing
notification API, ready for the future service integration.

Older saved tile lines are retained as RETIRED_PLUGIN.<ID> migration metadata,
with OPTION.<ID> settings preserved. Long tile definitions use numbered
RETIRED_PLUGIN.<ID>.<chunk> fields. They are not launched, shown as missing
plugins, or offered in Add/Plugins. Saving a profile keeps these settings.
The installers remove only the retired managed executables, manifests, launch
icons, artwork directories and shipped AmIRC hook from NovaDock's shared data.
They do not remove the standalone AmIRC or AmiRocket applications.

Reference sources remain in plugin_src/alerts, plugin_src/amirc and
plugin_src/amirocket. Their old manifests/artwork are in each directory's
legacy subdirectory. The legacy AmIRC hook is
plugin_src/amirc/legacy/NovaDockNotify.amirx. These references are not runnable
components of the new NovaDock package.

AmiNotis and its SDK are developed separately. AMINOTIS reads the shared
AmiNotis inbox; it does not start the service. Existing internal session notice
collection remains available to code using the current API; no new external
service is started by this build. General Preferences offers **Send alerts
to AmiNotis**, off by default. It forwards only new notices while the service
is available; local session collection continues independently. The saved
choice survives service absence. Delivery uses asynchronous version-2 SDK
messages with a bounded queue and timeout; it never waits for the service in
the dock event loop. Old history is not replayed when forwarding is enabled.

Support diagnostics and reports include session delivery/drop counts, queued and pending notices, unavailable/full-queue/timeout/rejection counts and the last forwarding failure/result. Diagnostics Reset clears these counters without cancelling an in-flight delivery. Disabled forwarding creates no attempts; disabling it cancels and counts already queued notices.

Current SDK clients negotiate 80-character titles and 512-character bodies. Older hosts retain their 31-character transport previews. NovaDock keeps the legacy history-control preview layout, but compares the complete bounded text when coalescing and forwards the full text to AmiNotis.
