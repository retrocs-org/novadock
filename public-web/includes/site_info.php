<?php
/** Published packages stay separate from the working-tree application version. */
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
function site_is_development(): bool {
    $release = site_release();
    return $release === null || version_compare(NOVADOCK_VERSION, $release['version'], '>');
}
function site_version_is_upcoming(string $version): bool {
    $release = site_release();
    return $release === null || version_compare($version, $release['version'], '>');
}
function site_plugin_is_upcoming(array $plugin): bool {
    return isset($plugin['added']) && site_version_is_upcoming($plugin['added']);
}
