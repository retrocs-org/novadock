<?php
require_once __DIR__ . '/includes/layout.php';
site_header('Features', 'Features', 'A short guide to NovaDock launchers, themes, plugins and settings.');
?>
<!-- content -->
<p class="eyebrow">Made for your daily Workbench</p>
<h1>Useful. Flexible.<br>Easy to make yours.</h1>
<p class="lead">Start with your favourite apps. Add a few tools. Arrange everything the way you like.</p>
<p class="category-links"><a href="#features">Features</a> &middot; <a href="#screenshots">Screenshots</a></p>
<div class="feature-list" id="features">
<div><h2>Launch anything</h2><p>Add apps, drawers, commands and groups. Drop icons onto the dock to get started.</p></div>
<div><h2>Put it where it works</h2><p>Use any screen edge, up to three horizontal rows, or auto-hide to keep your desktop clear.</p></div>
<div><h2>Find things faster</h2><p>Browse drawers, jump to an open app by title and use hot keys for your favourite actions.</p></div>
<div><h2>Choose your style</h2><p>Twelve themes, four tile shapes and two sizes. Preview changes in Preferences before applying them.</p></div>
<div><h2>Add useful tools</h2><p>Clock, system meters, clipboard history, music and more. Built-in plugins are included with NovaDock.</p><a href="marketplace.php">Browse the marketplace &rarr;</a></div>
<div><h2>Keep settings safe</h2><p>Use changes for this session or save them permanently. Recover a previous valid settings file from Diagnostics.</p></div>
<div><h2>Stay light</h2><p>A low-resource preset reduces background work. Hidden docks defer scans, and artwork loads in small batches.</p></div>
<div><h2>Go a little further</h2><p>Use ARexx commands, script-driven tiles and the plugin SDK to build your own workflow.</p><a href="developers.php">Developer guide &rarr;</a></div>
</div>
<p class="note">Requires AmigaOS 3.0 or later and a 68000 or better. Some plugins need additional software or hardware. Graphs show samples rather than a fixed time scale.</p>

<section id="screenshots" class="section-heading"><p class="eyebrow">See it on Workbench</p><h2>Take a closer look.</h2><p>Ten views of the dock, Preferences and bundled tools. Select a preview for the full image.</p>
<table class="gallery" width="100%" border="0" cellpadding="10" cellspacing="4">
<tr><td width="50%" valign="top" bgcolor="#313244"><b>Clock calendar</b><br><br><a href="screenshots/calendar.gif"><img src="screenshots/calendar-thumb.gif" width="236" height="193" border="0" alt="NovaDock Clock calendar"></a><p>Open the month calendar from Clock, with the current date highlighted and previous/next month controls.</p><small><a href="screenshots/calendar.gif">Full image (GIF)</a></small></td><td width="50%" valign="top" bgcolor="#313244"><b>Clipboard History</b><br><br><a href="screenshots/clipboard-manager.gif"><img src="screenshots/clipboard-manager-thumb.gif" width="268" height="145" border="0" alt="NovaDock Clipboard History"></a><p>Manage history from Clippy, with a search field and Copy, Delete, Clear, Pause and Pin controls. This view shows an empty history.</p><small><a href="screenshots/clipboard-manager.gif">Full image (GIF)</a></small></td></tr>
<tr><td width="50%" valign="top" bgcolor="#313244"><b>Default Nord dock</b><br><br><a href="screenshots/default-dock.gif"><img src="screenshots/default-dock-thumb.gif" width="268" height="37" border="0" alt="NovaDock 0.1: Default Nord dock"></a><p>Six default tiles: System, Shell, Music, CPU, RAM and Clock.</p><small><a href="screenshots/default-dock.gif">Full crop (10 KB GIF)</a></small></td><td width="50%" valign="top" bgcolor="#313244"><b>General Preferences</b><br><br><a href="screenshots/preferences.gif"><img src="screenshots/preferences-thumb.gif" width="268" height="91" border="0" alt="NovaDock 0.1: General Preferences"></a><p>Click actions, auto-hide, refresh mode, hot key and Use/Save.</p><small><a href="screenshots/preferences.gif">Full crop (17 KB GIF)</a></small></td></tr>
<tr><td width="50%" valign="top" bgcolor="#313244"><b>Appearance</b><br><br><a href="screenshots/appearance.gif"><img src="screenshots/appearance-thumb.gif" width="268" height="116" border="0" alt="NovaDock 0.1: Appearance"></a><p>Theme, shape, text and tile-size controls with the actual preview.</p><small><a href="screenshots/appearance.gif">Full crop (24 KB GIF)</a></small></td><td width="50%" valign="top" bgcolor="#313244"><b>Diagnostics</b><br><br><a href="screenshots/diagnostics.gif"><img src="screenshots/diagnostics-thumb.gif" width="268" height="90" border="0" alt="NovaDock 0.1: Diagnostics"></a><p>Live counters, save state, recovery and low-resource controls.</p><small><a href="screenshots/diagnostics.gif">Full crop (24 KB GIF)</a></small></td></tr>
<tr><td width="50%" valign="top" bgcolor="#313244"><b>Low-resource preset</b><br><br><a href="screenshots/low-resource.gif"><img src="screenshots/low-resource-thumb.gif" width="268" height="59" border="0" alt="NovaDock 0.1: Low-resource preset"></a><p>The confirmation explains the changes before staging them.</p><small><a href="screenshots/low-resource.gif">Full crop (10 KB GIF)</a></small></td><td width="50%" valign="top" bgcolor="#313244"><b>Validated recovery</b><br><br><a href="screenshots/recovery.gif"><img src="screenshots/recovery-thumb.gif" width="268" height="46" border="0" alt="NovaDock 0.1: Validated recovery"></a><p>A known-good copy is validated before replacing the Preferences session.</p><small><a href="screenshots/recovery.gif">Full crop (11 KB GIF)</a></small></td></tr>
<tr><td width="50%" valign="top" bgcolor="#313244"><b>Organize the dock</b><br><br><a href="screenshots/organize.gif"><img src="screenshots/organize-thumb.gif" width="268" height="95" border="0" alt="NovaDock 0.1: Organize the dock"></a><p>Order configured tiles and add optional items without editing text files.</p><small><a href="screenshots/organize.gif">Full crop (20 KB GIF)</a></small></td><td width="50%" valign="top" bgcolor="#313244"><b>Clippy picker</b><br><br><a href="screenshots/clippy-picker.gif"><img src="screenshots/clippy-picker-thumb.gif" width="268" height="122" border="0" alt="NovaDock 0.1 Clippy empty clipboard picker"></a><p>An empty clipboard is a normal state, with a clear message and Escape to close.</p><small><a href="screenshots/clippy-picker.gif">Full crop (GIF)</a></small></td></tr>
</table>
</section>
<!-- /content -->
<?php site_footer(); ?>
