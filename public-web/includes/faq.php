<details class="home-faq" id="faq">
<summary class="faq-heading">Frequently asked questions</summary>
<div class="faq-questions">
<details><summary>Does it run on AmigaOS 3.1?</summary>
<p>NovaDock targets AmigaOS 3.0 or later and a 68000 or better.</p>
</details>
<details><summary>Can I download a ready-made program?</summary>
<p>The LHA package contains the programs and a graphical installer. Check <a href="get.php">Get NovaDock</a> for release availability, or choose the source package to build it yourself.</p>
</details>
<details><summary>Where do I change settings?</summary>
<p>Open NovaDockPrefs from SYS:Prefs/ for dock settings. Right-click a plugin and choose <b>Edit settings...</b> for its options.</p>
</details>
<details><summary>Are plugin settings in separate files?</summary>
<p>No. The dock and all plugins share one novadock.cfg format. AmigaOS keeps a session copy in ENV: and a saved copy in ENVARC:.</p>
</details>
<details><summary>What happens if novadock.cfg is damaged?</summary>
<p>NovaDock tries valid session and saved backups before using the packaged defaults. Diagnostics shows which settings file was loaded.</p>
</details>
<details><summary>What does the Refresh setting do?</summary>
<p>Low memory is the compatible default and redraws exposed dock areas without keeping a full backing bitmap. Smooth keeps a smart-refresh backing bitmap for cleaner exposes, but uses more display memory. Select it in General, then choose Use or Save.</p>
</details>
<details><summary>How do I make NovaDock start with Workbench?</summary>
<p>After installation, use SetupStartup for classic WBStartup, or configure the WBStartup Preferences system on a compatible Workbench version. Avoid enabling both methods.</p>
</details>
<details><summary>Where can developers begin?</summary>
<p>See the <a href="developers.php">plugin SDK notes</a> and sample source in the downloadable ZIP.</p>
</details>
<details><summary>Why can Network not query or control my TCP/IP stack?</summary>
<p>Roadshow status and Online/Offline commands require a compatible socket library. Details explains unsupported queries when another TCP/IP stack is active. Use that stack's own connection controls. Packet meters require a compatible SANA-II device.</p>
</details>
<details><summary>What happens if I reconnect Network immediately?</summary>
<p>Online waits for an in-progress Offline operation to finish before reconnecting the selected interface. Roadshow stays loaded. The dock remains responsive during DHCP; a command that exceeds its wait allowance gets an explanation, and no competing command is started.</p>
</details>
<details><summary>Do graphs represent fixed seconds?</summary>
<p>No. Columns are received samples. Adaptive polling changes the time between samples.</p>
</details>
<details><summary>How can I recover settings?</summary>
<p>Open Diagnostics, choose Recover settings, and select a validated ENV or ENVARC backup. Confirm loading it into Preferences, then use Use or Save to apply it. The low-resource preset is also staged until applied.</p>
</details>
<details><summary>Which plugins are included?</summary>
<p>The latest package includes the plugins listed in the <a href="marketplace.php#plugins">marketplace</a>, from Home Assistant controls and music collections to BENCH, QR, COLOUR, MOON, AGENDA and PICTURES. Add them through Preferences &gt; Organize &gt; Add Item &gt; Plugins. Some integrations need their corresponding application or hardware.</p>
</details>
<details><summary>Where do I configure Home Assistant?</summary>
<p>Install AmiHASS separately and save the connection in AmiHASSPrefs. HASS settings choose a room/type, device, action, refresh and display for each tile. Read only disables controls. The token stays in AmiHASS.</p>
</details>
<details><summary>How do notifications work?</summary>
<p>Install <a href="https://aminotis.retrocs.org">AmiNotis</a> separately, then add the AMINOTIS tile for an unread badge and inbox panel. To send the dock's own notices to the same inbox, turn on Send alerts to AmiNotis in General Preferences.</p>
</details>
<details><summary>Can NovaDock check for updates?</summary>
<p>The Update preferences page has separate startup and running checks, both off by default. Running checks use a 2, 4 or 6 hour interval. Check now shows checking, the last check and whether a newer release is available.</p>
</details>
<details><summary>Where is a plugin's artwork?</summary>
<p>Artwork can travel beside the plugin in Program.assets/Artwork.ndart. Keep the executable, manifest and matching assets together, then reload or restart NovaDock. Diagnostics explains missing or damaged artwork. HASS uses confirmed device states for its on/off icons.</p>
</details>
</div>
</details>
