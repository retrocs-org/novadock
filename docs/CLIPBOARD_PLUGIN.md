# Clippy clipboard-history plugin

Right-click the dock background, choose **Add → Plugins → Clippy**.
It is optional and does not change the six default tiles or an existing layout.
The tile uses a pre-generated clipboard icon and a bottom label, matching Apps.
An empty history reports EMPTY instead of treating absent clipboard data as an
error.

Click the tile to open a **centered, themed quick picker** like Apps. Each card
previews a stored text clip. Click a card or use the arrow keys and Return to
copy it, then paste with Amiga-V in the target application. Escape or a click
outside closes it. A larger history is paged; use the footer's left/right halves
or the arrow keys at a page boundary. The picker takes a stable memory snapshot
so new clipboard activity cannot change the text underneath your selection.

Right-click the tile and choose **Manage history...** for the detailed list.
Select a row and use **Copy**, or use its management controls:

- **Delete** removes the selected history entry.
- **Clear** removes the stored history, including a capture already in progress.
- **Pause / Resume** controls capture. Pausing retains existing entries.
- **Close** or Escape closes the popup.

Tile settings set the history limit (1–32, default 10) and its optional numbered
count badge. Identical clips are deduplicated and moved to the front. History
is kept only in the plugin's memory and is cleared when it exits. There is no
history file, network access or automatic clipboard logging to disk. Standard
AmigaOS clipboard storage is independent of this plugin.

This version captures FTXT text from clipboard unit 0. It accepts multiple CHRS
chunks and skips other chunk types. Binary/image clips are ignored. Entries
larger than 8192 bytes, raw forms larger than 64 KiB, malformed forms and embedded
NULs are skipped whole, never silently truncated. Text retains its original
Amiga character bytes and line breaks.

A clipboard owner can post data lazily. Transfers are asynchronous and the
plugin continues answering NovaDock while waiting. **CLIPBOARD BUSY** means a
transfer has not completed; another Copy is refused until it finishes.
After the dock closes, an outstanding transfer finishes quietly before its
process exits; this releases the system clipboard correctly even if the
clipboard owner supplies its data late.
The list is intentionally bounded and reads at most once per second when idle.

Deleting or clearing history does not clear the system clipboard. Pause capture
before copying text you do not want retained in the history.

### Search and pinned snippets

The management window has a case-insensitive search field. Enter text and press
Return to filter the history; Copy, Delete and Pin act on the displayed
selection. Pinned entries have a `*` marker and survive automatic eviction by
new clipboard text. The existing total history limit still applies. If every
entry is pinned, new text is not added until an entry is unpinned or deleted.
Explicit Clear/Delete and reducing the configured limit can remove pinned
entries. Pins and clipboard contents remain session-local.
