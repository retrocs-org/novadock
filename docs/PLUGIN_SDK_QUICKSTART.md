# NovaDock Plugin SDK quick start

The standalone SDK contains everything needed to build a basic NovaDock
status plugin without building NovaDock itself:

```text
NovaDock-PluginSDK/
  README.md
  Makefile
  include/nd_protocol.h
  include/nd_client.h
  src/nd_client.c
  include/nd_option_runtime.h
  src/nd_option_runtime.c
  examples/hello.c
  examples/Hello.ndplugin
```

Plugins are ordinary AmigaOS command-line programs. NovaDock starts a plugin
with its configured ID in `argv[1]`; the plugin sends small status messages to
the public `NovaDock.1` Exec message port. NovaDock owns all tile drawing.

## Requirements

- Classic AmigaOS 3.0 or newer.
- vbcc configured with an AmigaOS m68k target and compatible NDK headers.
- A running NovaDock installation for testing.

The API targets the baseline 68000 and uses C89. No MUI, Reaction, shared
library, socket connection or NovaDock source build is required.

## Build the Hello example

Unpack the SDK, open its directory in an Amiga Shell, and run:

```text
make
```

If `make` is unavailable, enter this as one Shell line:

```text
vc -O -Iinclude -o Hello examples/hello.c src/nd_client.c src/nd_option_runtime.c
```

Copy the program and its manifest into NovaDock's plugin drawer:

```text
Copy Hello NovaDock:Plugins/Hello
Copy examples/Hello.ndplugin NovaDock:Plugins/Hello.ndplugin
```

Open the dock's **Add item > Plugins** window. Hello appears in the installed-plugin list marked
`[New]`; select it, choose **Add item**, then **Save**. Its **Settings** page
already shows the Greeting and Refresh fields described by the manifest. No
hand-editing of `novadock.cfg` is needed. See `docs/PLUGIN_MANIFEST.md`.

## The complete simple plugin

```c
#include <proto/dos.h>
#include "nd_client.h"

int main(int argc, char **argv)
{
    struct NDClient client;
    unsigned int event;
    int clicked = 0;

    if (argc < 2 || !nd_client_init(&client, argv[1])) return 20;
    for (;;) {
        event = nd_client_update(&client, "HELLO",
                                 clicked ? "CLICKED!" : "HELLO AMIGA",
                                 ND_NO_METER);
        if (event & ND_EVENT_QUIT) break;
        if (event & ND_EVENT_CLICK) clicked = !clicked;
        Delay(nd_client_delay_ticks(&client,50));
    }
    nd_client_close(&client);
    return 0;
}
```

The helper keeps one message in flight, waits for NovaDock's reply and safely
handles the public port disappearing. `caption` and `value` are limited to 31
visible bytes plus the terminating NUL. A meter value is `0` through `100`, or
`ND_NO_METER` to hide it.

## Turn the example into your plugin

1. Copy `examples/hello.c` and rename the file and Makefile target.
2. Keep `nd_client.c`, `nd_client.h` and `nd_protocol.h` unchanged.
3. Replace the caption/value with your own short status text.
4. Call `nd_client_update()` no more often than needed; once per second is a
   good default for status plugins.
5. Handle `ND_EVENT_CLICK` if the tile should react to a normal click.
6. Always handle `ND_EVENT_QUIT` and call `nd_client_close()` before exiting.
7. Give every configured plugin instance a unique 1-19 character ID.

For progress meters, pass a percentage instead of `ND_NO_METER`. For one or
two normalized graph lines, use `nd_client_update_graph()`. Media integrations
can use `nd_client_update_media()` for Previous, Play/Pause, Next and volume.
If graph sampling must continue but identical values should not wake the dock,
use `nd_client_update_graph_quiet()` with a heartbeat shorter than 20 seconds.

## Settings window from the dock menu

Right-click a plugin tile and choose **Edit settings...** to open the shared
NovaDockPrefs editor for that plugin. You supply the fields in its manifest:

```ini
NOVADOCK_OPTION=GREETING|Greeting|TEXT||HELLO AMIGA
NOVADOCK_OPTION=POLL_TICKS|Refresh ticks|UINT|10,500|50
```

The SDK Hello example reads these values at startup and on
`ND_EVENT_SETTINGS`. Use applies them for the session; Save also saves the
permanent copy. You do not need to build a separate preferences executable.

Read [Creating your plugin's settings window](PLUGIN_SDK_SETTINGS.md) for
all field types, a configuration-reader example, import steps, testing and the
current limits on custom settings applications. This guide is also bundled
with the standalone SDK.

## API levels

- ABI v1: caption, value, meter, click, settings and quit events.
- ABI v2: v1 plus two normalized graph samples.
- ABI v3: v1 plus media capabilities, state, volume and control replies.

The v1 structure prefix remains unchanged. Choose the lowest API that provides
the feature you need. See `docs/PLUGIN_API.md` in the complete source package
for structure layout, lifecycle, validation, watchdog and compatibility rules.

## Common mistakes

- Do not cache the host port pointer; the supplied helper performs lookup and
  enqueue together under `Forbid()`/`Permit()`.
- Do not mutate or free an Exec message while it is in flight.
- Do not send unterminated or oversized strings.
- Do not busy-loop. Sleep between updates so the plugin remains inexpensive.
- Do not perform dock drawing from a plugin. Send data and let NovaDock draw.
- Do not ignore quit replies; disabled or removed plugins must exit promptly.



The SDK also contains `nd_option_runtime.c`/`.h`, so a plugin can read its
own `OPTION.<ID>.<key>` settings with `nd_plugin_option_string()` or
`nd_plugin_options_effective()`, and the example uses `nd_client_wait()` so it
reacts to clicks immediately.

## Package and import a plugin

Distribute the compiled executable with a manifest named after it,
`<NOVADOCK_PROGRAM>.ndplugin`, and optionally its icon:

```text
Hello-0.1/
  README.txt
  Plugins/
    Hello
    Hello.info          (optional)
    Hello.ndplugin
```

**Import...** in Preferences > Plugins accepts a package only when its
manifest is complete: `NOVADOCK_ID`, `NOVADOCK_VERSION`, `NOVADOCK_AUTHOR`,
`NOVADOCK_DESCRIPTION`, `NOVADOCK_LICENSE`, `NOVADOCK_FEATURES` and
`NOVADOCK_PROGRAM` must all be present, the program name must match the
manifest's name, and the ID must not belong to a bundled plugin or to a
different installed plugin. Add `NOVADOCK_MULTIPLE=YES` only when separate
instances are safe. The README should state the plugin name, author, license,
compatible NovaDock version, ABI level and dependencies.

Import copies the program, its icon and the manifest into NovaDock's Plugins
drawer as one change: if anything fails, nothing is left half-installed and a
previous version stays in place. A newer version replaces an installed one
once its tiles are removed from the dock. **Remove...** uninstalls a
third-party plugin that has no tiles, deleting its manifest first so a
partly removed plugin is never offered again. A plugin is a native
executable, so install packages only from a source you trust.
