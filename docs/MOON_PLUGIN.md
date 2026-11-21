# MOON plugin

Add MOON from Add Item → Plugin. Its sixteen-frame tile follows the estimated
moon phase. Click for phase name, illumination, lunar age, sunrise/sunset and
the date and UTC offset used. No network connection or FPU is required.

Configure latitude and longitude in decimal degrees (north/east positive) and
UTC offset as `+01:00` or `-05:30`. Set the current local offset, including any
summer-time adjustment; MOON does not infer daylight-saving rules. The Amiga
clock is treated as local time. Default location and offset are zero, so enter
your own values for local sunrise/sunset. Preferences validates coordinates and
UTC offsets before Use/Save and explains invalid values in the window title. Refresh defaults to sixty minutes,
with an optional interval from fifteen to 360 minutes and a manual Refresh.

The mean-cycle lunar estimate uses the 2000-01-06 18:14 UTC new moon and a
29.530588853-day cycle. Solar estimates use NOAA's fractional-year equations
and apparent horizon. They are approximate, ignore local terrain and weather,
and report polar day/night when the sun does not cross that horizon. Dates
1978–2099 are supported. These estimates are unsuitable for navigation or
precise astronomical timing. Artwork ships in Moon.assets.

References: [NASA lunar phases](https://eclipse.gsfc.nasa.gov/phase/phases1901.html)
and [NOAA solar equations](https://gml.noaa.gov/grad/solcalc/solareqns.PDF).
