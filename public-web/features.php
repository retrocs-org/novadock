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
<div><h2>Choose your look</h2><p>Twelve themes, four tile shapes and two sizes. Preview your changes before saving.</p></div>
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
<!-- /content -->
<?php site_footer(); ?>
