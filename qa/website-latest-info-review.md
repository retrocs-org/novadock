# Website information review — 18 November 2026

The live GitHub catalogue was checked read-only: NovaDock 0.3, 0.2 and 0.1 are published; 0.4 remains in development. The saved fallback now includes 0.3 and an unpublished 0.4 entry. No release or website was published by this task.

Updated Home, Features, Marketplace, Downloads, Developers, Plugin SDK, Support/FAQ and website maintenance instructions. The site explains the current HASS room/type/search selection, subscriptions, read-only/display/refresh settings, capabilities and confirmed controls. New utilities, monitoring plugins, AmiRocket, update checks, artwork packaging, hardware prerequisites and validation limits are documented. Existing screenshots are identified as 0.3.

Released and upcoming plugin labels/counts share the published release catalogue. The home page currently reports 32 released plugins and eight upcoming plugins. SDK filenames and tags come from the same published record, fixing the mixed 0.3-tag/0.4-filename URL. Preview improvements to released plugins are labelled separately. SDK settings documentation now matches the verified 32-option manifest limit in both 0.3 and current source.

Checks passed:

- PHP rendering, local links, image references, shared navigation and generated chrome.
- Offline 0.3 and future-published-0.4 fixtures for SDK asset URLs, plugin counts and development labels.
- Full website archive checks, current source and standalone SDK ZIPs, and release-cache regression checks.
- Served HTTP 200 responses for Home, Features, Marketplace, Plugin SDK, Downloads and Support, with no PHP warnings. Downloads selected public 0.3 and the SDK URL matched that version.
- Application version consistency, generated release notes and git diff whitespace checks.

Upload-ready bundle: outputs/website-latest-20261118/NovaDock-0.4-public-web-updated.zip.
Local preview: http://127.0.0.1:18184/.
Live deployment awaits the site's SSH/SFTP profile or deployment command. The repository documents a manual upload and no matching local SSH profile was found. No credentials were requested in chat.
