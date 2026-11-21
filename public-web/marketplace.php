<?php
require_once __DIR__ . '/includes/layout.php';
site_header('Marketplace', 'Marketplace', 'Explore NovaDock plugins and themes, with bundled items and future community additions.');
?>
<!-- content -->
<?php
$catalog = require __DIR__ . '/includes/plugin_catalog.php';
$themes = require __DIR__ . '/includes/theme_catalog.php';
?>
<p class="eyebrow">The NovaDock marketplace</p>
<h1>Make room for<br>something you like.</h1>
<p class="lead">Explore every bundled plugin and thirteen themes. Each plugin shows its own version.</p>
<p class="market-sections"><a href="#plugins">Plugins <small><?= count($catalog) ?></small></a> <a href="#themes">Themes <small><?= count($themes) ?></small></a></p>
<h2 id="plugins">Plugins</h2>
<p class="catalog-note">Add plugins from Preferences &gt; Organize &gt; Add Item &gt; Plugins. <a href="get.php">Download NovaDock</a>.</p>
<p class="category-links"><a href="#everyday">Everyday</a> &middot; <a href="#system">System</a> &middot; <a href="#media-apps">Media &amp; apps</a> &middot; <a href="#tools-fun">Tools &amp; fun</a></p>
<?php foreach (['Everyday' => 'everyday', 'System' => 'system', 'Media & apps' => 'media-apps', 'Tools & fun' => 'tools-fun'] as $group => $anchor): ?>
<h2 id="<?= site_escape($anchor) ?>"><?= site_escape($group) ?></h2>
<div class="plugin-list">
<?php foreach ($catalog as $plugin): if ($plugin['group'] !== $group) continue; ?>
<div class="plugin-entry" id="plugin-<?= strtolower(site_escape($plugin['executable'])) ?>" data-executable="<?= site_escape($plugin['executable']) ?>"><h3><?= site_escape($plugin['name']) ?> <small>Version <?= site_escape($plugin['version']) ?></small></h3><p><?= site_escape($plugin['description']) ?></p><?php if (isset($plugin['website'])): ?><p><a href="<?= site_escape($plugin['website']) ?>">BitSkim website &rarr;</a></p><?php endif; ?><?php if ($plugin['requires'] !== ''): ?><small><?= site_escape($plugin['requires']) ?></small><?php endif; ?></div>
<?php endforeach; ?>
</div>
<?php endforeach; ?>
<p class="note">No community plugins are listed yet.</p>
<h2 id="themes" class="section-heading">Themes</h2>
<p class="lead">Thirteen colour palettes for your Workbench.</p>
<p class="catalog-note">Choose Preferences &gt; Appearance, then Test or Save. These are the real theme colours.</p>
<div class="theme-list">
<?php foreach ($themes as $theme): ?>
<div class="theme-entry" data-theme-id="<?= site_escape($theme['id']) ?>"><div class="palette" aria-label="<?= site_escape($theme['name']) ?> palette"><?php foreach ($theme['palette'] as $colour): ?><span style="background-color:<?= site_escape($colour) ?>"></span><?php endforeach; ?></div><h3><?= site_escape($theme['name']) ?> <small>Bundled</small></h3><p><?= site_escape($theme['description']) ?></p><small><?= site_escape(ucfirst($theme['shape'])) ?> tiles</small></div>
<?php endforeach; ?>
</div>
<p class="note">No community themes are listed yet.</p>
<div class="community"><p class="eyebrow">Made by the community</p><h2>Make your own.</h2><p>The <a href="developers.php">developer guide</a> covers plugins, themes and packaging.</p></div>
<!-- /content -->
<?php site_footer(); ?>
