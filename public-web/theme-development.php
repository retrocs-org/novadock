<?php
require_once __DIR__ . '/includes/layout.php';
site_header('Theme development', 'Theme development', 'Create, preview, package and maintain NovaDock themes.');
?>
<!-- content -->
<p class="eyebrow"><a href="developers.php">Developers</a> / Theme development</p>
<h1>Make the dock your own.</h1>
<p class="lead">Create a theme with a text editor. No compiler is needed.</p>
<p class="market-sections"><a href="#format">Theme format</a> <a href="#package">Packaging</a> <a href="#import">Import and test</a></p>
<p>A NovaDock theme is one plain text file named <b>theme.cfg</b>. It controls
colours, tile shape, placement, font, labels and spacer defaults. Graphic dock
backdrops are not used in NovaDock 0.1.</p>
<ol>
<li>Copy one bundled theme directory to a working drawer.</li>
<li>Edit <b>theme.cfg</b> with a plain text editor.</li>
<li>Give it a new display name and adjust only the values you need.</li>
<li>Keep it in its own newly named theme drawer, import it in Appearance and
choose <b>Test</b>.</li>
</ol>
<table width="100%" border="0" cellpadding="4" cellspacing="0" bgcolor="#11111B"><tr><td><pre><font size="-1" color="#F9E2AF">NAME=Blue Copper
SHAPE=CHAMFER
SIDE=BOTTOM
POSITION=CENTER
FONT=TOPAZ
LABELS=ALWAYS
SPACER_WIDTH=52
SPACER_LINE=YES
SPACER_LINE_COLOR=ACCENT
SPACER_LINE_WIDTH=2
BG=25,38,76
TILE=32,50,92
HOVER=48,72,120
TEXT=235,240,255
SUBTLE=155,175,205
ACCENT=86,155,235
BAR=30,42,65
ACCENT2=86,210,170</font></pre></td></tr></table>
<h2 id="format">Supported values</h2>
<ul>
<li><b>SHAPE:</b> ROUNDED, SQUARE, CHAMFER or MINIMAL.</li>
<li><b>SIDE:</b> BOTTOM, TOP, LEFT or RIGHT.</li>
<li><b>POSITION:</b> START, CENTER or END.</li>
<li><b>FONT:</b> WORKBENCH, TOPAZ or BOLD.</li>
<li><b>LABELS:</b> ALWAYS, HOVER or HIDDEN.</li>
<li><b>BEVEL:</b> YES for a raised frame around the dock, or NO.</li>
<li><b>Colours:</b> decimal red, green and blue values from 0 through 255.</li>
<li><b>SPACER_WIDTH:</b> 20 through 140 pixels.</li>
<li><b>SPACER_LINE:</b> YES or NO; colour is SUBTLE, ACCENT, SECONDARY or TEXT;
width is 1 through 8.</li>
</ul>
<table width="100%" border="0" cellpadding="0" cellspacing="0" bgcolor="#313244"><tr><td width="4" bgcolor="#89B4FA"><img src="images/px.gif" width="4" height="1" alt=""></td><td><table width="100%" border="0" cellpadding="10" cellspacing="0"><tr><td><font face="Helvetica,Arial,sans-serif"><font color="#F9E2AF"><b>Expandable theme catalogue</b></font>
<p>Appearance lists the bundled presets followed by every valid theme drawer
found in <b>PROGDIR:Themes</b>. Third-party themes do not replace built-in
slots, and the catalogue grows dynamically with the installed themes.</p>
<p>The drawer name is the stable theme ID. Use only letters, digits, underscore
and hyphen, with a maximum of 31 characters. The <b>NAME</b> entry is the
friendly label displayed in Preferences.</p></font></td></tr></table></td></tr></table>
<p>BG is the dock background; TILE and HOVER distinguish items; TEXT and SUBTLE control labels. ACCENT and ACCENT2 colour meters and graphs, while BAR is the unfilled meter colour. Check that both normal and hovered text remain readable.</p>
<p>Find starting points in the <a href="https://github.com/retrocs-org/novadock/tree/v0.1/Themes">bundled theme source</a> when the tagged release is available. Keep your own unique drawer ID so an update does not replace another author's theme.</p>
<h2 id="package">Package a theme</h2>
<p>A portable ZIP or LHA package should contain no executable and can use:</p>
<table width="100%" border="0" cellpadding="4" cellspacing="0" bgcolor="#11111B"><tr><td><pre><font size="-1" color="#F9E2AF">BlueCopper-1.0/
  README.txt
  Themes/
    BlueCopper/
      theme.cfg</font></pre></td></tr></table>
<p>Include the theme name, author, license, NovaDock version, theme ID,
installation and uninstall instructions. Keep filenames and directory case
exactly as shown. Test the package from a clean copy before publishing it.</p>
<h2 id="import">Import and test</h2>
<ol>
<li>Extract the package anywhere.</li>
<li>Open NovaDock Preferences and choose <b>Appearance</b>.</li>
<li>Choose <b>Import...</b> and select the pack's <b>theme.cfg</b>.</li>
<li>NovaDock validates and copies it to
<b>PROGDIR:Themes/&lt;theme-id&gt;/theme.cfg</b>, then selects it.</li>
<li>Choose <b>Test</b>. Use <b>Save</b> only when the result is correct.</li>
</ol>
<p>Selecting a theme changes the Preferences sample. <b>Test</b> temporarily applies the complete preset while Preferences stays open; closing Preferences after a Test restores the original appearance. <b>Use</b> applies it for this session and exits; <b>Save</b> keeps it permanently and exits.</p>
<p>Test horizontal and vertical placement, long labels, hover colours, meters and spacers. Refresh mode is a separate resource preference and is not changed by a theme.</p>
<p>Importing the same theme ID updates that theme safely: the new file is copied
and validated before the installed copy is replaced. For manual installation,
copy the complete theme drawer into <b>NovaDock:Themes/</b> and
reopen Appearance.</p>
<p>To uninstall an imported theme, select it in Appearance and choose
<b>Import...</b>; NovaDock offers to remove it. A theme that is in use or
saved cannot be removed, so <b>Use</b> or <b>Save</b> another theme first.
Bundled themes cannot be removed.</p>
<p>Want to create an executable extension instead? See the
<a href="plugin-sdk.php">Plugin SDK guide</a>.</p>
<!-- /content -->
<?php site_footer(); ?>
