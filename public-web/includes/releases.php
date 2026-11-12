<?php
/** Public GitHub release catalogue, with a bounded anonymous request and disk cache. */
const ND_RELEASE_REPO = 'https://github.com/retrocs-org/novadock';
const ND_RELEASE_API = 'https://api.github.com/repos/retrocs-org/novadock/releases?per_page=20';

function nd_release_entry(string $tag, string $version): array {
    $encoded = rawurlencode($tag);
    $base = ND_RELEASE_REPO.'/releases/download/'.$encoded.'/';
    return ['version'=>$version, 'tag'=>$tag, 'status'=>'published', 'prerelease'=>false,
        'date'=>null, 'published_at'=>'', 'release_url'=>ND_RELEASE_REPO.'/releases/tag/'.$encoded,
        'binary_url'=>null, 'binary_size'=>null, 'checksums_url'=>null,
        'sdk_url'=>null, 'sdk_size'=>null, 'sdk_checksums_url'=>null,
        'source_url'=>ND_RELEASE_REPO.'/archive/refs/tags/'.$encoded.'.zip',
        'source_tar_url'=>ND_RELEASE_REPO.'/archive/refs/tags/'.$encoded.'.tar.gz'];
}

function nd_release_identity($tag, $version): bool {
    return is_string($tag) && preg_match('~^[A-Za-z0-9][A-Za-z0-9._/-]{0,127}$~D', $tag) === 1
        && is_string($version) && preg_match('~^[0-9]+(?:\.[0-9]+)*(?:[-.][A-Za-z0-9]+)*$~D', $version) === 1;
}

function nd_release_normalize(array $record): ?array {
    if (!empty($record['draft']) || !is_string($record['tag_name'] ?? null)) return null;
    $tag = $record['tag_name'];
    $assets = [];
    foreach (is_array($record['assets'] ?? null) ? $record['assets'] : [] as $asset) {
        if (is_array($asset) && is_string($asset['name'] ?? null)) $assets[$asset['name']] = $asset;
    }
    if (preg_match('~^v?([0-9]+(?:\.[0-9]+)*(?:[-.][A-Za-z0-9]+)*)$~D', $tag, $match)) {
        $version = $match[1];
    } else {
        $versions = [];
        foreach (array_keys($assets) as $name) {
            if (preg_match('~^NovaDock-([0-9]+(?:\.[0-9]+)*(?:[-.][A-Za-z0-9]+)*)\.lha$~D', $name, $match)) $versions[$match[1]] = true;
        }
        if (count($versions) !== 1) return null;
        $version = (string) array_key_first($versions);
    }
    if (!nd_release_identity($tag, $version)) return null;
    $entry = nd_release_entry($tag, $version);
    $base = ND_RELEASE_REPO.'/releases/download/'.rawurlencode($tag).'/';
    foreach (['binary_url'=>'NovaDock-'.$version.'.lha', 'checksums_url'=>'NovaDock-'.$version.'.lha.sha256',
              'sdk_url'=>'NovaDock-'.$version.'-plugin-sdk.zip', 'sdk_checksums_url'=>'NovaDock-'.$version.'-plugin-sdk.zip.sha256'] as $key=>$name) {
        if (($assets[$name]['browser_download_url'] ?? null) !== $base.$name) continue;
        $entry[$key] = $base.$name;
        $size = $assets[$name]['size'] ?? null;
        if (in_array($key, ['binary_url','sdk_url'], true) && is_int($size) && $size >= 0) $entry[$key === 'binary_url' ? 'binary_size' : 'sdk_size'] = $size;
    }
    $entry['prerelease'] = !empty($record['prerelease']);
    if (is_string($record['published_at'] ?? null) && preg_match('~^\d{4}-\d{2}-\d{2}T[0-9:]+Z$~D', $record['published_at'])) {
        $entry['published_at'] = $record['published_at'];
        $entry['date'] = substr($record['published_at'], 0, 10);
    }
    return $entry;
}

function nd_release_saved_entries($records): array {
    $result = [];
    foreach (is_array($records) ? $records : [] as $record) {
        if (!is_array($record) || ($record['status'] ?? '') !== 'published' || !nd_release_identity($record['tag'] ?? null, $record['version'] ?? null)) continue;
        $entry = nd_release_entry($record['tag'], $record['version']);
        $base = ND_RELEASE_REPO.'/releases/download/'.rawurlencode($record['tag']).'/';
        foreach (['binary_url'=>'NovaDock-'.$record['version'].'.lha', 'checksums_url'=>'NovaDock-'.$record['version'].'.lha.sha256',
                  'sdk_url'=>'NovaDock-'.$record['version'].'-plugin-sdk.zip', 'sdk_checksums_url'=>'NovaDock-'.$record['version'].'-plugin-sdk.zip.sha256'] as $key=>$name) {
            if (($record[$key] ?? null) === $base.$name) $entry[$key] = $base.$name;
        }
        foreach (['binary_size','sdk_size'] as $key) if (is_int($record[$key] ?? null) && $record[$key] >= 0) $entry[$key] = $record[$key];
        $entry['prerelease'] = !empty($record['prerelease']);
        if (is_string($record['date'] ?? null) && preg_match('~^\d{4}-\d{2}-\d{2}$~D', $record['date'])) $entry['date'] = $record['date'];
        $entry['published_at'] = is_string($record['published_at'] ?? null) ? $record['published_at'] : ($entry['date'] ?? '');
        $result[] = $entry;
    }
    usort($result, static function ($a, $b) { return strcmp($b['published_at'], $a['published_at']); });
    return $result;
}

function nd_release_fetch(string $etag): ?array {
    $headers = ['Accept: application/vnd.github+json', 'User-Agent: NovaDock-website', 'X-GitHub-Api-Version: 2026-03-10'];
    if ($etag !== '' && strpos($etag, "\r") === false && strpos($etag, "\n") === false) $headers[] = 'If-None-Match: '.$etag;
    $responseHeaders = []; $code = 0; $body = false;
    if (function_exists('curl_init')) {
        $curl = curl_init(ND_RELEASE_API);
        curl_setopt_array($curl, [CURLOPT_RETURNTRANSFER=>true, CURLOPT_FOLLOWLOCATION=>false,
            CURLOPT_CONNECTTIMEOUT=>2, CURLOPT_TIMEOUT=>4, CURLOPT_HTTPHEADER=>$headers,
            CURLOPT_SSL_VERIFYPEER=>true, CURLOPT_SSL_VERIFYHOST=>2,
            CURLOPT_WRITEFUNCTION=>static function ($handle, string $chunk) use (&$body) {
                if ($body === false) $body = '';
                if (strlen($body)+strlen($chunk)>2097152) return 0;
                $body .= $chunk; return strlen($chunk);
            }, CURLOPT_HEADERFUNCTION=>static function ($handle, string $line) use (&$responseHeaders) { $responseHeaders[] = $line; return strlen($line); }]);
        $ok = curl_exec($curl); $code = (int) curl_getinfo($curl, CURLINFO_RESPONSE_CODE); unset($curl);
        if ($ok === false) return null;
    } elseif (in_array(strtolower(trim((string) ini_get('allow_url_fopen'))), ['1','on','yes','true'], true)) {
        $context = stream_context_create(['http'=>['timeout'=>4, 'follow_location'=>0, 'ignore_errors'=>true, 'header'=>implode("\r\n", $headers)],
                                         'ssl'=>['verify_peer'=>true, 'verify_peer_name'=>true]]);
        $body = @file_get_contents(ND_RELEASE_API, false, $context, 0, 2097153);
        $responseHeaders = function_exists('http_get_last_response_headers') ? (http_get_last_response_headers() ?? []) : ($http_response_header ?? []);
        if ($responseHeaders && preg_match('~^HTTP/\S+ (\d{3})~', $responseHeaders[0], $match)) $code = (int) $match[1];
    }
    if ($body !== false && strlen($body)>2097152) return null;
    $newEtag = '';
    foreach ($responseHeaders as $header) if (stripos($header, 'ETag:') === 0) $newEtag = trim(substr($header, 5));
    if ($code === 304) return ['status'=>304, 'etag'=>$etag];
    if ($code !== 200 || $body === false) return null;
    $records = json_decode($body, true);
    return is_array($records) && nd_release_is_list($records) ? ['status'=>200, 'etag'=>$newEtag, 'records'=>$records] : null;
}

function nd_release_is_list(array $items): bool { return $items === [] || array_keys($items) === range(0, count($items)-1); }

function nd_release_catalog(?callable $fetch = null, ?string $cacheDir = null, ?int $now = null, ?string $fallbackFile = null): array {
    $now = $now ?? time();
    $fallbackFile = $fallbackFile ?? __DIR__.'/../releases.json';
    $fallback = nd_release_saved_entries(json_decode((string) @file_get_contents($fallbackFile), true));
    $result = ['releases'=>$fallback, 'source'=>'bundled', 'stale'=>false];
    // CLI rendering and ordinary tests stay offline. Tests inject their transport.
    if ($fetch === null && PHP_SAPI === 'cli') return $result;
    $cacheDir = $cacheDir ?? sys_get_temp_dir().'/novadock-releases-'.substr(hash('sha256', __DIR__), 0, 20);
    if (is_link($cacheDir) || (!is_dir($cacheDir) && !@mkdir($cacheDir, 0700, true))) return $result;
    $path = $cacheDir.'/catalog.json';
    if (is_link($path) || is_link($cacheDir.'/refresh.lock')) return $result;
    $cache = json_decode((string) @file_get_contents($path), true);
    $saved = is_array($cache) ? nd_release_saved_entries($cache['releases'] ?? null) : [];
    $hasCache = is_array($cache) && is_array($cache['releases'] ?? null) && count($saved) === count($cache['releases']);
    if ($hasCache) $result = ['releases'=>$saved, 'source'=>'cache', 'stale'=>empty($cache['ok'])];
    if (is_array($cache) && ($cache['retry_at'] ?? 0) > $now) return $result;
    $lock = @fopen($cacheDir.'/refresh.lock', 'c');
    if (!$lock || !flock($lock, LOCK_EX | LOCK_NB)) { if ($lock) fclose($lock); return $result; }
    try {
        // Recheck after acquiring the lock; another request may have refreshed it.
        $current = json_decode((string) @file_get_contents($path), true);
        if (is_array($current) && ($current['retry_at'] ?? 0) > $now) {
            $entries = nd_release_saved_entries($current['releases'] ?? []);
            $valid = is_array($current['releases'] ?? null) && count($entries) === count($current['releases']);
            return ['releases'=>$valid ? $entries : $fallback, 'source'=>$valid ? 'cache' : 'bundled', 'stale'=>empty($current['ok'])];
        }
        $fetch = $fetch ?? 'nd_release_fetch';
        try { $response = $fetch(is_string($cache['etag'] ?? null) ? $cache['etag'] : ''); } catch (Throwable $error) { $response = null; }
        $entries = null;
        if (($response['status'] ?? null) === 304 && $hasCache) $entries = $saved;
        elseif (($response['status'] ?? null) === 200 && is_array($response['records'] ?? null)) {
            $entries = [];
            foreach ($response['records'] as $record) if (is_array($record) && ($entry = nd_release_normalize($record))) $entries[] = $entry;
            usort($entries, static function ($a, $b) { return strcmp($b['published_at'], $a['published_at']); });
        }
        $success = $entries !== null;
        $payload = ['ok'=>$success, 'retry_at'=>$now+($success ? 900 : 60), 'etag'=>$success ? (string) ($response['etag'] ?? '') : (string) ($cache['etag'] ?? ''),
                    'releases'=>$success ? $entries : ($hasCache ? $saved : $fallback)];
        $temporary = @tempnam($cacheDir, 'catalog-');
        if ($temporary !== false) {
            if (@file_put_contents($temporary, json_encode($payload, JSON_UNESCAPED_SLASHES), LOCK_EX) !== false) @rename($temporary, $path);
            if (is_file($temporary)) @unlink($temporary);
        }
        return ['releases'=>$payload['releases'], 'source'=>$success ? 'github' : ($hasCache ? 'cache' : 'bundled'), 'stale'=>!$success];
    } finally { flock($lock, LOCK_UN); fclose($lock); }
}

function nd_release_size($bytes): string {
    if (!is_int($bytes) || $bytes < 0) return '';
    return $bytes >= 1048576 ? number_format($bytes/1048576, 1).' MB' : number_format($bytes/1024, 0).' KB';
}
