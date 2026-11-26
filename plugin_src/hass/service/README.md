# AmiHASS client interface

Vendored public headers, REST/control policy and client from AmiHASS 0.3,
local ABI 1. `sdk.json` records the exact upstream SHA-256 values, including
the UTF-8 display helper. Only the client/core include paths are adapted to
NovaDock. AmiHASS remains a separate service and owns credentials and HTTP.

The tested pair is NovaDock 0.6 / AmiHASS 0.3. Feature-capable AmiHASS 0.2
services remain supported: required areas, combined catalogue filters and
subscriptions are negotiated by STATUS rather than the version string.
Optional 0.3 operations (batch reads, labels, cameras, history, attributes,
notifications and additional device controls) are not automatically exposed
in NovaDock's current tile UI merely because their SDK definitions exist.

The 0.3 client tolerates boot-time clock adjustments. Shared control parsing
also includes current climate step validation and metadata extensions.
Selection-capacity replies back off and show a specific reason. Authentication
rejection is shown as a credentials problem. Door/cover actions refused by the
service explain its Script doors setting; NovaDock never changes that policy
or identifies itself as the AmiHASS Dashboard to bypass it. Local confirmation
and read-only settings continue to apply. Code-protected devices remain outside
NovaDock's current control UI.

Run `python3 tools/check_amihass_pair.py --bridge ../AmiHASS` before packaging.
The pairing check verifies headers, client, policy parser, display helper and
reply ownership against the independent service repository.
