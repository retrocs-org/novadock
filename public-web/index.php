<?php
require_once __DIR__ . '/includes/layout.php';
site_header('Home', 'Home', 'NovaDock 0.5 - a Classic AmigaOS Workbench dock with plugin support.');
?>
<!-- content -->
<?php $plugins = require __DIR__.'/includes/plugin_catalog.php'; ?>
<section class="intro hero-split">
<div><p class="eyebrow">A little more Workbench</p>
<h1>Make it<br><span>feel like yours.</span></h1>
<p class="lead">Your favourite apps. Useful little tools. A dock that feels right at home on your Amiga.</p>
<p class="actions"><a class="button" href="get.php">Get NovaDock <span aria-hidden="true">&darr;</span></a><a class="text-link" href="features.php#screenshots">Take a look &rarr;</a></p>
<?php if (site_is_development()): ?><p class="catalog-note">NovaDock 0.5 is in development. <a href="get.php">Downloads</a> lists the latest published packages.</p><?php endif; ?>
<?php if (site_version_is_upcoming('0.4')): ?>
<p class="catalog-note">Coming in 0.4: Home Assistant room filters and controls, CALC, TOOLTYPES, WATCH, LOGS, SHELF, ALERTS and AmiRocket, plus update checks and hardware improvements. <a href="features.php#next">Explore what is coming &rarr;</a></p><?php endif; ?>
<p class="compatibility">AmigaOS 3.0+ &middot; 68000 or better &middot; No RTG or MUI required</p></div>
<figure class="hero-scene"><a href="features.php#saver"><picture><source media="(prefers-reduced-motion: reduce)" srcset="screenshots/saver-boing-still.gif"><img src="screenshots/saver-boing.gif" width="320" height="160" alt="The rotating red-and-white Boing Ball bouncing over a grid"></picture></a><figcaption>New in 0.3 &middot; A little demo-scene spirit, with SAVER.</figcaption></figure>
</section>
<figure class="hero-capture"><a href="features.php#screenshots"><img src="screenshots/dock-03.gif" width="1005" height="90" alt="NovaDock with Music, ARCHIVE, Weather, AMINET, CPU, RAM, Clock, Network and SAVER"></a><figcaption>A look at 0.3 on Workbench. Add just what you need, in the order you like.</figcaption></figure>
<section class="release-intro" aria-labelledby="new-release">
<div><p class="eyebrow">New in 0.3</p><h2 id="new-release">More to explore.<br>Still your Workbench.</h2></div>
<div><p>Browse Aminet in a native window. Build your own music and WHDLoad collections. Try a classic screen saver, or add hardware sensors and quick system actions.</p><a class="text-link" href="get.php">See published releases &rarr;</a></div>
</section>
<table class="story" width="100%" cellpadding="14" cellspacing="0"><tr>
<td width="42%" valign="middle"><p class="eyebrow">Notes</p><h2>A thought worth keeping.</h2><p>Write a quick note, add a heading or highlight a few words. Switch to Markdown whenever you like. Your notes stay saved after restarting.</p><a href="marketplace.php#plugin-notes">Meet Notes &rarr;</a></td>
<td><figure><a href="screenshots/notes-editor.gif"><img src="screenshots/notes-editor.gif" width="620" height="400" loading="lazy" alt="Notes with a white writing page, formatted headings and editing controls"></a><figcaption>A proper place for your Workbench ideas.</figcaption></figure></td>
</tr></table>
<table class="story reverse" width="100%" cellpadding="14" cellspacing="0"><tr>
<td width="42%" valign="middle"><p class="eyebrow">Weather</p><h2>A little outlook.</h2><p>Today's conditions in your dock. Click for five days ahead, with conditions, precipitation chances and rain/snow amounts.</p><a href="marketplace.php#plugin-weather">Set up Weather &rarr;</a></td>
<td><figure><a href="screenshots/weather-forecast.gif"><img src="screenshots/weather-forecast.gif" width="630" height="252" loading="lazy" alt="Weather showing Stockholm conditions and a five-day forecast"></a><figcaption>The refreshed 0.3 forecast: conditions, chances and rain/snow amounts.</figcaption></figure></td>
</tr></table>
<table class="story" width="100%" cellpadding="14" cellspacing="0"><tr>
<td width="42%" valign="middle"><p class="eyebrow">AMINET &middot; New in 0.3</p><h2>Find your next favourite.</h2><p>Browse recent uploads, explore categories or search for a package. Read its notes, choose a destination and follow the download progress.</p><a href="marketplace.php#plugin-aminet">Explore AMINET &rarr;</a></td>
<td><figure><a href="screenshots/aminet-download.gif"><img src="screenshots/aminet-download.gif" width="740" height="460" loading="lazy" alt="AMINET browser showing a download progress bar with percentage and received size"></a><figcaption>Real download progress, right on Workbench.</figcaption></figure></td>
</tr></table>
<table class="story" width="100%" cellpadding="14" cellspacing="0"><tr>
<td width="42%" valign="middle"><p class="eyebrow">ARCHIVE</p><h2>Drop. Pack. Unpack.</h2><p>Drop files and drawers together to make one ZIP or LHA. Drop an archive to unpack it. Choose where it goes and keep its Workbench icons.</p><a href="marketplace.php#plugin-archiveextractor">Explore ARCHIVE &rarr;</a></td>
<td><figure><a href="screenshots/archive-create.gif"><img src="screenshots/archive-create.gif" width="560" height="206" loading="lazy" alt="ARCHIVE compression window with format, destination, filename and Workbench icon options"></a><figcaption>One place for packing and unpacking.</figcaption></figure></td>
</tr></table>
<section class="collection-preview"><p class="eyebrow">Make it your own</p><h2>A useful little collection.</h2><p>Explore <?= count(array_filter($plugins, static function ($plugin) { return !site_plugin_is_upcoming($plugin); })) ?> released plugins and thirteen themes. Add a timer, watch your system or build a collection of your own. <?php $upcoming = count(array_filter($plugins, 'site_plugin_is_upcoming')); if ($upcoming): ?>A further <?= $upcoming ?> plugins are planned for newer releases.<?php endif; ?></p><p class="inline-links"><a href="marketplace.php#plugins">Browse plugins &rarr;</a><a href="marketplace.php#themes">Find your theme &rarr;</a><a href="features.php#screenshots">More screenshots &rarr;</a></p></section>
<div class="closing"><h2>At home on your Amiga.</h2><p>Download the LHA and double-click <b>Install</b> (<b>Install-NovaDock</b> in older archives). The installer handles new installs, updates and removal.</p><p><a class="button" href="get.php">Get NovaDock <span aria-hidden="true">&darr;</span></a> <a class="text-link" href="support.php">Need a hand? &rarr;</a></p></div>
<!-- /content -->
<?php site_footer(); ?>
