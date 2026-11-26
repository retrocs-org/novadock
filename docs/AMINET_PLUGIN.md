# AMINET plugin

Click **AMINET** to open its native browser window. It needs a running TCP/IP
stack with `bsdsocket.library`; no external downloader or web browser is needed.
The first opening loads recent uploads. Networking runs in separate processes,
so the dock and browser controls remain responsive during a transfer.

## Browse and search

- **Recent** lists recent uploads. Previous and Next move through result pages.
- **Categories** lists Aminet's main sections. Select one and press **Open**,
  then select a subcategory and press Open again to list its packages.
- Enter at least two characters in the search field and press Return or Search.
- Select a package and press **Open** for its summary, version, architecture,
  author and requirements. Check these before using a package.
- **Readme** displays its text in a scrollable list. **Back** returns to packages.
- **Get** opens a Save dialog. Choose the drawer and filename to download it.
  A progress bar shows the received percentage and downloaded/total size. It clears
  when the transfer finishes or fails; closing and reopening the browser keeps
  the current progress.

Existing destination files are never overwritten. A transfer is staged beside
its destination and committed by rename only after the declared size is
received. A Workbench project icon is added when icon.library is available.
Downloaded archives and installers are never executed automatically.

## Settings and limits

Right-click AMINET and choose Edit settings to set its default download drawer.
The default is `RAM:`; choose a disk drawer to retain packages after a reboot.
One transfer runs at a time. Closing the browser does not cancel an active
transfer; it can finish while the dock is running. If NovaDock quits, its worker
finishes independently and removes the unclaimed temporary download.

The browser retains its current result page for the running session, without
background polling. Metadata is limited to 256 KB and a readme to 512 displayed
lines; packages are limited to 64 MB. A listing contains at most 50 packages.
No result page is presented as current when its request fails.

The transport contacts only `m68k.aminet.net` over HTTP. HTTP does not authenticate
package contents. Redirects, compressed/chunked responses, malformed headers,
oversized and incomplete downloads are rejected. DNS timing depends on the
installed TCP/IP stack. Aminet's HTML layout can change and require a parser
update. The plugin is not affiliated with Aminet.

Parsing and transport were adapted from Beam68 under its MIT licence.
The package includes `Aminet-parser-LICENSE.txt` with the original notice.

## Live FS-UAE test

Run `python3 tools/run_aminet_qa.py` from a configured source tree. The test
starts a disposable AmigaOS 3.2.3 guest with host-socket networking enabled, opens
the real Aminet browser, searches for JustName, selects the result and saves it
through the normal ASL requester. It checks the archive header, compares the
bytes with an independent HTTPS download, and tests LHA member CRCs. The package
is never installed, extracted or executed. Existing persistent guest drives are
not mounted. Network/service failures make this opt-in test fail; it is separate
from the normal offline host suite. `--no-build` uses existing native binaries.

Downloads validate the HTTP response, declared size, byte bounds and archive
magic before the staged file is committed. Final numeric transfer statistics
are reported even for files that complete between progress updates.
