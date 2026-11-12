<?php
require __DIR__.'/../public-web/includes/releases.php';
function check($condition, string $message): void { if (!$condition) throw new RuntimeException($message); }
function record(string $version, string $date, bool $draft = false): array {
    $base = ND_RELEASE_REPO.'/releases/download/'.$version.'/';
    $names = ['NovaDock-'.$version.'.lha','NovaDock-'.$version.'.lha.sha256','NovaDock-'.$version.'-plugin-sdk.zip','NovaDock-'.$version.'-plugin-sdk.zip.sha256'];
    return ['tag_name'=>$version, 'published_at'=>$date, 'draft'=>$draft, 'prerelease'=>false,
        'assets'=>array_map(static function ($name) use ($base) { return ['name'=>$name,'browser_download_url'=>$base.$name,'size'=>700000]; }, $names)];
}
$root = sys_get_temp_dir().'/novadock-web-test-'.bin2hex(random_bytes(8));
mkdir($root, 0700);
$fallback = $root.'/fallback.json';
file_put_contents($fallback, json_encode([nd_release_normalize(record('0.1','2026-11-11T12:00:00Z'))]));
try {
    $calls = 0; $now = 2000000000;
    $fetch = static function ($etag) use (&$calls) {
        ++$calls;
        return ['status'=>200,'etag'=>'"first"','records'=>[
            record('0.1','2026-11-11T12:00:00Z'), record('9.0','2027-01-01T12:00:00Z',true), record('0.2','2026-12-01T12:00:00Z')]];
    };
    $a = nd_release_catalog($fetch, $root.'/cache', $now, $fallback);
    check($a['releases'][0]['version']==='0.2' && count($a['releases'])===2, 'Newest published release must lead; drafts excluded');
    check($a['releases'][0]['sdk_checksums_url'] === ND_RELEASE_REPO.'/releases/download/0.2/NovaDock-0.2-plugin-sdk.zip.sha256', 'SDK checksum is an actual asset');
    $b = nd_release_catalog($fetch, $root.'/cache', $now+800, $fallback);
    check($calls===1 && $b['releases'][0]['version']==='0.2', 'Fresh cache must avoid API calls');
    $notModified = static function ($etag) use (&$calls) { ++$calls; check($etag==='"first"', 'Conditional requests use ETag'); return ['status'=>304,'etag'=>$etag]; };
    $c = nd_release_catalog($notModified, $root.'/cache', $now+901, $fallback);
    check($calls===2 && $c['releases'][0]['version']==='0.2', '304 must preserve releases');
    $failure = static function ($etag) use (&$calls) { ++$calls; return null; };
    $d = nd_release_catalog($failure, $root.'/cache', $now+1802, $fallback);
    check($d['stale'] && $d['releases'][0]['version']==='0.2', 'API failures retain last good data');
    nd_release_catalog($failure, $root.'/cache', $now+1810, $fallback);
    check($calls===3, 'Failures must be throttled');
    $offline = nd_release_catalog($failure, $root.'/offline', $now, $fallback);
    check($offline['releases'][0]['version']==='0.1', 'Bundled catalogue is the offline fallback');
    $empty = static function ($etag) { return ['status'=>200,'etag'=>'','records'=>[]]; };
    check(nd_release_catalog($empty, $root.'/empty', $now, $fallback)['releases']===[], 'Valid empty response must stay empty');
    check(nd_release_catalog($empty, $root.'/empty', $now+1, $fallback)['releases']===[], 'Fresh empty cache must not resurrect removed releases');
    $blocked = $root.'/locked'; mkdir($blocked,0700); $lock = fopen($blocked.'/refresh.lock','c'); flock($lock,LOCK_EX);
    $never = static function ($etag) { throw new RuntimeException('Concurrent request fetched'); };
    check(nd_release_catalog($never, $blocked, $now, $fallback)['releases'][0]['version']==='0.1', 'Lock contention must use fallback without blocking');
    flock($lock,LOCK_UN);fclose($lock);
    $malicious = record('0.3','2027-01-01T12:00:00Z');
    $malicious['assets'][0]['browser_download_url']='https://other.example/payload';
    check(nd_release_normalize($malicious)['binary_url']===null, 'Foreign download URL must be rejected');
    $malicious['tag_name']='\"><script>alert(1)</script>';
    check(nd_release_normalize($malicious)===null, 'Unsafe tags must be rejected');
    $missing = record('0.4','2027-02-01T12:00:00Z');$missing['assets']=[];
    check(nd_release_normalize($missing)['binary_url']===null && nd_release_normalize($missing)['sdk_url']===null, 'Missing assets must not become guessed downloads');
    check(nd_release_size(1048576)==='1.0 MB', 'Readable download size');
    echo "Automatic GitHub releases: latest selection, cache, ETag, offline fallback, throttling, locks and safe URLs passed.\n";
} finally {
    $iterator = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($root, FilesystemIterator::SKIP_DOTS), RecursiveIteratorIterator::CHILD_FIRST);
    foreach ($iterator as $file) { if ($file->isDir()) rmdir($file->getPathname()); else unlink($file->getPathname()); }
    rmdir($root);
}
