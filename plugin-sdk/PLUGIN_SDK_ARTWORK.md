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

The helper negotiates support through an ABI 5 reply capability. Older docks
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
