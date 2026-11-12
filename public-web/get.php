<?php
require_once __DIR__ . '/includes/layout.php';
site_header('Get NovaDock', 'Get NovaDock', 'Download the latest NovaDock release for Classic AmigaOS, the Plugin SDK, or the tagged source.');
?>
<!-- content -->
<?php
require_once __DIR__.'/includes/releases.php';
$catalog = nd_release_catalog();
$releases = $catalog['releases'];
$latest = $releases[0] ?? null;
?>
<section class="download-section">
<p class="eyebrow">Ready for your Workbench</p>
<h1>Get NovaDock.</h1>
<p class="lead">Download the dock, unpack it, and make yourself at home.</p>
<div class="release-meta"><span>AmigaOS 3.0+</span><span>68000 or better</span><span>Free &middot; MIT licensed</span></div>
<?php if ($latest): ?>
<div class="download-panel">
<div class="download-heading"><img src="images/logo.gif" width="108" height="44" alt=""><div><p class="eyebrow">Classic AmigaOS</p><h2>NovaDock <?= site_escape($latest['version']) ?></h2><p><?php if ($latest['date']): ?>Published <?= site_escape($latest['date']) ?><?php endif; ?><?php if ($latest['prerelease']): ?> <span class="preview-label">Preview release</span><?php endif; ?></p></div></div>
<div class="download-action"><?php if ($latest['binary_url']): ?><a class="button" href="<?= site_escape($latest['binary_url']) ?>">Download for AmigaOS <span aria-hidden="true">&darr;</span></a><span class="download-size">LHA<?php if ($latest['binary_size'] !== null): ?> &middot; <?= site_escape(nd_release_size($latest['binary_size'])) ?><?php endif; ?></span><?php else: ?><span class="unavailable">LHA not available</span><?php endif; ?><?php if ($latest['checksums_url']): ?><a class="checksum-link" href="<?= site_escape($latest['checksums_url']) ?>">SHA-256 checksum</a><?php endif; ?></div>
<p class="download-caption">The dock, bundled plugins, themes, installer and help in one download.</p>
<div class="release-links"><a href="<?= site_escape($latest['release_url']) ?>">Release notes &#8599;</a><a href="<?= site_escape($latest['source_url']) ?>">Source ZIP</a><a href="<?= site_escape($latest['source_tar_url']) ?>">Source tar.gz</a><a href="https://github.com/retrocs-org/novadock/releases">All releases &#8599;</a><a href="<?= site_escape('https://github.com/retrocs-org/novadock/tree/'.rawurlencode($latest['tag'])) ?>">Browse source &#8599;</a></div>
</div>
<p class="download-caption"><?php if ($catalog['stale']): ?>Showing saved release information while GitHub is unavailable.<?php else: ?>Downloads follow the latest published GitHub release automatically.<?php endif; ?></p>
<div class="sdk-download"><div><p class="eyebrow">Build something useful</p><h2>Plugin SDK</h2><p>Headers, client helpers and a working example.</p><a href="plugin-sdk.php">SDK guide &#8599;</a></div><div class="download-action"><?php if ($latest['sdk_url']): ?><a class="button secondary" href="<?= site_escape($latest['sdk_url']) ?>">Download SDK ZIP <span aria-hidden="true">&darr;</span></a><span class="download-size"><?php if ($latest['sdk_size'] !== null): ?><?= site_escape(nd_release_size($latest['sdk_size'])) ?><?php endif; ?></span><?php endif; ?><?php if ($latest['sdk_checksums_url']): ?><a class="checksum-link" href="<?= site_escape($latest['sdk_checksums_url']) ?>">SHA-256 checksum</a><?php endif; ?></div></div>
<details class="setup-details"><summary>Installation and checksum verification</summary><div class="install-columns"><div><h3>Install NovaDock</h3><ol><li>Unpack the LHA and open the NovaDock drawer.</li><li>Double-click <b>Install-NovaDock</b>.</li></ol><p>The graphical installer needs Installer 42 or newer. For a Shell installation:</p><pre class="code">Execute install.amiga</pre></div><div><h3>Verify your download</h3><p>Save the matching SHA-256 file beside the download. On a computer with shasum, open that folder and run:</p><pre class="code">shasum -a 256 -c NovaDock-<?= site_escape($latest['version']) ?>.lha.sha256</pre><p>A matching download reports <b>OK</b>. The SDK ZIP has its own checksum file.</p></div></div></details>
<?php if (count($releases)>1): ?><details class="older-releases"><summary>Earlier releases</summary><?php foreach (array_slice($releases, 1) as $release): ?><div class="older-release"><strong>NovaDock <?= site_escape($release['version']) ?></strong><span><?php if ($release['binary_url']): ?><a href="<?= site_escape($release['binary_url']) ?>">LHA</a> &middot; <?php endif; ?><a href="<?= site_escape($release['release_url']) ?>">Release notes</a></span></div><?php endforeach; ?></details><?php endif; ?>
<?php else: ?><p class="note">No downloads are listed yet. Check <a href="https://github.com/retrocs-org/novadock/releases">GitHub releases</a> for availability.</p><?php endif; ?>
<p class="get-help">Build the app from source in <a href="developers.php#source">Developers</a>, or visit <a href="support.php">Support</a> for help.</p>
</section>
<!-- /content -->
<?php site_footer(); ?>
