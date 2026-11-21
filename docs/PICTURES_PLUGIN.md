# PICTURES plugin

Add PICTURES from Add Item → Plugin and select its Picture drawer in tile
settings. The slideshow interval is 10–3600 seconds (default 30). Each instance
has its own drawer and slideshow. Images and their original files are read-only.

The plugin indexes up to the first 32 supported filenames in sorted order,
scanning at most 4096 immediate drawer entries. It recognises IFF/ILBM, PNG,
JPEG, GIF and BMP filenames; decoding requires a matching installed picture
DataType. Files must be at most 4 MiB with dimensions at most 1024×1024. Header
checks happen before invoking the decoder. Unsupported/corrupt files report an
error; manual Next or the next slideshow interval can move past them.

Image indexing, decoding and 16-colour preview conversion run in a separate
worker. One 128×96 preview is retained; the dock receives a fixed 32×32 thumbnail
and caches its display bitmap. The renderer uses existing Workbench pens and
does not change the shared palette. Image fidelity depends on screen colours;
the larger viewer displays the bounded preview rather than the full-resolution
source. Unsupported true-colour DataTypes require compatible CyberGraphX support.

Click for a larger preview and Previous, Next, Play/Pause and Rescan controls.
Left/right arrows navigate in the preview, Space pauses/resumes and Escape
closes it. Rescan picks up directory changes. The slideshow avoids decoding
while the dock is hidden/covered and only repaints changed images or exposed
windows. Older docks without thumbnail support retain the plugin's static art.
