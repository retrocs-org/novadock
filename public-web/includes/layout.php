<?php
declare(strict_types=1);
const NOVADOCK_VERSION = '0.1';
function site_escape(string $text): string {
    return htmlspecialchars($text, ENT_QUOTES | ENT_SUBSTITUTE, 'ISO-8859-1');
}
function site_release_tag(): string {
    $entries = json_decode((string) @file_get_contents(__DIR__.'/../releases.json'), true);
    foreach (is_array($entries) ? $entries : [] as $entry) {
        if (($entry['version'] ?? '') === NOVADOCK_VERSION && ($entry['status'] ?? '') === 'published' && is_string($entry['tag'] ?? null)) return $entry['tag'];
    }
    return 'v'.NOVADOCK_VERSION;
}
function site_header(string $title, string $current, string $description): void {
    header('Content-Type: text/html; charset=ISO-8859-1');
?>
<!DOCTYPE html>
<html lang="en">
<head>
<title>NovaDock - <?= site_escape($title) ?></title>
<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
<meta name="description" content="<?= site_escape($description) ?>">
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="stylesheet" href="site.css" type="text/css">
</head>
<body bgcolor="#1E1E2E" text="#CDD6F4" link="#89B4FA" vlink="#CBA6F7" alink="#F9E2AF">
<center>
<table class="site-shell" width="600" border="0" cellpadding="0" cellspacing="0"><tr><td>
<table class="site-header" width="100%" border="0" cellpadding="0" cellspacing="0"><tr>
<td width="120"><a href="index.php"><img src="images/logo.gif" width="108" height="44" border="0" alt="NovaDock"></a></td>
<td><font face="Helvetica,Arial,sans-serif"><font size="+3"><b>NovaDock</b></font><br><font color="#A6ADC8">A dock for Classic AmigaOS Workbench</font></font></td>
<td align="right"><table border="0" cellpadding="4" cellspacing="0"><tr><td bgcolor="#A6E3A1"><font face="Helvetica,Arial,sans-serif" size="-1" color="#1E1E2E"><b>VERSION 0.1</b></font></td></tr></table></td>
</tr></table>
<br>
<table class="site-nav" width="100%" border="0" cellpadding="6" cellspacing="2"><tr>
<?php if ($current === 'Home'): ?><td align="center" bgcolor="#89B4FA"><font face="Helvetica,Arial,sans-serif" size="-1" color="#1E1E2E"><b>Home</b></font></td><?php else: ?><td align="center" bgcolor="#313244"><font face="Helvetica,Arial,sans-serif" size="-1"><a href="index.php">Home</a></font></td><?php endif; ?>
<?php if ($current === 'Features'): ?><td align="center" bgcolor="#89B4FA"><font face="Helvetica,Arial,sans-serif" size="-1" color="#1E1E2E"><b>Features</b></font></td><?php else: ?><td align="center" bgcolor="#313244"><font face="Helvetica,Arial,sans-serif" size="-1"><a href="features.php">Features</a></font></td><?php endif; ?>
<?php if ($current === 'Screenshots'): ?><td align="center" bgcolor="#89B4FA"><font face="Helvetica,Arial,sans-serif" size="-1" color="#1E1E2E"><b>Screenshots</b></font></td><?php else: ?><td align="center" bgcolor="#313244"><font face="Helvetica,Arial,sans-serif" size="-1"><a href="screenshots.php">Screenshots</a></font></td><?php endif; ?>
<?php if ($current === 'Marketplace'): ?><td align="center" bgcolor="#89B4FA"><font face="Helvetica,Arial,sans-serif" size="-1" color="#1E1E2E"><b>Marketplace</b></font></td><?php else: ?><td align="center" bgcolor="#313244"><font face="Helvetica,Arial,sans-serif" size="-1"><a href="marketplace.php">Marketplace</a></font></td><?php endif; ?>
<?php if (in_array($current, ['Developers', 'Plugin SDK', 'Theme development'], true)): ?><td align="center" bgcolor="#89B4FA"><font face="Helvetica,Arial,sans-serif" size="-1" color="#1E1E2E"><b>Developers</b></font></td><?php else: ?><td align="center" bgcolor="#313244"><font face="Helvetica,Arial,sans-serif" size="-1"><a href="developers.php">Developers</a></font></td><?php endif; ?>
<?php if ($current === 'Source'): ?><td align="center" bgcolor="#89B4FA"><font face="Helvetica,Arial,sans-serif" size="-1" color="#1E1E2E"><b>Source</b></font></td><?php else: ?><td align="center" bgcolor="#313244"><font face="Helvetica,Arial,sans-serif" size="-1"><a href="source.php">Source</a></font></td><?php endif; ?>
<?php if ($current === 'Download'): ?><td align="center" bgcolor="#89B4FA"><font face="Helvetica,Arial,sans-serif" size="-1" color="#1E1E2E"><b>Download</b></font></td><?php else: ?><td align="center" bgcolor="#313244"><font face="Helvetica,Arial,sans-serif" size="-1"><a href="download.php">Download</a></font></td><?php endif; ?>
<?php if ($current === 'Support'): ?><td align="center" bgcolor="#89B4FA"><font face="Helvetica,Arial,sans-serif" size="-1" color="#1E1E2E"><b>Support</b></font></td><?php else: ?><td align="center" bgcolor="#313244"><font face="Helvetica,Arial,sans-serif" size="-1"><a href="support.php">Support</a></font></td><?php endif; ?>
</tr></table>
<br>
<font class="site-content" face="Helvetica,Arial,sans-serif">
<?php
}
function site_footer(): void {
?>
</font>
<br>
<table class="site-footer" width="100%" border="0" cellpadding="10" cellspacing="0" bgcolor="#181825"><tr><td align="center"><font face="Helvetica,Arial,sans-serif" size="-1" color="#A6ADC8">NovaDock 0.1 &middot; Classic AmigaOS 3.x &middot; <a href="https://github.com/retrocs-org/novadock">Source on GitHub</a> &middot; <a href="marketplace.php">Marketplace</a> &middot; <a href="screenshots.php">Screenshots</a> &middot; <a href="theme-development.php">Theme guide</a> &middot; <a href="developers.php">Developers</a> &middot; <a href="source.php">Source</a> &middot; <a href="support.php#faq">Support &amp; FAQ</a></font></td></tr></table>
</td></tr></table>
</center>
</body>
</html>
<?php
}
