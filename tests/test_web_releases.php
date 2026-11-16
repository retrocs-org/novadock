<?php
require __DIR__.'/../public-web/includes/releases.php';
function check($condition, string $message): void { if (!$condition) throw new RuntimeException($message); }
function record(string $version, string $date, bool $draft = false): array {
    $base = ND_RELEASE_REPO.'/releases/download/'.$version.'/';
    $names = ['NovaDock-'.$version.'.lha','NovaDock-'.$version.'.lha.sha256','NovaDock-'.$version.'-plugin-sdk.zip','NovaDock-'.$version.'-plugin-sdk.zip.sha256'];
    return ['tag_name'=>$version, 'published_at'=>$date, 'draft'=>$draft, 'prerelease'=>false,
        'assets'=>array_map(static function ($name) use ($base) { return ['name'=>$name,'browser_download_url'=>$base.$name,'size'=>700000,'download_count'=>substr($name, -4)==='.lha' ? 1234 : (substr($name, -4)==='.zip' ? 0 : 999999)]; }, $names)];
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
    check($a['releases'][0]['binary_download_count']===1234 && $a['releases'][0]['sdk_download_count']===0, 'Use each package count, including zero; ignore checksum counts');
    $b = nd_release_catalog($fetch, $root.'/cache', $now+800, $fallback);
    check($calls===1 && $b['releases'][0]['version']==='0.2', 'Fresh cache must avoid API calls');
    check($b['releases'][0]['binary_download_count']===1234 && $b['releases'][0]['sdk_download_count']===0, 'Fresh cache must preserve package counts');
    $notModified = static function ($etag) use (&$calls) { ++$calls; check($etag==='"first"', 'Conditional requests use ETag'); return ['status'=>304,'etag'=>$etag]; };
    $c = nd_release_catalog($notModified, $root.'/cache', $now+901, $fallback);
    check($calls===2 && $c['releases'][0]['version']==='0.2', '304 must preserve releases');
    check($c['releases'][0]['binary_download_count']===1234, '304 must preserve download counts');
    $failure = static function ($etag) use (&$calls) { ++$calls; return null; };
    $d = nd_release_catalog($failure, $root.'/cache', $now+1802, $fallback);
    check($d['stale'] && $d['releases'][0]['version']==='0.2', 'API failures retain last good data');
    check($d['releases'][0]['binary_download_count']===1234, 'API failure must retain last known count');
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
    $oldCache = nd_release_normalize(record('0.5','2027-02-01T12:00:00Z'));
    unset($oldCache['binary_download_count'], $oldCache['sdk_download_count']);
    $restored = nd_release_saved_entries([$oldCache])[0];
    check($restored['binary_download_count']===null && $restored['sdk_download_count']===null, 'Older cache must not invent zero downloads');
    foreach ([-1, '12', 2.5, true, null, '<script>'] as $invalid) {
        $invalidRecord = record('0.5','2027-02-01T12:00:00Z');
        $invalidRecord['assets'][0]['download_count']=$invalid;
        check(nd_release_normalize($invalidRecord)['binary_download_count']===null, 'Invalid API count must be omitted');
        $saved = $oldCache; $saved['binary_download_count']=$invalid;
        check(nd_release_saved_entries([$saved])[0]['binary_download_count']===null, 'Invalid saved count must be omitted');
        check(nd_release_downloads($invalid)==='', 'Invalid count must never be rendered');
    }
    $update = static function ($etag) {
        $record = record('0.2','2026-12-01T12:00:00Z');
        $record['assets'][0]['download_count']=2345;
        return ['status'=>200,'etag'=>'"second"','records'=>[$record]];
    };
    $updated = nd_release_catalog($update,$root.'/cache',$now+2703,$fallback);
    check($updated['releases'][0]['binary_download_count']===2345, 'Next successful refresh updates counts');
    check(nd_release_downloads(0)==='0 downloads on GitHub' && nd_release_downloads(1)==='1 download on GitHub'
        && nd_release_downloads(1234)==='1,234 downloads on GitHub', 'Readable package counts with singular and zero');
    check(nd_release_size(1048576)==='1.0 MB', 'Readable download size');
    echo "Automatic GitHub releases: latest selection, cache, download counts, ETag, offline fallback, throttling, locks and safe URLs passed.\n";
} finally {
    $iterator = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($root, FilesystemIterator::SKIP_DOTS), RecursiveIteratorIterator::CHILD_FIRST);
    foreach ($iterator as $file) { if ($file->isDir()) rmdir($file->getPathname()); else unlink($file->getPathname()); }
    rmdir($root);
}
