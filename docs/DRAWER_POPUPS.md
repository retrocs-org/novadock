# NovaDock drawer popups

Right-click a drawer or volume tile and choose **Popup contents** to browse
the drawer without opening a Workbench window.

## Layout

```text
+-----------------------------------------+
| <  Projects               Name  =       |   header
|  [icon]   [icon]   [icon]   [icon]      |
|  Apps     Docs     Games    readme      |   icon grid (or list)
+-----------------------------------------+
```

- **Icon grid** shows each object's own classic Workbench icon, scaled down to
  fit when it is large. Colour 0 is transparent so icons sit on the theme
  colour. Objects without an icon get a drawer or file symbol. GlowIcons and
  NewIcons show their classic fallback image, because NovaDock never remaps
  the Workbench palette.
- **List** shows a symbol, the name and, for files, the size.
- Drawers come first, then files sorted by **Name**, **Date** (newest first)
  or **Size** (largest first).
- Up to 64 objects are listed; a `+` after the title means the drawer holds
  more (open it in Workbench to see everything). A bar on the right edge
  shows the scroll position.

## Using it

| Action | Effect |
|---|---|
| Click a drawer | Shows its contents in the popup |
| Click a file | Opens it through Workbench and closes the popup |
| Shift- or Alt-click a drawer | Opens it in a Workbench window |
| `<` in the header, Backspace | Up one level (never above the tile's drawer) |
| Click the title | Opens the shown drawer in Workbench |
| Click the sort name | Cycles Name, Date, Size |
| Click `=` / `#` | Switches between list and icon grid |
| Typing | Jumps to the first name starting with the typed letters; the same letter again steps to the next match; a pause of a second starts over |
| Cursor keys | Move the selection (in the list, Left goes up and Right enters a drawer) |
| Return / Shift-Return | Open the selection / open a drawer in Workbench |
| Mouse wheel | Scrolls (NewMouse wheel events) |
| Esc, right button, clicking elsewhere | Close the popup |

## Defaults

The view and sort order you pick in a popup are remembered, written to
`ENV:` straight away and to `ENVARC:` a few seconds later. They are stored as:

```text
FOLDER_VIEW=ICONS    (or LIST)
FOLDER_SORT=NAME     (or DATE, SIZE)
```

Reading a drawer and its icons never raises "Please insert volume"
requesters. On floppy-based systems the list view is faster, because the icon
grid reads every object's `.info` file.

## Native testing checklist

1. Open a drawer popup on `SYS:`; check icons, scaling and the `+` marker in a
   large drawer such as `SYS:C`.
2. Enter a sub-drawer, go up with `<` and Backspace, and check the popup
   never goes above the tile's drawer.
3. Type the first letters of a name; press the same letter repeatedly.
4. Switch view and sort, close, reopen, and restart NovaDock: the choice is
   kept.
5. Open the popup on a floppy tile and remove the disk: no requester appears.
