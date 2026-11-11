<?php
require_once __DIR__ . '/includes/layout.php';
site_header('Developers', 'Developers', 'Choose a guide to creating NovaDock plugins or themes.');
?>
<!-- content -->
<p class="eyebrow">For makers</p>
<h1>Build a tool.<br>Make a theme.</h1>
<p class="lead">Two ways to make NovaDock your own. Start with a working example and follow the guide that fits your idea.</p>
<div class="dev-section" id="plugins"><h2>Plugin SDK</h2><p>Create an AmigaOS program that adds a useful tile. Learn the message lifecycle, manifest format, right-click settings window and packaging.</p><p><a class="button" href="plugin-sdk.php">Explore the Plugin SDK &rarr;</a> <a class="text-link" href="plugin-sdk.php#settings">Add a settings window</a></p></div>
<div class="dev-section" id="themes"><h2>Theme development</h2><p>Change colours, shape, placement and typography with one text file. Learn the format, preview your design and package it for others.</p><p><a class="button" href="theme-development.php">Create a theme &rarr;</a></p></div>
<p class="note">Building the application itself? Use the separate <a href="source.php">Source and build guide</a>. Need help using NovaDock? Visit <a href="support.php">Support</a>.</p>
<div class="community"><h2>Share something useful.</h2><p>The <a href="marketplace.php">marketplace</a> lists bundled plugins and themes. We plan to feature reviewed community contributions in both sections.</p></div>
<!-- /content -->
<?php site_footer(); ?>
