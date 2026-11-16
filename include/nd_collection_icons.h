#ifndef ND_COLLECTION_ICONS_H
#define ND_COLLECTION_ICONS_H
#include <string.h>
#define ND_COLLECTION_ICON_COUNT 30
struct NDCollectionIcon {const char *name,*path;};
static const struct NDCollectionIcon nd_collection_icons[ND_COLLECTION_ICON_COUNT]={
 {"WHDLoad","Icons/WHDLoad.iff"},
 {"Games","Icons/Games.iff"},
 {"Drawer","Icons/Drawer.iff"},
 {"Workbench","Icons/Workbench.iff"},
 {"Tools","Icons/Tools.iff"},
 {"Internet","Icons/Internet.iff"},
 {"Demos","Icons/Demos.iff"},
 {"Apps","Icons/Apps.iff"},
 {"Arcade","Icons/Arcade.iff"},
 {"Music","Icons/Music.iff"},
 {"Graphics","Icons/Graphics.iff"},
 {"Code","Icons/Code.iff"},
 {"Movies","Icons/Movies.iff"},
 {"Books","Icons/Books.iff"},
 {"Disks","Icons/Disks.iff"},
 {"Network","Icons/Network.iff"},
 {"Utilities","Icons/Utilities.iff"},
 {"Favourites","Icons/Favourites.iff"},
 {"Aminet","Icons/Aminet.iff"},
 {"Downloads","Icons/Downloads.iff"},
 {"Search","Icons/Search.iff"},
 {"Radio","Icons/Radio.iff"},
 {"Feeds","Icons/Feeds.iff"},
 {"Printer","Icons/Printer.iff"},
 {"Backup","Icons/Backup.iff"},
 {"Calendar","Icons/Calendar.iff"},
 {"Terminal","Icons/Terminal.iff"},
 {"DiskSpace","Icons/DiskSpace.iff"},
 {"Mail","Icons/Mail.iff"},
 {"Camera","Icons/Camera.iff"},
};
static int nd_collection_icon_index(const char *name)
{int i;if(name)for(i=0;i<ND_COLLECTION_ICON_COUNT;++i)if(!strcmp(name,nd_collection_icons[i].name))return i;return -1;}
#endif
