# Drawer and volume space

Drawer/Volume items report their containing volume's capacity, rather than
the size of the drawer itself. Enable **Meter** in the item's editor to show
usage. Hover for free, used and total space, formatted with binary-sized
KB/MB/GB/TB units. Drawing and tooltips use cached readings and do not scan
files or perform filesystem requests.

In a drawer item's editor, **Space...** sets the low-space threshold from
1 to 99 percent remaining; 0 disables warnings. The default is 10 percent.
Below the threshold the meter uses the theme's secondary accent and sends
one internal notification, optionally forwarded to AmiNotis. It can notify again after space recovers and then
falls below the threshold. Missing/invalid readings never mean a full disk.

Enable **Show free space** in that dialog to show remaining capacity in
place of the icon label. Use/Save in the parent editor applies/persists these settings.
Existing configurations remain readable with their original meter settings.

Right-click a drawer/volume and choose **Volume overview...** for a filtered
list of mounted volumes and their capacities. Select a row and click Open,
press Return or double-click. Refresh resamples the list on demand. It uses
the bundled PLACES helper, without needing a PLACES tile in the dock.

Queries run in a bounded, independent worker. Identical paths are sampled once per refresh, active paths are protected when the 64-entry cache is full, and each completed reading is delivered immediately instead of waiting for later volumes in the batch.

The tooltip shows **Last updated**, **cached/stale**, or **unavailable**, together with the last DOS error when a query fails. Readings older than 15 seconds are marked stale. Failed requests preserve the last successful capacity; an initial failure shows capacity unavailable. Stale values do not imply that the disk is full or trigger another low-space warning merely because a query failed. Drawing, hovering, reload and Quit do not wait for a volume-space query.
