<?php
/** Download links follow published releases; product copy is release-independent. */
require_once __DIR__.'/releases.php';
function site_release_catalog(): array {
    static $catalog = null;
    if ($catalog === null) $catalog = nd_release_catalog();
    return $catalog;
}
function site_release(): ?array {
    $catalog = site_release_catalog();
    return $catalog['releases'][0] ?? null;
}
function site_release_tag(): string {
    $release = site_release();
    return $release === null ? 'main' : $release['tag'];
}
