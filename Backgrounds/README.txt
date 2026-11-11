NovaDock sample dock-bar skins and backgrounds
==============================================

ClassicGold.iff + ClassicGoldLeft/Right.iff
  A clean, warm ochre/gold classic-Amiga dock body with compact rounded end
  caps, a dark lower edge and restrained pixel texture.

ClassicBlue.iff + ClassicBlueLeft/Right.iff
  The same original three-part design in cobalt and steel blue.

NovaDock recognises a matching BaseLeft.iff/BaseRight.iff pair and keeps those
caps fixed while repeating Base.iff through the centre. This makes a coherent
bar at any horizontal dock width instead of wallpapering a noisy texture.
Transparent pixels reveal the captured Workbench desktop behind the shaped
skin, including the narrow margin above and below the bar.

The samples were generated specifically for NovaDock, reduced to 64 x 64 per
part, and converted to compressed 4-plane IFF/ILBM files for classic Amiga
screens. The generated source strips are retained in Source/.

BrushedMetal.iff and Circuit.iff remain for compatibility with old packages,
but settings that reference them are upgraded to ClassicGold and ClassicBlue.
Custom single-image backgrounds still tile and use the same limits: ILBM,
1--4 planes, no larger than 64 x 64 pixels.
