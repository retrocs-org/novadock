<?php
require_once __DIR__ . '/includes/layout.php';
site_header('Features', 'Features', 'A short guide to NovaDock launchers, themes, plugins and settings.');
?>
<!-- content -->
<p class="eyebrow">Made for your daily Workbench</p>
<h1>Useful by nature.<br>Yours by design.</h1>
<p class="lead">Shortcuts where you want them. Tools when you need them.</p>
<p class="category-links"><a href="#features">Features</a> &middot; <a href="#screenshots">Screenshots</a> &middot; <a href="marketplace.php#plugins">All plugins</a></p>
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
<p class="note">AmigaOS 3.0 or later, with a 68000 or better. Some plugins need additional software or hardware; each marketplace listing explains what is needed.</p>
<section id="screenshots" class="section-heading"><p class="eyebrow">See it on Workbench</p><h2>A closer look.</h2><p>Real application windows and dock views. Select an image to see it at full size.</p>
<?php $shots = require __DIR__.'/includes/screenshot_catalog.php'; ?>
<table class="gallery" width="100%" border="0" cellpadding="10" cellspacing="4"><tbody>
<?php foreach (array_chunk($shots, 2) as $row): ?><tr>
<?php foreach ($row as $shot): ?><td width="50%" valign="top"<?php if ($shot['wide']): ?> class="gallery-wide"<?php endif; ?>><b><?= site_escape($shot['title']) ?></b><a href="<?= site_escape('screenshots/'.$shot['file']) ?>"><img src="<?= site_escape('screenshots/'.$shot['file']) ?>" width="<?= $shot['width'] ?>" height="<?= $shot['height'] ?>" loading="lazy" alt="<?= site_escape($shot['alt']) ?>"></a><p><?= site_escape($shot['description']) ?></p><small><a href="<?= site_escape('screenshots/'.$shot['file']) ?>">View full image &rarr;</a></small></td><?php endforeach; ?>
</tr><?php endforeach; ?>
</tbody></table></section>
<div class="closing"><h2>Pick your favourites.</h2><p>Every bundled plugin and theme comes with NovaDock.</p><p class="inline-links"><a href="get.php">Get NovaDock &rarr;</a><a href="marketplace.php">Explore the collection &rarr;</a><a href="developers.php">Make something of your own &rarr;</a></p></div>
<section id="saver" class="section-heading"><p class="eyebrow">Coming in NovaDock 0.3</p><h2>A break, the Amiga way.</h2><p>SAVER brings copper waves, rotating vectors, fireworks and a maze that solves itself. Choose a favourite or let Demo shuffle cycle through your selected scenes. Any key or click returns to Workbench.</p></section>
<div class="scene-gallery">
<figure><picture><source media="(prefers-reduced-motion: reduce)" srcset="screenshots/saver-copper-still.gif"><img src="screenshots/saver-copper.gif" width="320" height="160" loading="lazy" alt="Copper waves with colourful raster bars and stars"></picture><figcaption>Copper waves &middot; Raster bars, sine waves and stars.</figcaption></figure>
<figure><picture><source media="(prefers-reduced-motion: reduce)" srcset="screenshots/saver-vectors-still.gif"><img src="screenshots/saver-vectors.gif" width="320" height="160" loading="lazy" alt="Rotating wireframe cube, pyramid and checkmark"></picture><figcaption>Vector show &middot; Cubes, pyramids and a checkmark.</figcaption></figure>
<figure><picture><source media="(prefers-reduced-motion: reduce)" srcset="screenshots/saver-fireworks-still.gif"><img src="screenshots/saver-fireworks.gif" width="320" height="160" loading="lazy" alt="Colourful fireworks above a night skyline"></picture><figcaption>Fireworks &middot; A small celebration above the city.</figcaption></figure>
<figure><picture><source media="(prefers-reduced-motion: reduce)" srcset="screenshots/saver-maze-still.gif"><img src="screenshots/saver-maze.gif" width="320" height="160" loading="lazy" alt="A maze builds its passages and a character walks to its exit"></picture><figcaption>Workbench maze &middot; Watch it build, then follow the solution.</figcaption></figure>
</div><p class="catalog-note">These are previews rendered from SAVER's actual artwork. Display colours depend on your Workbench screen.</p>
<section class="section-heading"><p class="eyebrow">Coming in NovaDock 0.3</p><h2>Find something on Aminet.</h2><p class="lead">AMINET has its own native browser: recent uploads, categories, search, package details, readmes and downloads. Choose where each package is saved.</p><p><a href="marketplace.php#plugin-aminet">Explore AMINET &rarr;</a></p></section>
<section class="section-heading"><p class="eyebrow">Coming in NovaDock 0.3</p><h2>A few more favourites.</h2><p>Twelve more icons join the included library. Choose one for a collection or an action tile, or use your own picture.</p><div class="icon-rail">
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
