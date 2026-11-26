# Plugin-owned artwork

NovaDock 0.4 supports artwork that travels with a plugin. Keep the executable,
manifest and its `.assets` drawer together:

```text
Plugins/
  Example
  Example.ndplugin
  Example.assets/
    Artwork.ndart
```

The asset drawer uses the executable's exact filename, followed by `.assets`.
It is independent of NovaDock's shared `Icons` library. Add `artwork` to
`NOVADOCK_FEATURES`. Preferences > Plugins > Import validates and installs the
bank with the program and manifest as one transaction. Removal deletes the
owned bank and its empty drawer. Other files in that drawer are left alone.
Keep authoring sources in your project; only `Artwork.ndart` is needed at runtime.

## Required Tiny and Normal artwork coverage

Every plugin that displays artwork **must provide a recognizable icon in both Tiny and Normal (large) dock modes for every state it can send**. This includes idle, active, busy/loading, disabled, disconnected/unknown and sad/error states, plus every animation frame. Do not replace an icon with an empty tile or a text-only placeholder in Tiny mode. Preserve meaningful readings and state badges without covering the artwork.

A single 32×32 `Artwork.ndart` source per state satisfies both sizes when the host can scale it clearly; duplicate small/large files are not required. The included plugins use this approach. Keep important features readable at the smallest supported tile, use transparent backgrounds, and verify Colourful and theme-coloured modes on RTG and classic planar screens. Every name/state combination passed to `nd_client_update_art_quiet()` or `nd_client_update_art_meter_quiet()` must exist in the shipped bank. Package the matching executable, manifest and complete `.assets` drawer together.

Before release, run `make check` or `python3 tools/check_plugin.py .`. The included `tools/check_plugin_art.py` checks declared banks, binary bounds and every packaged state at Normal, Tiny, minimum Tiny and compressed 55×46 Normal sizes. It rejects missing banks and states whose pixels disappear when scaled. It cannot prove that an icon is recognizable or infer states selected dynamically by C code: compare your emitted name/state combinations with the bank and visually test every state in FS-UAE at both sizes, idle and magnified, on Bottom, Top, Left and Right docks. Include sad/error/offline states explicitly; successful connected screenshots alone do not establish coverage.

## Select a named frame and state

Link the current `nd_client.c` and include `nd_client.h` as usual:

```c
event = nd_client_update_art_quiet(&client,
    "LAMP",                      /* text fallback for older docks */
    is_on ? "On" : "Off",        /* tooltip/status text */
    "lamp",                      /* artwork name in the bank */
    is_on ? 1 : 0,                /* state number defined by your plugin */
    elapsed_ticks, 500U);
```

Names contain 1–23 ASCII letters, digits, `_` or `-`. State numbers are 0–255;
you decide their meaning. HASS uses 0 off/closed, 1 on/open and 2 unknown.
Use a separate frame for errors or pending operations. The bundled WHDLoad,
MUSIC and SENSOR plugins each ship their sad status frame in their own bank. For hardware controls,
select a frame from the confirmed reading, not the requested action.

The helper negotiates support through an ABI 5 reply capability and immediately republishes the first artwork state when support becomes known, without waiting for the next heartbeat. Older docks
receive the ordinary text caption. Quiet updates retain the usual heartbeat
and click/settings handling; there is no extra polling process or message.
The reserved `ART:name:state` caption is internal to the helper. Badge support
continues to work independently.

NovaDock validates and caches banks while loading plugin resources, shares one
copy between instances of the same executable, and maps the palette to the
Workbench screen. Painting a changed state does not open files. Reload or
restart NovaDock after updating a bank. Missing, damaged or unsupported banks
fall back to the normal tile rather than reading untrusted offsets.

## Build an artwork bank

The standalone SDK includes `tools/pack_plugin_art.py` and
`examples/artwork.json`. Run on a host with Python 3.9 or later:

```sh
python3 tools/pack_plugin_art.py examples/artwork.json ArtHello.assets/Artwork.ndart
```

The JSON document has a `palette` array of sixteen RGB triples and a `frames`
array. Each frame contains `name`, `state` and `pixels`: 32 rows of 32 palette
indexes. A row may be an array of integers or a string of hexadecimal digits
`0`–`f`. Index zero is transparent. Authoring does not require a graphics library
or Python imaging dependencies. Use the included two-state ArtHello example
as a starting point.

The basic runtime format is NDAR version 1: a six-byte magic/version prefix,
big-endian 16-bit frame count, 48-byte RGB palette, then 32-byte frame records.
Each record has a 24-byte NUL-terminated name, state byte, reserved zero byte,
big-endian 16-bit rectangle count and 32-bit data offset. Rectangle records
are five bytes: x, y, width, height and colour index. The packer builds these
runs from the source pixels. Banks are limited to 256 KiB and 256 frames;
each frame is 32×32 with up to 1024 nontransparent runs.

Do not put credentials, local configuration or user data in an asset bank.
ArtHello only changes a local example state; it never controls a real device.

## Theme colours and value layouts

The packer also accepts `palette_roles`, an array of sixteen numbers. Zero
uses the RGB colour; 1 through 5 use the dock theme's background, subtle,
text, accent and second accent colours. Entry zero remains transparent and
must have role zero. Theme roles follow theme changes without reloading files.

A frame's optional `flags` is a bitmask: 1 draws its value below the icon,
2 permits a larger icon, and 4 formats an HH:MM:SS duration compactly. Add
flags together when needed. The Weather and Timer banks demonstrate value
layouts. Animation is a sequence of ordinary named states selected by the
plugin; no additional host animation timer is created.

Roles and presentation flags use NDAR version 2: the same 56-byte prefix,
followed by sixteen role bytes, then the frame table. The frame record's byte
25 carries flags. Version 1 remains supported. A separate negotiated ABI 5
capability carries the artwork name key and state in reserved packet fields,
so captions, tooltips and status text remain readable. Hosts without this
capability receive the legacy text fallback or reserved ART caption.

Preferences marks a required missing, invalid or newer-format bank as
**Needs repair** and excludes that package from Add Item. Reinstall/update the
complete plugin package, or update NovaDock for a newer artwork format.

## Dynamic thumbnails (NovaDock 0.5)

For changing local content, `nd_client_thumbnail` sends a bounded 32×32 indexed
frame with sixteen RGB colours. `nd_client.h` includes its portable
`nd_thumbnail.h` definition. The helper negotiates
`ND_EVENT2_HOST_THUMBNAIL` before sending; older hosts receive no new command.
A zero-size frame clears the thumbnail. No plugin-supplied paths or decoder
calls execute in the dock. The host validates indices, caches screen bitmaps
and uses existing Workbench pens. Keep normal status/heartbeat updates running
and resend a frame after settings changes or screen reloads. Package static
artwork as the fallback for hosts without this optional capability.

### Independent live headings

Artwork flag `ND_ARTWORK_HEADING` (8), combined with `ND_ARTWORK_VALUE` (1),
reserves a separate content row for the update's caption above its value. The
configured tile label remains a footer. This is useful for rotating sensor
channels: use `nd_client_update_art_quiet` with a readable channel caption and a
sensor-type artwork key. Keep names out of diagnostic events. Heading banks
require a host accepting flag 8; older hosts reject the bank and retain their
ordinary text fallback. The SDK packer validates all four presentation flags.

Use `nd_client_update_art_meter_quiet()` when an artwork state accompanies a live meter or graph reading. Its arguments add `meter` between `state` and `elapsed_ticks`; it preserves the same quiet heartbeat and older-host text fallback as `nd_client_update_art_quiet()`. State banks are scaled for Tiny and Normal sizes, including busy, offline and sad/error frames.
