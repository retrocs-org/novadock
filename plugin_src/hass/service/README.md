# AmiHASS client interface

Vendored public headers from AmiHASS 0.1, local ABI 1.
The SDK client implementation is ../nd_hass_client.c; only its header include
is adapted to NovaDock. Per-tile UI and dock IPC remain in the HASS plugin.
AmiHASSService and credential storage are separate, in the AmiHASS project.
Refresh these headers and the client together when changing the ABI.

`SDK/amihass.h`, `SDK/amihass_core.h` and `SDK/amihass_client.c` from the bridge
are tracked by the SHA-256 snapshot in sdk.json. The dock regression checks that
those exact bytes are used (apart from the client header include). ABI 1 also
provides read-only diagnostic pages; no credential data enters the dock.

The current snapshot adds room-scoped LIST/DEVICES, SUBSCRIBE/UNSUBSCRIBE,
per-selection refresh preferences and credential-free source/connection hints.
The dock renews only selected-entity leases and releases them through SDK close.
These additive operations retain ABI 1. An older service may explicitly reject
SUBSCRIBE; only then does the adapter fall back to legacy QUERY. Room filtering
requires the matching service build. The dock has no credential or HTTP transport.

The final 0.4 client also checks STATUS feature.areas,
feature.catalogue_filters and feature.subscriptions metadata. Discovery fails
closed for unadvertised support. AMIHASS_AREAS supplies the paged room picker.
