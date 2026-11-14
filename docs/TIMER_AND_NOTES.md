# Timer and Notes

Available in NovaDock 0.2. Add either plugin from **Add plugin** or enable it in
Preferences. Right-click **Edit settings...** to save its preferences in the
normal NovaDock configuration.

## Timer

Click Timer to open its controls. Enter a duration in seconds (1 to 86400), then
choose **Start**. **Pause** keeps the remaining time; Start resumes it. **Reset**
uses the entered duration and stops the countdown. Close the controls to leave
a running timer counting down in the dock. Changing the duration during a run
requires Reset. The tile shows remaining hours, minutes and seconds and a
progress meter.

At zero, a reminder window opens or comes forward. The tile says **TIME IS UP**
until Reset or a new countdown. The optional screen flash happens once.
The reminder stays responsive and the dock continues running.

Settings: **Default seconds**, **Reminder message**, **Flash screen on completion**.
Changed defaults apply to the next fresh countdown; they do not interrupt a run.
The duration and message preferences are saved; an active countdown is
session-only. Timing uses the hardware EClock, so changing the wall clock or
pausing dock display updates does not change the elapsed duration.

## Notes

Right-click Notes and choose **Add new note...**. Enter a title and write in the
multiline editor. **Save** keeps it open and reports the saved-note count;
**Save/close**, the close gadget and Escape save before closing. **Discard**
closes without saving. A completely empty new draft is not saved.

Click the dock icon, or choose **Browse notes**, to open a picker in the same
style as Running Apps. Each entry shows its title and a readable preview.
Click an entry, or use the arrow keys and Return, to open it. An empty collection
starts a new draft. The dock badge counts saved notes. Selecting another note saves the current note before switching. If saving
fails, the current editor stays open. Selecting the already-open note brings
its window forward without changing it.

### Editing and formatting

The white page wraps and scrolls. Drag across text, or use Shift and the arrow
keys, to select it. Choose **Normal**, **H1**, **H2**, **H3** or **H4** for paragraph
styles. **B** and **I** toggle bold and italic. The font chooser offers Topaz 8,
Topaz 9 and Courier 13; the colour chooser offers black, red, blue, green,
purple and grey. Inline font, style and colour changes apply to the selection.
Without a selection they set the style for the next text you type. Courier
falls back to Topaz when its font is unavailable.

Right-click the page for **Cut**, **Copy**, **Paste**, **Delete selected text** and
**Remove formatting**. Amiga+A selects all; Amiga+C/X/V copies, cuts and pastes;
Amiga+S saves; Amiga+B/I changes style. Control shortcuts work too. Clipboard
transfers are plain text. A failed clipboard operation preserves the note.
Text uses the system's Amiga character encoding.

### Markdown and permanent files

**View** switches between **Formatted** and **Markdown**. Both edit the same
saved text; switching views preserves the cursor/selection. Headings, bold,
italic, escapes and inline code use Markdown. Fonts and colours are saved separately in note metadata, so the Markdown
source contains no generated HTML. The formatted editor handles this subset;
other Markdown constructs remain visible as source text. Formatting a selection
normalizes the supported markup while preserving its visible text.

Notes live in `ENVARC:NovaDock/Notes` and survive dock and system restarts.
New notes are `.md` files with YAML front matter containing the title and
note metadata. Older `.ndnote` files remain readable. The **Notes drawer**
setting selects another permanent drawer; its parent must exist. A folder
change applies after the current editor closes. Back up the whole drawer.
Up to 32 notes and 8 KB of Markdown per note (1024 logical lines) are supported.
An edit or formatting change that cannot fit is refused without losing text.
Custom font/colour runs have a separate 2 KB metadata limit. External changes
to a file's Markdown invalidate stale font/colour metadata; its text is retained.

**Delete** moves the open saved note to the collection's **Deleted** drawer and
updates the count. To recover one, move its file back to the notes drawer under
an unused `note-NNNN.md` filename. Deleting an unsaved draft just closes it.

Saving checks for external changes and stages complete files before replacing
them. Failure leaves the editor open with your text intact; a conflicting file
is not overwritten. On shutdown, unsaved work is saved when safe. Conflicts
attempt a separate `recovered-...ndnote` file; failed replacement can retain a
`.save-...` drawer with `new` and `old` recovery files. Invalid files are kept
and reported as read errors.

Both plugins sleep for up to ten seconds while idle with their windows closed,
and wake immediately for dock actions. Timer refreshes once a second while
running. Editing windows remain responsive without blocking the dock.
