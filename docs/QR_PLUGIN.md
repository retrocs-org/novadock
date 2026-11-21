# QR plugin

Add QR from Add Item → Plugin. Enter text or a URL, or use Paste to read the
Amiga clipboard, then Generate. Encoding runs locally in a separate worker;
text is never sent to a web service. The preview uses exact black and white,
whole pixels and a four-module quiet zone. If the current screen cannot fit a
readable code or allocate those colours, QR reports the problem.

Input is limited to 512 UTF-8 bytes. Valid UTF-8 is retained; legacy clipboard
bytes are interpreted as Latin-1 and converted to UTF-8. The encoded QR declares
UTF-8 and uses at least medium error correction. Shorter text is easier to scan.

Enter an Amiga filename and choose Export to save a one-plane black/white ILBM
with the quiet zone included, at two pixels per module. Export is enabled only
when the generated code matches the current input and no job is running. Existing files are not
overwritten. Preview and export use the same encoded matrix. Changing the input
while a job runs discards its outdated result. QR.assets contains the artwork;
the bundled Nayuki encoder retains its MIT licence and pinned source provenance.

Export reports write-protected, full or missing destinations and keeps the input,
export path and generated preview for retry. Temporary filenames fit classic
Amiga filesystems and are removed after a failed write. Existing exports remain intact.
