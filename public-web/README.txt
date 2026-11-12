NOVADOCK 0.1 - PUBLIC WEBSITE
============================

This is the public NovaDock PHP website. Shared server-side templates render
HTML pages; one local stylesheet provides responsive layout, modern typography
and open feature layouts. Classic browsers retain a readable table-based fallback. No client
scripts, external fonts, database or framework are required.

Requirements: PHP 7.4 or newer on the web server; use a maintained PHP version.
Each route includes includes/layout.php for the shared header, navigation and
footer. Edit content between <!-- content --> and <!-- /content --> markers.
Regenerate/check shared templates with tools/update_site_chrome.py.

PUBLIC LOCATIONS
  Website: https://novadock.retrocs.org
  Source:  https://github.com/retrocs-org/novadock

CONTENTS
  index.php        Home page
  features.php     Dock features and ten screenshots
  marketplace.php  Plugin and theme catalogs; future community additions
  plugins.php      Redirect for older links
  theme-development.php  Theme creation, packaging and import
  themes.php       Redirect for older theme guide links
  get.php          Automatic GitHub downloads and installation
  download.php     Redirect for older download links
  developers.php   SDK/theme guides and source compilation/install instructions
  plugin-sdk.php   Plugin SDK, settings windows and packaging
  support.php      Help, contact information and FAQ
  source.php       Redirect to Developers source section
  includes/faq.php  Collapsible questions on the Support page
  faq.php          Redirect for older FAQ links
  screenshots.php  Redirect to Features screenshot gallery
  includes/layout.php  Shared PHP rendering functions
  includes/releases.php  Automatic public GitHub catalogue and cache
  includes/*_catalog.php  Bundled plugin and theme listings
  site.css          Responsive presentation and classic fallback
  images/*.gif      32x32 indexed icons (legacy browser friendly)
  images/logo.gif   header logo; images/dock.gif legacy illustration
  screenshots/*.gif actual application crops and lightweight previews
  images/px.gif     transparent 1x1 spacer for card accent strips
  downloads/*.zip   Standalone downloads (no recursive website copy)
  downloads/NovaDock-0.1-plugin-sdk.zip   Standalone plugin SDK and example
  downloads/NovaDock-0.1.lha  Verified runnable release
  downloads/NovaDock-0.1.readme  Release information
  downloads/SHA256SUMS.txt   Binary, readme, source and SDK checksums

HOW TO PREVIEW AND PUBLISH
  1. For local preview: php -S 127.0.0.1:8080 -t public-web
     (Run from the NovaDock project root.)
  2. Copy the contents of public-web/ to a PHP-enabled web server document root.
  3. Set index.php as the directory index and enable PHP processing for .php.
     Do not serve PHP source as plain text. No URL rewriting is required.
  4. Keep includes/, images/, screenshots/, site.css and downloads/ together.
  5. Serve .gif as image/gif, .css as text/css, .zip as application/zip and
     .lha as application/octet-stream. Use HTTPS for the public endpoint.
  6. Remove obsolete .html files from earlier deployments so index.php is used.

The PHP templates escape page metadata and take no request input. Navigation
and downloads work without CSS, images or client scripts. Publishing a release
attaches the website ZIP; deployment to your web server remains a separate step.

SOURCE RELEASE WARNING
The downloadable ZIP is the development source tree without public-web/
(to avoid packaging the website ZIP recursively). The OUTER NovaDock
project archive includes both the source tree and this public-web/ folder.
Compiled 68k binaries and the installer are included in the separate ready-to-install package; the website source ZIP remains source-only.

VERSION: 0.1 (no change without project owner approval)

REBUILD PACKAGES
  From the NovaDock project root on a modern machine run:
  python3 tools/package_release.py --out /path/to/output
  This refreshes downloads/ and generates full-project and website-only ZIPs.

RELEASE LIST
  Download links point to tagged GitHub release assets. The current planned
  release is labelled Upcoming until it is published. The links intentionally
  do not claim that unpublished files exist.
  Refresh published entries: python3 tools/sync_github_releases.py
  Prepare the VERSION tag offline: python3 tools/sync_github_releases.py --upcoming
  The SDK source is included in the repository under plugin-sdk/.

AUTOMATIC RELEASE DATA
  Get NovaDock reads the public retrocs-org/novadock GitHub releases API.
  Published releases, including previews, are ordered by publication time.
  A private cache below the system temporary directory refreshes every 15 minutes.
  A non-blocking lock avoids duplicate refreshes. API failures retain the last
  valid release list and retry after one minute; releases.json is the fallback.
  No token, cron job, database or JavaScript is required. HTTPS needs PHP cURL or
  allow_url_fopen with working certificate trust. The site directory may remain
  read-only; the PHP user only needs access to its system temporary directory.
  The Source page only explains obtaining, compiling and installing the code.
