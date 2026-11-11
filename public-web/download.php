<?php
require_once __DIR__ . '/includes/layout.php';
site_header('Download', 'Download', 'NovaDock tagged releases: Amiga LHA binaries, source packages and release notes on GitHub.');
?>
<!-- content -->
<?php $releases = json_decode(file_get_contents(__DIR__ . '/releases.json'), true, 512, JSON_THROW_ON_ERROR); ?>
<p class="eyebrow">Get NovaDock</p>
<h1>One release.<br>Everything you need.</h1>
<p class="lead">Choose the LHA to install NovaDock, or the source package to build it yourself. Files and release notes are hosted on GitHub.</p>
<?php foreach ($releases as $release): ?>
<div class="release-entry">
<h2>NovaDock <?= site_escape($release['version']) ?> <small><?= $release['status'] === 'upcoming' ? 'Upcoming' : ($release['prerelease'] ? 'Preview' : 'Release') ?></small></h2>
<?php if ($release['date']): ?><p class="release-date"><?= site_escape($release['date']) ?></p><?php endif; ?>
<p class="actions"><?php if ($release['binary_url']): ?><a class="button" href="<?= site_escape($release['binary_url']) ?>">LHA package</a><?php endif; ?> <?php if ($release['source_url']): ?><a class="text-link" href="<?= site_escape($release['source_url']) ?>">Source ZIP &rarr;</a><?php endif; ?> <?php if (!empty($release['source_tar_url'])): ?><a class="text-link" href="<?= site_escape($release['source_tar_url']) ?>">Source tar.gz &rarr;</a><?php endif; ?></p>
<p><a href="<?= site_escape($release['release_url']) ?>">Tagged release <?= site_escape($release['tag']) ?> &rarr;</a><?php if ($release['checksums_url']): ?> &nbsp; <a href="<?= site_escape($release['checksums_url']) ?>">LHA checksum</a><?php endif; ?></p>
<?php if ($release['status'] === 'upcoming'): ?><p class="note">This tag has not been published yet. These links will become available when the release is uploaded.</p><?php endif; ?>
</div>
<?php endforeach; ?>
<h2>Install in two steps</h2>
<ol><li>Unpack the LHA and open the NovaDock drawer.</li><li>Double-click <b>Install-NovaDock</b> and follow the instructions.</li></ol>
<p>Requires AmigaOS 3.0+ and a 68000 or better. The graphical installer requires Installer 42 or newer; otherwise use <b>Execute install.amiga</b> from the drawer.</p>
<p>Updating keeps your settings. Quit and restart the dock and its plugins after replacing binaries.</p>
<p class="note">Building the app? See <a href="source.php">Source</a>. Creating plugins or themes? Visit <a href="developers.php">Developers</a>. For compatibility and setup questions, see the <a href="support.php#faq">FAQ</a>.</p>
<!-- /content -->
<?php site_footer(); ?>
