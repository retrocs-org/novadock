<?php
require_once __DIR__ . '/includes/layout.php';
site_header('Developers', 'Developers', 'Choose a guide to creating NovaDock plugins or themes.');
?>
<!-- content -->
<p class="eyebrow">For makers</p>
<h1>Make NovaDock<br>your own.</h1>
<p class="lead">Write a plugin, create a theme, or build the dock from source.</p>
<p class="category-links"><a href="#plugins">Plugin SDK</a> &middot; <a href="#themes">Theme development</a> &middot; <a href="#source">Build from source</a></p>
<div class="dev-section" id="plugins"><h2>Plugin SDK</h2><p>Create an AmigaOS program that adds a useful tile. Learn the message lifecycle, manifest format, right-click settings window and packaging.</p><p><a class="button" href="plugin-sdk.php">Explore the Plugin SDK &rarr;</a> <a class="text-link" href="plugin-sdk.php#settings">Add a settings window</a></p></div>
<div class="dev-section" id="themes"><h2>Theme development</h2><p>Change colours, shape, placement and typography with one text file. Learn the format, preview your design and package it for others.</p><p><a class="button" href="theme-development.php">Create a theme &rarr;</a></p></div>

<section id="source" class="section-heading"><p class="eyebrow">Build it yourself</p><h2>Build NovaDock from source</h2><p>Get the code, compile it and install it on your Amiga.</p><div class="dev-section"><h3>1. Get the source</h3>
<p><a class="button" href="<?= site_escape('https://github.com/retrocs-org/novadock/archive/refs/tags/'.rawurlencode(site_release_tag()).'.zip') ?>">Download source ZIP</a> <a class="text-link" href="<?= site_escape('https://github.com/retrocs-org/novadock/tree/'.rawurlencode(site_release_tag())) ?>">Browse the source &rarr;</a></p>
<p>Unpack the ZIP and open its source directory. Or clone the tagged source:</p>
<pre class="code">git clone --branch <?= site_escape(site_release_tag()) ?> https://github.com/retrocs-org/novadock.git
cd novadock</pre></div>
<div class="dev-section"><h3>2. Compile</h3>
<p>You need vbcc for m68k-amigaos, vasm, vlink and compatible Amiga NDK headers. The code targets a 68000 and AmigaOS 3.0 or later.</p>
<h4>On the Amiga</h4><p>Configure your vbcc installation, then run this from the source drawer:</p>
<pre class="code">Execute build.amiga</pre>
<h4>On a host computer</h4><p>With the cross toolchain configured:</p>
<pre class="code">export VBCC=/path/to/vbcc
export PATH="$VBCC/bin:$PATH"
sh scripts/build-amiga.sh -B</pre>
<p>If you need to set up a Linux cross toolchain, the source includes:</p>
<pre class="code">sh scripts/setup-vbcc-toolchain.sh ~/amiga-toolchain</pre>
<p>This builds NovaDock, Preferences, the bundled plugins and installation tools.</p></div>
<div class="dev-section"><h3>3. Install on your Amiga</h3>
<p>From the compiled source drawer, double-click <b>Install-NovaDock</b>. The graphical installer requires Installer 42 or newer.</p>
<p>For a Shell installation, run:</p>
<pre class="code">Execute install.amiga</pre>
<p>Built on another computer? Copy a ready-to-install drawer to your transfer volume:</p>
<pre class="code">ND_SKIP_BUILD=1 sh scripts/build-and-stage-amiga.sh /path/to/transfer-volume</pre>
<p>On the Amiga, open the transferred NovaDock drawer and run the installer. Quit NovaDock and its plugins before replacing an existing installation.</p></div>
</section>
<div class="community"><h2>Share something useful.</h2><p>The <a href="marketplace.php">marketplace</a> lists bundled plugins and themes. We plan to feature reviewed community contributions in both sections.</p></div>
<!-- /content -->
<?php site_footer(); ?>
