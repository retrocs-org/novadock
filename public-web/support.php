<?php
require_once __DIR__ . '/includes/layout.php';
site_header('Support', 'Support', 'Frequently asked questions and help with NovaDock.');
?>
<!-- content -->
<p class="eyebrow">Help &amp; answers</p>
<h1>A little help for your dock.</h1>
<p class="lead">Find setup answers, recover your settings or tell us about a problem.</p>
<?php require __DIR__ . '/includes/faq.php'; ?>
<div class="dev-section"><h2>Still need help?</h2><p><a href="https://github.com/retrocs-org/novadock">Visit the GitHub repository</a> to check or report issues, or email <a href="mailto:support@retrocs.org">support@retrocs.org</a>.</p><p>Include your NovaDock version, AmigaOS version, relevant plugin and the steps that cause the problem. Diagnostics can help identify the configuration in use. Remove private information before sharing it.</p></div>
<p>Making an extension? Start with <a href="developers.php">Developers</a>. Compiling the app? See <a href="source.php">Source</a>.</p>
<!-- /content -->
<?php site_footer(); ?>
