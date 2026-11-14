# Weather plugin

Add **Weather** from Preferences → Add item → Plugin. Its executable is
`Plugins/Weather`, ID `WEATHER`, and its independent plugin version is 0.1.

## Location and forecasts

Right-click the tile and choose **Edit settings...**. Enter a city and country,
for example `Stockholm, Sweden`, and choose Celsius or Fahrenheit. Save or Use
these settings. An empty location leaves network fetching disabled.

The tile shows the current modelled temperature and a weather symbol. Its
pixel-art icon changes automatically for clear skies, partial cloud, overcast,
fog, drizzle, rain, sleet, snow and thunderstorms, without animation timers. Click
it for today's conditions and five daily forecasts labelled with local weekday
names, with low/high temperatures
and rain probability. **Refresh** asks for an update; manual requests are
limited to one a minute. Close or Escape dismisses the window.

Automatic updates run every 30 minutes by default. Downloads take place in a
separate process so the dock and forecast controls remain responsive. Failed
updates retain the last successful data; a star badge marks stale cached
conditions. The window shows the observation time and update status.

## Requirements

An internet connection and an HTTPS-capable Amiga Wget or curl are required.
Select the downloader executable and its type in the plugin settings. The default is `C:curl` with downloader type **CURL**. A downloader with current TLS and certificate support
is needed; certificate checks are not disabled. Only one download runs at a
time. HTTPS connections allow up to 90 seconds for DNS, connection and TLS
setup, and up to 120 seconds for the complete transfer. The dock stops waiting
after 150 seconds; the independent downloader cleans up when it exits. The
downloader runs below normal task priority. Failed updates retry after five
minutes.

For Classic AmigaOS, use a TLS-enabled CPU build of
[curl](https://aminet.net/package/comm/tcp/curl-8.23.0) and the
[AmiSSL version required by that build](https://github.com/jens-maus/amissl/releases).
The 68000 curl build has no TLS; choose a 68020 or later build on a matching CPU.
Install the downloader separately, then set its executable path and type in
Weather settings. Existing settings are kept when upgrading. A Wget build that
reports “Unsupported scheme” cannot fetch HTTPS URLs, even when ping and ordinary
web downloads work. Weather reports this limitation in its forecast window.
Do not disable certificate checks; ensure the Amiga's date is correct and its
certificate bundle is current.

The last successful forecast is saved in `ENVARC:NovaDock/Weather.cache`,
keyed by location and units. A cached forecast can be shown after restarting
or when offline. Invalid or partial responses never replace valid data.

## Settings

| Setting | Meaning | Default |
|---|---|---|
| Location | City and country used for lookup | Empty |
| Temperature units | Celsius or Fahrenheit | C |
| HTTPS downloader | Wget or curl executable | `C:curl` |
| Downloader type | Command-line options for WGET or CURL | CURL |
| Refresh minutes | Automatic interval, 15–360 minutes | 30 |

## Weather data

Data is provided by [Open-Meteo](https://open-meteo.com/) under
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Current conditions
are weather-model estimates. The free API is for non-commercial use; see
[Open-Meteo's terms](https://open-meteo.com/en/terms) for service details.
The city entered in settings is sent for geocoding, then its coordinates are
used to request a forecast. No precise device-location detection is used.

The weather test fixture in the source tree is an Open-Meteo Stockholm
response and retains this attribution.
