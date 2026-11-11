<?php
// Built-in catalog. Add reviewed community listings separately when available.
return [
    ['executable' => 'Clock', 'name' => 'Clock', 'group' => 'Everyday', 'description' => 'Time at a glance, with a month calendar a click away.', 'requires' => ''],
    ['executable' => 'Clipboard', 'name' => 'Clippy', 'group' => 'Everyday', 'description' => 'Keep, search and pin text clips during your session.', 'requires' => 'History is session-only.'],
    ['executable' => 'RunningApps', 'name' => 'Running Apps', 'group' => 'Everyday', 'description' => 'Find an open app by title and bring its window forward.', 'requires' => ''],
    ['executable' => 'Trash', 'name' => 'Trash', 'group' => 'Everyday', 'description' => 'Open the Trashcan or drop files onto it from the dock.', 'requires' => 'Uses each file\'s own volume Trashcan.'],
    ['executable' => 'Activity', 'name' => 'CPU Activity', 'group' => 'System', 'description' => 'Watch activity as a compact meter or history graph.', 'requires' => 'An activity estimate, not an exact CPU percentage.'],
    ['executable' => 'Memory', 'name' => 'RAM', 'group' => 'System', 'description' => 'See Chip and Fast memory in a meter or graph.', 'requires' => ''],
    ['executable' => 'Network', 'name' => 'Network', 'group' => 'System', 'description' => 'See traffic, inspect connection details and control Roadshow.', 'requires' => 'Requires compatible Roadshow/SANA-II support. Offline stops the stack.'],
    ['executable' => 'Mixer', 'name' => 'Mixer', 'group' => 'System', 'description' => 'Adjust AHI output volume or mute it from the dock.', 'requires' => 'Requires a usable AHI setup.'],
    ['executable' => 'Screenshot', 'name' => 'Screenshot', 'group' => 'System', 'description' => 'Capture the front screen or Workbench as IFF/ILBM.', 'requires' => 'Indexed screens with 1-8 bitplanes; no deep true-colour capture.'],
    ['executable' => 'ScreenMode', 'name' => 'Screen Mode', 'group' => 'System', 'description' => 'Choose a saved screen preset and open the system tool.', 'requires' => 'Requires configured ScreenMode presets.'],
    ['executable' => 'MusicPlayer', 'name' => 'MusicPlayer', 'group' => 'Media & apps', 'description' => 'Bring your player forward and control playback.', 'requires' => 'Supports EaglePlayer, AmigaAMP and HippoPlayer via ARexx.'],
    ['executable' => 'DOpus4', 'name' => 'DOpus 4', 'group' => 'Media & apps', 'description' => 'Start Directory Opus 4 or bring it back to the front.', 'requires' => 'Directory Opus 4 must be installed.'],
    ['executable' => 'Games', 'name' => 'WHDLoad Games', 'group' => 'Media & apps', 'description' => 'Browse your games drawer and launch a game from its icon.', 'requires' => 'WHDLoad and games must be installed.'],
    ['executable' => 'BitSkim', 'name' => 'BitSkim', 'website' => 'https://bitskim.retrocs.org', 'group' => 'Media & apps', 'description' => 'See transfers, incoming offers and peer status.', 'requires' => 'Requires BitSkim; transfer approval remains in BitSkim.'],
    ['executable' => 'AmIRC', 'name' => 'AmIRC', 'group' => 'Media & apps', 'description' => 'Open AmIRC and see unread message alerts.', 'requires' => 'Requires AmIRC; unread counts need the supplied event hook.'],
    ['executable' => 'ArchiveExtractor', 'name' => 'Archive Extractor', 'group' => 'Tools & fun', 'description' => 'Drop an archive onto the tile to unpack it nearby.', 'requires' => 'Requires matching archive commands in C:.'],
    ['executable' => 'Command', 'name' => 'Command', 'group' => 'Tools & fun', 'description' => 'Run a command regularly and display its output.', 'requires' => ''],
    ['executable' => 'Script', 'name' => 'Script', 'group' => 'Tools & fun', 'description' => 'Build a tile with your own ARexx captions, values and actions.', 'requires' => 'Advanced configuration; see source documentation.'],
    ['executable' => 'Eyes', 'name' => 'Eyes', 'group' => 'Tools & fun', 'description' => 'A pair of eyes follows your Workbench pointer.', 'requires' => ''],
];
