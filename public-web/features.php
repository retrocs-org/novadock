<?php
require_once __DIR__ . '/includes/layout.php';
site_header('Features', 'Features', 'A short guide to NovaDock launchers, themes, plugins and settings.');
?>
<!-- content -->
<p class="eyebrow">Made for your daily Workbench</p>
<h1>Useful by nature.<br>Yours by design.</h1>
<p class="lead">Shortcuts where you want them. Tools when you need them.</p>
<p class="category-links"><a href="#features">Features</a> &middot; <a href="#next">NovaDock 0.4</a> &middot; <a href="#screenshots">Screenshots</a> &middot; <a href="marketplace.php#plugins">All plugins</a></p>
<div class="feature-list" id="features">
<div><h2>Drag it into place</h2><p>Drop apps and drawers onto the dock. An insertion line shows where they will land.</p></div>
<div><h2>Keep it out of the way</h2><p>Choose a screen edge, use up to three horizontal rows, or turn on auto-hide.</p></div>
<div><h2>Keep a thought</h2><p>Save titled Notes with headings, selected-text formatting and a clean Markdown view.</p></div>
<div><h2>Know what's coming</h2><p>Weather in the dock, a five-day forecast on click, and a Timer for your next task.</p></div>
<div><h2>Pack or unpack</h2><p>Make one ZIP or LHA from multiple files and drawers. Choose where to extract archives.</p></div>
<div><h2>Stay in control</h2><p>Music controls, public-screen switching and direct screen-mode selection, all close by.</p></div>
<div><h2>Choose your look</h2><p>Thirteen themes, four tile shapes and two sizes. Preview your changes before saving.</p></div>
<div><h2>Make room for more</h2><p>System meters, Trash, Clippy, screenshots and more. Explore every bundled tool in the <a href="marketplace.php">marketplace</a>.</p></div>
</div>
<section class="section-heading"><p class="eyebrow">In development for NovaDock 0.5</p><h2>Small tools for your Workbench.</h2><p>BENCH compares CPU, memory-copy and off-screen fill rates with percentage changes. QR creates codes locally with ILBM export. COLOUR samples Workbench pixels and saves named palettes. MOON estimates lunar phases and local sunrise/sunset offline. AGENDA saves tasks and recurring reminders with a due badge. PICTURES adds local picture-frame slideshows with larger previews. HASS adds read-only room summaries and capability-gated media playback, track information and volume through AmiHASS. Supported locks and motorised doors use explicit actions and opening confirmations. Tool windows keep their state during updates, COLOUR previews sampled colours and Preferences reuses validated plugin inventory. These additions are not included in the current 0.4 download.</p></section>
<p class="note">AmigaOS 3.0 or later, with a 68000 or better. Some plugins need additional software or hardware; each marketplace listing explains what is needed.</p>
<section id="next" class="section-heading"><p class="eyebrow"><?= site_version_is_upcoming('0.4') ? 'In development for' : 'New in' ?> NovaDock 0.4</p><h2>A more useful dock.</h2><p>More small tools, clearer status and a bridge to your Home Assistant devices. <?php if (site_version_is_upcoming('0.4')): ?>These changes are in the development build and are not included in the 0.3 download.<?php endif; ?></p>
<div class="feature-list">
<div><h3>Find the right light</h3><p>HASS offers a paged room chooser with combined room, type and search filters through the separate AmiHASS service. Add a light, scene, sensor or other supported entity; icons follow its reported state.</p></div>
<div><h3>Control what is supported</h3><p>Light brightness and RGB, thermostat targets, fan speed and cover movement appear when the device advertises them. Read-only tiles keep readings without controls.</p></div>
<div><h3>Keep an eye on changes</h3><p>WATCH filters drawer changes and batches notifications. LOGS follows text files with pause, severity filters, copy and export. ALERTS collects session notifications.</p></div>
<div><h3>Keep tools close</h3><p>CALC evaluates expressions with decimal, hex and binary output. TOOLTYPES edits icon metadata with original-file backups. SHELF keeps shortcut collections.</p></div>
<div><h3>Stay informed</h3><p>AmiRocket shows unread chat counts and connection status. Optional update checks have startup and 2/4/6-hour settings, Check now and last-check status.</p></div>
<div><h3>Fit your Workbench</h3><p>Compact settings and pickers fit 640&times;256 and 640&times;480 screens. Plugins carry their own artwork. Drawer and volume tiles show capacity and low-space warnings.</p></div>
</div>
<p>SAVER caches Boing frames and reduces raster drawing work, with optional FPS measurements. SENSOR adds an optional i2csensors.library v2 provider. Supported Warp1260 controls and Prism Wi-Fi setup are implemented; physical-card validation is still pending.</p>
</section>
<section id="home-assistant" class="section-heading"><p class="eyebrow">HASS &middot; NovaDock 0.4</p><h2>Your rooms, on Workbench.</h2><p>Install <a href="https://github.com/retrocs-org/amihass">AmiHASS</a> separately and save the server and token in AmiHASSPrefs. HASS tiles share that connection through AmiHASS; each tile keeps its own device and click action.</p>
<ol><li>Add HASS, choose a Type and use Rooms to select a room, or leave Room blank for all rooms.</li><li>Select a matching device and entity, or browse matching entities without a physical device. Scenes and scripts open directly.</li><li>Choose the click action, refresh preference and display. Enable Read only for a status tile.</li></ol>
<p>The detail window identifies sample/server data, shows available controls and reports Sending, Waiting for confirmation, Confirmed or failure. Readings and artwork come from confirmed state. Selected-entity subscriptions share polling and are released when tiles close or change target.</p>
<p class="catalog-note">Room, control, subscription-cleanup and reconnect checks passed in Workbench using sample devices. Real-device behaviour and physical Amiga performance remain separate checks. No connection token is stored in NovaDock.</p></section>
<section id="screenshots" class="section-heading"><p class="eyebrow">See it on Workbench</p><h2>A closer look.</h2><p>Real application windows and dock views from NovaDock 0.3. Select an image to see it at full size.</p>
<?php $shots = require __DIR__.'/includes/screenshot_catalog.php'; ?>
<table class="gallery" width="100%" border="0" cellpadding="10" cellspacing="4"><tbody>
<?php foreach (array_chunk($shots, 2) as $row): ?><tr>
<?php foreach ($row as $shot): ?><td width="50%" valign="top"<?php if ($shot['wide']): ?> class="gallery-wide"<?php endif; ?>><b><?= site_escape($shot['title']) ?></b><a href="<?= site_escape('screenshots/'.$shot['file']) ?>"><img src="<?= site_escape('screenshots/'.$shot['file']) ?>" width="<?= $shot['width'] ?>" height="<?= $shot['height'] ?>" loading="lazy" alt="<?= site_escape($shot['alt']) ?>"></a><p><?= site_escape($shot['description']) ?></p><small><a href="<?= site_escape('screenshots/'.$shot['file']) ?>">View full image &rarr;</a></small></td><?php endforeach; ?>
</tr><?php endforeach; ?>
</tbody></table></section>
<div class="closing"><h2>Pick your favourites.</h2><p>Every bundled plugin and theme comes with NovaDock.</p><p class="inline-links"><a href="get.php">Get NovaDock &rarr;</a><a href="marketplace.php">Explore the collection &rarr;</a><a href="developers.php">Make something of your own &rarr;</a></p></div>
<section id="saver" class="section-heading"><p class="eyebrow">New in NovaDock 0.3</p><h2>A break, the Amiga way.</h2><p>SAVER brings copper waves, rotating vectors, fireworks and a maze that solves itself. Choose a favourite or let Demo shuffle cycle through your selected scenes. Any key or click returns to Workbench.</p></section>
<div class="scene-gallery">
<figure><picture><source media="(prefers-reduced-motion: reduce)" srcset="screenshots/saver-copper-still.gif"><img src="screenshots/saver-copper.gif" width="320" height="160" loading="lazy" alt="Copper waves with colourful raster bars and stars"></picture><figcaption>Copper waves &middot; Raster bars, sine waves and stars.</figcaption></figure>
<figure><picture><source media="(prefers-reduced-motion: reduce)" srcset="screenshots/saver-vectors-still.gif"><img src="screenshots/saver-vectors.gif" width="320" height="160" loading="lazy" alt="Rotating wireframe cube, pyramid and checkmark"></picture><figcaption>Vector show &middot; Cubes, pyramids and a checkmark.</figcaption></figure>
<figure><picture><source media="(prefers-reduced-motion: reduce)" srcset="screenshots/saver-fireworks-still.gif"><img src="screenshots/saver-fireworks.gif" width="320" height="160" loading="lazy" alt="Colourful fireworks above a night skyline"></picture><figcaption>Fireworks &middot; A small celebration above the city.</figcaption></figure>
<figure><picture><source media="(prefers-reduced-motion: reduce)" srcset="screenshots/saver-maze-still.gif"><img src="screenshots/saver-maze.gif" width="320" height="160" loading="lazy" alt="A maze builds its passages and a character walks to its exit"></picture><figcaption>Workbench maze &middot; Watch it build, then follow the solution.</figcaption></figure>
</div><p class="catalog-note">These are previews rendered from SAVER's actual artwork. Display colours depend on your Workbench screen.</p>
<section class="section-heading"><p class="eyebrow">New in NovaDock 0.3</p><h2>Find something on Aminet.</h2><p class="lead">AMINET has its own native browser: recent uploads, categories, search, package details, readmes and downloads. Choose where each package is saved.</p><p><a href="marketplace.php#plugin-aminet">Explore AMINET &rarr;</a></p></section>
<section class="section-heading"><p class="eyebrow">New in NovaDock 0.3</p><h2>A few more favourites.</h2><p>Twelve more icons join the included library. Choose one for a collection or an action tile, or use your own picture.</p><div class="icon-rail">
<figure><img src="images/icon-aminet.gif" width="32" height="32" alt="Aminet pixel-art icon"><figcaption>Aminet</figcaption></figure>
<figure><img src="images/icon-downloads.gif" width="32" height="32" alt="Downloads pixel-art icon"><figcaption>Downloads</figcaption></figure>
<figure><img src="images/icon-search.gif" width="32" height="32" alt="Search pixel-art icon"><figcaption>Search</figcaption></figure>
<figure><img src="images/icon-radio.gif" width="32" height="32" alt="Radio pixel-art icon"><figcaption>Radio</figcaption></figure>
<figure><img src="images/icon-feeds.gif" width="32" height="32" alt="Feeds pixel-art icon"><figcaption>Feeds</figcaption></figure>
<figure><img src="images/icon-printer.gif" width="32" height="32" alt="Printer pixel-art icon"><figcaption>Printer</figcaption></figure>
<figure><img src="images/icon-backup.gif" width="32" height="32" alt="Backup pixel-art icon"><figcaption>Backup</figcaption></figure>
<figure><img src="images/icon-calendar.gif" width="32" height="32" alt="Calendar pixel-art icon"><figcaption>Calendar</figcaption></figure>
<figure><img src="images/icon-terminal.gif" width="32" height="32" alt="Terminal pixel-art icon"><figcaption>Terminal</figcaption></figure>
<figure><img src="images/icon-diskspace.gif" width="32" height="32" alt="DiskSpace pixel-art icon"><figcaption>DiskSpace</figcaption></figure>
<figure><img src="images/icon-mail.gif" width="32" height="32" alt="Mail pixel-art icon"><figcaption>Mail</figcaption></figure>
<figure><img src="images/icon-camera.gif" width="32" height="32" alt="Camera pixel-art icon"><figcaption>Camera</figcaption></figure>
</div></section>
<!-- /content -->
<?php site_footer(); ?>
