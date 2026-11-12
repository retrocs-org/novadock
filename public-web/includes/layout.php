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
</tr></table>
<br>
<nav class="site-nav" aria-label="Main navigation"><div class="nav-links"><a href="index.php"<?php if ($current === 'Home'): ?> class="active" aria-current="page"<?php endif; ?>>Home</a> <a href="features.php"<?php if ($current === 'Features'): ?> class="active" aria-current="page"<?php endif; ?>>Features</a> <a href="marketplace.php"<?php if ($current === 'Marketplace'): ?> class="active" aria-current="page"<?php endif; ?>>Marketplace</a> <a href="developers.php"<?php if (in_array($current, ['Developers', 'Plugin SDK', 'Theme development'], true)): ?> class="active" aria-current="page"<?php endif; ?>>Developers</a> <a href="support.php"<?php if ($current === 'Support'): ?> class="active" aria-current="page"<?php endif; ?>>Support</a></div><details class="mobile-menu"><summary>Menu</summary><div class="mobile-links"><a href="index.php"<?php if ($current === 'Home'): ?> class="active" aria-current="page"<?php endif; ?>>Home</a> <a href="features.php"<?php if ($current === 'Features'): ?> class="active" aria-current="page"<?php endif; ?>>Features</a> <a href="marketplace.php"<?php if ($current === 'Marketplace'): ?> class="active" aria-current="page"<?php endif; ?>>Marketplace</a> <a href="developers.php"<?php if (in_array($current, ['Developers', 'Plugin SDK', 'Theme development'], true)): ?> class="active" aria-current="page"<?php endif; ?>>Developers</a> <a href="support.php"<?php if ($current === 'Support'): ?> class="active" aria-current="page"<?php endif; ?>>Support</a></div></details><a class="nav-cta" href="get.php"<?php if ($current === 'Get NovaDock'): ?> aria-current="page"<?php endif; ?>>Get NovaDock <span aria-hidden="true">&#8599;</span></a></nav>
<br>
<font class="site-content" face="Helvetica,Arial,sans-serif">
<?php
}
function site_footer(): void {
?>
</font>
<br>
<table class="site-footer" width="100%" border="0" cellpadding="10" cellspacing="0" bgcolor="#181825"><tr><td align="center"><font face="Helvetica,Arial,sans-serif" size="-1" color="#A6ADC8">NovaDock 0.1 &middot; Classic AmigaOS 3.x &middot; <a href="https://github.com/retrocs-org/novadock">Source on GitHub</a> &middot; <a href="marketplace.php">Marketplace</a> &middot; <a href="features.php#screenshots">Screenshots</a> &middot; <a href="theme-development.php">Theme guide</a> &middot; <a href="developers.php">Developers</a> &middot; <a href="developers.php#source">Build from source</a> &middot; <a href="support.php#faq">Support &amp; FAQ</a></font></td></tr></table>
</td></tr></table>
</center>
</body>
</html>
<?php
}
