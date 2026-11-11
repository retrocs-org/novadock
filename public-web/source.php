<?php
require_once __DIR__ . '/includes/layout.php';
site_header('Source', 'Source', 'Get the NovaDock source and build the application on AmigaOS or with a cross toolchain.');
?>
<!-- content -->
<p class="eyebrow">From source to Workbench</p>
<h1>Build NovaDock<br>from the ground up.</h1>
<p class="lead">Get the source, set up your tools and compile the application. Looking to create a plugin or theme instead? Start with <a href="developers.php">Developers</a>.</p>
<div class="dev-section">
<h2>1. Get the source</h2>
<p><a class="button" href="<?= site_escape('https://github.com/retrocs-org/novadock/archive/refs/tags/'.rawurlencode(site_release_tag()).'.zip') ?>">Source ZIP</a> <a class="text-link" href="<?= site_escape('https://github.com/retrocs-org/novadock/tree/'.rawurlencode(site_release_tag())) ?>">Browse the tagged source &rarr;</a></p>
<p>Unpack the ZIP and enter its source directory. Or clone the same tag:</p>
<pre class="code">git clone --branch <?= site_escape(site_release_tag()) ?> https://github.com/retrocs-org/novadock.git
cd novadock</pre>
<p><small>The tagged links become available when the <?= site_escape(NOVADOCK_VERSION) ?> release is published. See <a href="download.php">Downloads</a> for release status.</small></p>
</div>
<div class="dev-section">
<h2>2. Set up the toolchain</h2>
<p>Use vbcc with the m68k-amigaos target, vasm, vlink and a compatible Amiga NDK. Configure the compiler before building. NovaDock targets Classic AmigaOS 3.x and baseline 68000 processors.</p>
<h3>On AmigaOS</h3>
<p>Install a working AmigaOS vbcc toolchain and NDK, then open a Shell in the source directory. A build-capable installation needs enough free memory and disk space for the compiler and outputs.</p>
<h3>On a Linux host</h3>
<p>Install a host C compiler, make, Git, curl and lhasa using your distribution's package manager. The included setup script builds the cross compiler and obtains the NDK:</p>
<pre class="code">sh scripts/setup-vbcc-toolchain.sh ~/amiga-toolchain
export VBCC=~/amiga-toolchain/vbcc
export PATH="$VBCC/bin:$PATH"</pre>
<p>Already have a cross toolchain? Set <b>VBCC</b> to its installation directory. The optional <b>VC</b> variable selects a custom compiler driver.</p>
</div>
<div class="dev-section">
<h2>3. Compile</h2>
<p>On AmigaOS:</p><pre class="code">Execute build.amiga</pre>
<p>On a host with the configured cross toolchain:</p><pre class="code">sh scripts/build-amiga.sh -B</pre>
<p>This builds the dock, Preferences, 19 plugins and the setup utilities: 25 Amiga programs in total. Network includes the Roadshow connection controls; there is no separate Roadshow plugin.</p>
</div>
<div class="dev-section">
<h2>4. Check and stage</h2>
<p>Host tests require a C compiler and Python 3. PHP is also required when checking the website. From the source directory, run:</p>
<pre class="code">SANITIZE=1 sh tests/run_host_tests.sh</pre>
<p>Optional native source checks need m68k-amigaos GCC and AmigaOS headers. Set <b>M68K_CC</b> and <b>OPENNDK</b> for your installation:</p>
<pre class="code">sh tests/amiga_syntax_check.sh</pre>
<p>To assemble a runnable drawer on a transfer volume:</p>
<pre class="code">sh scripts/build-and-stage-amiga.sh /path/to/transfer-volume</pre>
<p>On your Amiga, open that NovaDock drawer and run <b>Install-NovaDock</b>. Automated checks do not replace testing on the AmigaOS configurations you support.</p>
</div>
<div class="dev-section">
<h2>Package a release</h2>
<p>The release tool has separate plan, build, check and prepare commands, plus a source-push command. Publishing requires explicit confirmation and uses the same verified files for GitHub, GitLab and optional Aminet submission.</p>
<p>The source includes <b>scripts/release.py</b> for compiling, testing, packaging and publishing tagged releases. LHA creation requires LHa for UNIX; an extractor alone is not enough.</p>
<pre class="code">python3 scripts/release.py build --output ../dist/release-<?= site_escape(NOVADOCK_VERSION) ?></pre>
<p>Preparation stays local. See <b>DEVELOPMENT.md</b> and <b>docs/RELEASING.md</b> in the source tree for complete setup, release credentials and publication steps.</p>
</div>
<!-- /content -->
<?php site_footer(); ?>
