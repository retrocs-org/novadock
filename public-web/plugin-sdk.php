<?php
require_once __DIR__ . '/includes/layout.php';
site_header('Plugin SDK', 'Plugin SDK', 'Build, configure, test and package a NovaDock plugin, including its right-click settings window.');
?>
<!-- content -->
<p class="eyebrow"><a href="developers.php">Developers</a> / Plugin SDK</p>
<h1>Build your first plugin.</h1>
<p class="lead">Compile Hello, add your settings and package a plugin others can install.</p>
<p class="market-sections"><a href="#start">Get started</a> <a href="#lifecycle">Lifecycle</a> <a href="#settings">Settings window</a> <a href="#packaging">Packaging</a> <a href="#testing">Testing</a></p>
<div class="dev-section" id="plugins">
<h2 id="start">Start with Hello</h2>
<p>Plugins are separate AmigaOS programs. They send text, values and events through Exec message ports; NovaDock draws the tile.</p>
<p><a class="button" href="<?= site_escape('https://github.com/retrocs-org/novadock/releases/download/'.rawurlencode(site_release_tag()).'/NovaDock-'.NOVADOCK_VERSION.'-plugin-sdk.zip') ?>">Download Plugin SDK</a> <a class="text-link" href="<?= site_escape('https://github.com/retrocs-org/novadock/tree/'.rawurlencode(site_release_tag()).'/plugin-sdk') ?>">Browse SDK on GitHub &rarr;</a></p>
<p>The SDK is also part of the repository under <a href="<?= site_escape('https://github.com/retrocs-org/novadock/blob/'.rawurlencode(site_release_tag()).'/plugin-sdk/README.md') ?>">plugin-sdk/README.md</a>. These tagged links become available when the <?= site_escape(NOVADOCK_VERSION) ?> release is published.</p>
<h3>1. Build the Hello example</h3>
<p>The SDK includes a complete example, protocol headers, client helpers and a Makefile. Set up vbcc with an AmigaOS m68k target and compatible NDK, unpack the SDK, then run <b>make</b>. Or compile directly:</p>
<pre class="code">vc -O -Iinclude -o Hello examples/hello.c src/nd_client.c src/nd_option_runtime.c</pre>
<h3>2. Describe your plugin</h3>
<p>Keep the program and its matching <b>.ndplugin</b> manifest together. Choose your own unique ID and program name. This is the SDK example:</p>
<pre class="code">NOVADOCK_ID=HELLO
NOVADOCK_NAME=Hello
NOVADOCK_VERSION=1.0
NOVADOCK_AUTHOR=Your Name
NOVADOCK_DESCRIPTION=Shows a greeting and reacts to clicks.
NOVADOCK_LICENSE=MIT
NOVADOCK_FEATURES=meter,click,badge
NOVADOCK_PROGRAM=Hello
NOVADOCK_VIEW=METER
NOVADOCK_ABI=1
NOVADOCK_OPTION=GREETING|Greeting|TEXT||HELLO AMIGA
NOVADOCK_OPTION=POLL_TICKS|Refresh ticks|UINT|10,500|50</pre>
<h3 id="lifecycle">3. Update the tile safely</h3>
<p>Use the supplied client helper, honour <b>ND_EVENT_QUIT</b>, and wait between updates with <b>nd_client_delay_ticks()</b>. Exchanges have a fifteen-second deadline. ABI 1 covers basic tiles; later versions add graphs, media controls and badges. The current host supports ABI 5.</p>
<h2 id="settings">Give your plugin a settings window</h2>
<p>Right-click the dock tile and choose <b>Edit settings...</b>. NovaDockPrefs creates that window from the manifest's <b>NOVADOCK_OPTION</b> fields, so ordinary options need no separate settings application.</p>
<pre class="code">NOVADOCK_OPTION=GREETING|Greeting|TEXT||HELLO AMIGA
NOVADOCK_OPTION=POLL_TICKS|Refresh ticks|UINT|10,500|50
NOVADOCK_OPTION=SHOW_LABEL|Show label|BOOL||YES
NOVADOCK_OPTION=STYLE|Style|CHOICE|COMPACT,FULL|COMPACT
NOVADOCK_OPTION=PROGRAM|Program|FILE||SYS:Utilities/MultiView</pre>
<p>The five fields are <b>key | label | type | choices or range | default</b>. Use TEXT for text, UINT for bounded numbers, BOOL for toggles, CHOICE for cycles and FILE for a path with a file picker. A manifest supports up to 12 settings.</p>
<p>NovaDock opens <b>NovaDockPrefs PAGE=PLUGIN PLUGIN=&lt;id&gt; FOCUSED=1</b> for the selected tile. Use the actual ID passed in argv[1], including any instance suffix, when reading options. The editor stores values as <b>OPTION.&lt;ID&gt;.&lt;KEY&gt;</b> in the shared novadock.cfg; plugins should not replace that file themselves.</p>
<p>Read values at startup with <b>nd_plugin_option_strings()</b>. Reread them when an update returns <b>ND_EVENT_SETTINGS</b>; the SDK Hello example already does this:</p>
<pre class="code">if (event &amp; ND_EVENT_QUIT) break;
if (event &amp; ND_EVENT_SETTINGS)
    read_settings(argv[1], greeting, sizeof(greeting), &amp;poll);</pre>
<p><b>Use</b> applies settings for the session; <b>Save</b> also keeps them permanently. The shared editor writes the configuration and notifies the plugin. ND_EVENT_SETTINGS is a change notification, not an instruction to open your own window.</p>
<p>After changing a schema, remove its dock tiles and re-import the updated manifest. The SDK includes <b>PLUGIN_SDK_SETTINGS.md</b> with the full reader, testing steps and custom-UI guidance. <a href="<?= site_escape('https://github.com/retrocs-org/novadock/blob/'.rawurlencode(site_release_tag()).'/plugin-sdk/PLUGIN_SDK_SETTINGS.md') ?>">Read it on GitHub &rarr;</a></p>
<p>A custom window can be opened by the plugin's own actions, but the current SDK cannot replace the right-click settings entry with an external preferences app.</p>
<h3>Keep your plugin responsive</h3>
<p>Initialise with nd_client_init(), read options, send updates and handle the returned events. Handle quit first, then clicks and changed settings. Use <b>nd_client_wait(&amp;client, nd_client_delay_ticks(&amp;client, poll))</b> between updates so the process sleeps and wakes for events. Call nd_client_close() before exiting.</p>
<p>If an action opens your own Intuition or GadTools window, keep its event handling responsive while the plugin continues to process dock updates and quit requests. Long-running work belongs outside the dock update path.</p>
<h3 id="packaging">4. Package and test</h3>
<pre class="code">Hello-1.0/
  README.txt
  Plugins/
    Hello
    Hello.ndplugin</pre>
<p>Include the author, licence, version, dependencies and install instructions. In Preferences &gt; Plugins, choose Import, review the preview, then Add to dock. Test on the AmigaOS configurations you support.</p>
<p>Full reference: <b>docs/PLUGIN_SDK_QUICKSTART.md</b>, <b>docs/PLUGIN_API.md</b> and <b>docs/PLUGIN_SETTINGS.md</b> in the source download.</p>
</div>
<h2 id="testing">Before sharing</h2>
<ul><li>Import the manifest, add a tile and check the initial greeting.</li><li>Click it and check both the changed text and badge.</li><li>Right-click, choose Edit settings..., change the greeting and choose Use. Check the live tile without restarting.</li><li>Save another greeting, restart NovaDock and check that it persists.</li><li>Try each option boundary and file picker. Close the editor without applying and check that unapplied values are discarded.</li><li>Remove the tile and quit NovaDock. The plugin must stop promptly and release its resources.</li></ul>
<h2>Troubleshooting</h2>
<p><b>No settings controls?</b> Check the five manifest fields and re-import the schema after removing its tiles. Editing a manifest on disk does not refresh an imported schema.</p>
<p><b>Changes do not reach the tile?</b> Read options on startup and on ND_EVENT_SETTINGS, using the actual runtime ID. Keep manifest defaults and reader fallbacks consistent.</p>
<p><b>Program does not launch?</b> Check NOVADOCK_PROGRAM, filename case, runtime dependencies and the executable in the package. The Hello program expects NovaDock to supply its ID; launch it through the dock.</p>
<!-- /content -->
<?php site_footer(); ?>
