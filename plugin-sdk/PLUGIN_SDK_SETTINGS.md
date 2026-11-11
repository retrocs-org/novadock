# Creating your plugin's settings window

NovaDock already supplies the settings application. Declare your plugin's
fields in its manifest, and NovaDockPrefs builds the window opened by
**right-clicking your dock tile > Edit settings...**. No separate preferences
executable or custom GadTools form is needed for ordinary options.

## 1. Declare the fields

Keep your executable and manifest together, with matching names. The Hello
example uses `Hello` and `Hello.ndplugin`. Retain the complete required
metadata from `examples/Hello.ndplugin`, then add option lines:

```ini
NOVADOCK_OPTION=GREETING|Greeting|TEXT||HELLO AMIGA
NOVADOCK_OPTION=POLL_TICKS|Refresh ticks|UINT|10,500|50
```

The format is:

```text
NOVADOCK_OPTION=KEY|Label|TYPE|Choices or range|Default
```

Use unique keys within the plugin. The manifest ID, such as `HELLO`, identifies
its configuration namespace. NovaDock passes the configured ID as `argv[1]`;
use that value rather than hard-coding it in your reader. A manifest supports
up to 12 option definitions.

| Type | Example | Settings control |
| --- | --- | --- |
| TEXT | `TITLE\|Title\|TEXT\|\|My tile` | Editable text |
| UINT | `POLL_TICKS\|Refresh ticks\|UINT\|10,500\|50` | Validated integer within the range |
| BOOL | `SHOW_LABEL\|Show label\|BOOL\|\|YES` | Yes/No cycle |
| CHOICE | `STYLE\|Style\|CHOICE\|COMPACT,FULL\|COMPACT` | Cycle of the listed choices |
| FILE | `PROGRAM\|Program\|FILE\|\|SYS:Utilities/MultiView` | Editable path and a Select file button |

The table omits the `NOVADOCK_OPTION=` prefix for readability. Defaults must
match their declared types. Do not put a pipe character inside a field.

## 2. Import and open the form

1. Build the SDK Hello example with `make`.
2. In NovaDock Preferences, open **Plugins > Import...** and select its manifest.
3. Review the import preview and choose **Add to dock...**.
4. Right-click the new tile and choose **Edit settings...**.
5. Change Greeting or Refresh ticks, then choose **Use** or **Save**.

The dock launches the shared editor with arguments of this form:

```text
NovaDockPrefs PAGE=PLUGIN PLUGIN=HELLO FOCUSED=1
```

`PLUGIN=` must be the actual configured ID. The focused editor shows that
plugin's declared fields; a plugin with no fields shows an explanatory message.
The window sizes itself for the Workbench screen and scrolls when necessary.

After changing a manifest's schema, remove its dock tiles and re-import the
updated package so Preferences receives the new definitions, then add the tile
again. Editing only the manifest on disk does not update an already imported
configuration's schema.

## 3. Read the values

The SDK's `examples/hello.c` already implements the complete flow. Include
`nd_option_runtime.h` and batch related values into one configuration read:

```c
static void read_settings(const char *id, char *greeting,
                          unsigned int size, unsigned int *poll)
{
    struct NDOptionString fields[2];
    char ticks[8];

    fields[0].key = "GREETING";
    fields[0].fallback = "HELLO AMIGA";
    fields[0].out = greeting;
    fields[0].limit = size;
    fields[1].key = "POLL_TICKS";
    fields[1].fallback = "50";
    fields[1].out = ticks;
    fields[1].limit = sizeof(ticks);
    nd_plugin_option_strings(id, fields, 2);
    *poll = (unsigned int)atoi(ticks);
    if (*poll < 10 || *poll > 500) *poll = 50;
}
```

This excerpt needs `<stdlib.h>` and `nd_option_runtime.h`. Call it once after
initializing the client, and again after an update reports settings changes:

```c
if (event & ND_EVENT_QUIT) break;
if (event & ND_EVENT_SETTINGS)
    read_settings(argv[1], greeting, sizeof(greeting), &poll);
```

Use `nd_client_wait(&client, nd_client_delay_ticks(&client, poll))` in the
update loop so the dock can wake the plugin for changes. Link `src/nd_client.c`
and `src/nd_option_runtime.c`, as the SDK Makefile does. The helper handles
configuration-source selection; do not assume that the active file always
lives beside your executable. Validate values in your plugin too, since users
can edit configuration files manually.

## 4. Understand Use, Save and notification

- **Use** applies settings for the current session.
- **Save** applies them and saves the permanent copy.
- Closing the editor discards changes that have not been applied.
- The shared editor writes `OPTION.<ID>.<KEY>` records to NovaDock's single
  configuration. Plugins should read those records, not rewrite the whole file.
- `ND_EVENT_SETTINGS` means applied settings changed. It is not a request for
  your plugin to open a preferences window. The dock may also restart a plugin
  for some changes, so always read settings at startup as well.

Check that changing Greeting updates the tile without restarting the dock,
that an invalid refresh value is rejected or safely replaced by a default,
and that Save survives quitting and restarting NovaDock.

## Custom preferences applications

The current manifest has no hook to replace **Edit settings...** with an
external preferences executable. Do not invent a `NOVADOCK_PREFS` field or use
`ND_EVENT_SETTINGS` as an open-window event.

If your plugin needs a specialised UI, it can own a native Intuition/GadTools
window or launch its own helper through a plugin action, such as handling
`ND_EVENT_CLICK`. Keep processing dock updates and honour `ND_EVENT_QUIT` while
that UI is open. Avoid a long blocking window loop in the update task; use a
separate worker or integrate the window's signals with the plugin event loop.
Such a UI does not automatically replace the dock's right-click settings form.
