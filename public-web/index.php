<?php
require_once __DIR__ . '/includes/layout.php';
site_header('Home', 'Home', 'NovaDock 0.2 - a Classic AmigaOS Workbench dock with plugin support.');
?>
<!-- content -->
<?php $plugins = require __DIR__.'/includes/plugin_catalog.php'; ?>
<div class="intro">
<p class="eyebrow">A dock for Classic AmigaOS</p>
<h1>Your Workbench.<br><span>Your way.</span></h1>
<p class="lead">Your apps, a few useful tools, and a little more room to get things done.</p>
<p class="actions"><a class="button" href="get.php">Get NovaDock <span aria-hidden="true">&darr;</span></a> <a class="text-link" href="features.php#screenshots">See it in action &rarr;</a></p>
<p class="compatibility">AmigaOS 3.0+ &middot; 68000 or better &middot; No RTG or MUI required</p>
<figure class="hero-capture"><a href="features.php#screenshots"><img src="screenshots/dock-02.gif" width="1010" height="90" alt="NovaDock 0.2 with ARCHIVE, Timer, Notes, CPU, RAM, Clock, Trash, SHOT and Weather"></a><figcaption>Your tools. Your order. Add just what you need.</figcaption></figure>
</div>
<section class="release-intro" aria-labelledby="new-release">
<div><p class="eyebrow">New in NovaDock 0.2</p><h2 id="new-release">Small additions.<br>Everyday favourites.</h2></div>
<div><p>Weather, lasting notes and a handy countdown. A smarter archiver. Music controls that now work with LumiAMP.</p><a class="text-link" href="https://github.com/retrocs-org/novadock/releases/tag/0.2">Read the release notes &rarr;</a></div>
</section>
<table class="story" width="100%" cellpadding="14" cellspacing="0"><tr>
<td width="42%" valign="middle"><p class="eyebrow">Notes</p><h2>A thought worth keeping.</h2><p>Write a quick note, add a heading or highlight a few words. Switch to Markdown whenever you like. Your notes stay saved after restarting.</p><a href="marketplace.php#plugin-notes">Meet Notes &rarr;</a></td>
<td><figure><a href="screenshots/notes-editor.gif"><img src="screenshots/notes-editor.gif" width="620" height="400" loading="lazy" alt="Notes with a white writing page, formatted headings and editing controls"></a><figcaption>A proper place for your Workbench ideas.</figcaption></figure></td>
</tr></table>
<table class="story reverse" width="100%" cellpadding="14" cellspacing="0"><tr>
<td width="42%" valign="middle"><p class="eyebrow">Weather</p><h2>A little outlook.</h2><p>Today's conditions in your dock. Click for the next five days, with real weekday names. Your last forecast stays available offline.</p><a href="marketplace.php#plugin-weather">Set up Weather &rarr;</a></td>
<td><figure><a href="screenshots/weather-forecast.gif"><img src="screenshots/weather-forecast.gif" width="560" height="220" loading="lazy" alt="Weather showing Stockholm conditions and a five-day forecast"></a><figcaption>Weather icons change with the conditions.</figcaption></figure></td>
</tr></table>
<table class="story" width="100%" cellpadding="14" cellspacing="0"><tr>
<td width="42%" valign="middle"><p class="eyebrow">ARCHIVE</p><h2>Drop. Pack. Unpack.</h2><p>Drop files and drawers together to make one ZIP or LHA. Drop an archive to unpack it. Choose where it goes and keep its Workbench icons.</p><a href="marketplace.php#plugin-archiveextractor">Explore ARCHIVE &rarr;</a></td>
<td><figure><a href="screenshots/archive-create.gif"><img src="screenshots/archive-create.gif" width="560" height="208" loading="lazy" alt="ARCHIVE compression window with format, destination, filename and Workbench icon options"></a><figcaption>One place for packing and unpacking.</figcaption></figure></td>
</tr></table>
<section class="collection-preview"><p class="eyebrow">Make it your own</p><h2>A useful little collection.</h2><p><?= count($plugins) ?> bundled plugins and twelve themes. Add a timer, keep an eye on memory, switch screens or control your music.</p><p class="inline-links"><a href="marketplace.php#plugins">Browse plugins &rarr;</a><a href="marketplace.php#themes">Find your theme &rarr;</a><a href="features.php#screenshots">More screenshots &rarr;</a></p></section>
<div class="closing"><h2>At home on your Amiga.</h2><p>Download the LHA and double-click <b>Install-NovaDock</b>. The installer handles new installs, updates and removal.</p><p><a class="button" href="get.php">Get NovaDock <span aria-hidden="true">&darr;</span></a> <a class="text-link" href="support.php">Need a hand? &rarr;</a></p></div>
<!-- /content -->
<?php site_footer(); ?>
