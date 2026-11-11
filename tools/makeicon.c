/* Generate original NovaDock WBTOOL icons without overwriting user icons.
 * Usage: MakeIcon (all shipped executables), or MakeIcon <program> [id]. */
#include <exec/types.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>
#include <proto/exec.h>
#include <proto/icon.h>
#include <proto/dos.h>
#include <stdlib.h>
#include "nd_version.h"

/* Version string for the AmigaDOS Version command (kept global so the
 * linker never discards it). */
const char nd_version[]=ND_VER_STRING("MakeIcon");

#define ICON_W 32
#define ICON_H 24
#define ROW_WORDS 2
#define PLANE_WORDS (ROW_WORDS * ICON_H)
#define ICON_WORDS (PLANE_WORDS * 2)

enum IconKind {
    ICON_DOCK = 0, ICON_PREFS, ICON_LAUNCHER_PREFS, ICON_PLUGIN_PREFS,
    ICON_MAKEICON, ICON_STARTUP, ICON_CLOCK, ICON_EYES, ICON_ACTIVITY,
    ICON_MEMORY, ICON_NETWORK, ICON_MUSIC, ICON_DOPUS,
    ICON_ARCHIVE, ICON_COMMAND
};

struct IconTarget { const char *path; int kind; };
static const struct IconTarget targets[] = {
    { "PROGDIR:NovaDock", ICON_DOCK },
    { "PROGDIR:NovaDockPrefs", ICON_PREFS },
    { "PROGDIR:MakeIcon", ICON_MAKEICON },
    { "PROGDIR:SetupStartup", ICON_STARTUP },
    { "PROGDIR:Plugins/Clock", ICON_CLOCK },
    { "PROGDIR:Plugins/Eyes", ICON_EYES },
    { "PROGDIR:Plugins/Activity", ICON_ACTIVITY },
    { "PROGDIR:Plugins/Memory", ICON_MEMORY },
    { "PROGDIR:Plugins/Network", ICON_NETWORK },
    { "PROGDIR:Plugins/MusicPlayer", ICON_MUSIC },
    { "PROGDIR:Plugins/DOpus4", ICON_DOPUS },
    { "PROGDIR:Plugins/ArchiveExtractor", ICON_ARCHIVE },
    { "PROGDIR:Plugins/Command", ICON_COMMAND }
};

static UWORD normal_data[ICON_WORDS];
static UWORD selected_data[ICON_WORDS];
struct Library *IconBase = NULL;

static void clear_words(UWORD *data)
{
    int i;
    for (i = 0; i < ICON_WORDS; ++i) data[i] = 0;
}

static void pixel(int x, int y, int color)
{
    int plane;
    UWORD mask;
    int word;
    if (x < 0 || x >= ICON_W || y < 0 || y >= ICON_H) return;
    mask = (UWORD)(1U << (15 - (x & 15)));
    word = y * ROW_WORDS + (x >> 4);
    for (plane = 0; plane < 2; ++plane) {
        if (color & (1 << plane)) normal_data[plane * PLANE_WORDS + word] |= mask;
        else normal_data[plane * PLANE_WORDS + word] &= (UWORD)~mask;
    }
}

static int read_pixel(int x, int y)
{
    int plane, color = 0;
    UWORD mask = (UWORD)(1U << (15 - (x & 15)));
    int word = y * ROW_WORDS + (x >> 4);
    for (plane = 0; plane < 2; ++plane)
        if (normal_data[plane * PLANE_WORDS + word] & mask) color |= 1 << plane;
    return color;
}

static void selected_pixel(int x, int y, int color)
{
    int plane;
    UWORD mask = (UWORD)(1U << (15 - (x & 15)));
    int word = y * ROW_WORDS + (x >> 4);
    for (plane = 0; plane < 2; ++plane)
        if (color & (1 << plane)) selected_data[plane * PLANE_WORDS + word] |= mask;
}

static void rect(int x1, int y1, int x2, int y2, int color)
{
    int x, y;
    for (y = y1; y <= y2; ++y)
        for (x = x1; x <= x2; ++x) pixel(x, y, color);
}

static void line(int x1, int y1, int x2, int y2, int color)
{
    int dx = x2 > x1 ? x2 - x1 : x1 - x2;
    int sx = x1 < x2 ? 1 : -1;
    int dy = y2 > y1 ? y1 - y2 : y2 - y1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        if (2 * err >= dy) { err += dy; x1 += sx; }
        if (2 * err <= dx) { err += dx; y1 += sy; }
    }
}

static void frame(void)
{
    rect(2, 3, 29, 21, 1);
    rect(3, 4, 28, 20, 0);
    rect(3, 18, 28, 20, 2);
    rect(5, 16, 9, 19, 3);
    rect(13, 16, 17, 19, 3);
    rect(21, 16, 25, 19, 3);
}

static void circle(int cx, int cy, int r, int color)
{
    int x, y, d;
    for (y = -r; y <= r; ++y)
        for (x = -r; x <= r; ++x) {
            d = x * x + y * y;
            if (d >= (r - 1) * (r - 1) && d <= r * r + r) pixel(cx + x, cy + y, color);
        }
}

static void draw_icon(int kind)
{
    int x, y, c;
    clear_words(normal_data);
    clear_words(selected_data);
    /* The two public applications get their own recognizable imagery.
     * Other helper/plugin icons retain the compact dock-shaped frame. */
    if (kind != ICON_DOCK && kind != ICON_PREFS) frame();
    switch (kind) {
    case ICON_DOCK:
        rect(3, 6, 28, 19, 1); rect(5, 7, 26, 17, 2);
        pixel(3, 6, 0); pixel(28, 6, 0);
        pixel(3, 19, 0); pixel(28, 19, 0);
        rect(7, 9, 11, 15, 3); rect(14, 9, 18, 15, 3);
        rect(21, 9, 25, 15, 3);
        line(4, 18, 27, 18, 3); break;
    case ICON_PREFS:
        rect(3, 4, 28, 20, 1); rect(5, 6, 26, 18, 3);
        rect(5, 6, 26, 8, 2); rect(23, 6, 25, 7, 1);
        line(7, 11, 24, 11, 1); line(7, 14, 24, 14, 1);
        line(7, 17, 24, 17, 1);
        rect(10, 10, 12, 12, 2); rect(19, 13, 21, 15, 2);
        rect(14, 16, 16, 18, 2); break;
    case ICON_LAUNCHER_PREFS:
        line(6, 11, 22, 11, 2); line(17, 6, 23, 11, 3); line(17, 16, 23, 11, 3);
        rect(6, 7, 8, 15, 1); break;
    case ICON_PLUGIN_PREFS:
        rect(8, 8, 23, 15, 2); rect(12, 5, 17, 9, 2); rect(21, 10, 26, 13, 2);
        rect(14, 10, 17, 13, 0); rect(7, 11, 10, 14, 3); break;
    case ICON_MAKEICON:
        rect(7, 6, 19, 14, 2); rect(10, 9, 22, 17, 3);
        line(23, 6, 26, 9, 1); line(26, 6, 23, 9, 1); break;
    case ICON_STARTUP:
        line(16, 5, 16, 15, 3); line(10, 10, 16, 5, 3); line(22, 10, 16, 5, 3);
        line(9, 15, 23, 15, 2); break;
    case ICON_CLOCK:
        circle(16, 11, 7, 2); pixel(16, 11, 3); line(16, 11, 16, 6, 3); line(16, 11, 21, 13, 3); break;
    case ICON_EYES:
        circle(11, 11, 5, 2); circle(21, 11, 5, 2);
        rect(10, 10, 12, 13, 3); rect(20, 10, 22, 13, 3); break;
    case ICON_ACTIVITY:
        line(6, 14, 10, 11, 3); line(10, 11, 13, 14, 3); line(13, 14, 17, 6, 3);
        line(17, 6, 20, 13, 3); line(20, 13, 26, 9, 3); break;
    case ICON_MEMORY:
        rect(9, 7, 23, 15, 2); rect(12, 9, 20, 13, 3);
        for (x = 10; x <= 22; x += 3) { pixel(x, 6, 1); pixel(x, 16, 1); } break;
    case ICON_NETWORK:
        rect(14, 6, 18, 10, 3); rect(6, 12, 10, 16, 2); rect(22, 12, 26, 16, 2);
        line(16, 10, 8, 12, 1); line(16, 10, 24, 12, 1); line(8, 16, 24, 16, 1); break;
    case ICON_MUSIC:
        line(13, 6, 13, 14, 3); line(13, 6, 23, 5, 3); line(23, 5, 23, 12, 3);
        circle(10, 15, 3, 2); circle(20, 13, 3, 2); break;
    case ICON_DOPUS:
        rect(6, 6, 14, 16, 2);rect(18,6,26,16,3);
        line(8,9,12,9,3);line(8,12,12,12,3);
        line(20,9,24,9,1);line(20,12,24,12,1);break;
    case ICON_ARCHIVE:
        rect(7, 10, 25, 17, 2); rect(9, 12, 23, 15, 0);
        line(16, 5, 16, 12, 3); line(12, 9, 16, 13, 3);
        line(20, 9, 16, 13, 3); break;
    case ICON_COMMAND:
        /* A shell prompt: ">" followed by a cursor bar. */
        line(8, 7, 12, 10, 3); line(12, 10, 8, 13, 3);
        rect(15, 12, 22, 13, 2); break;
    }
    for (y = 0; y < ICON_H; ++y)
        for (x = 0; x < ICON_W; ++x) {
            c = read_pixel(x, y);
            c = c == 0 ? 0 : (c == 1 ? 2 : (c == 2 ? 3 : 1));
            selected_pixel(x, y, c);
        }
}

static int make_one(const char *path, int kind)
{
    struct DiskObject *disk, *existing;
    struct Image normal_image, selected_image;
    APTR old_render, old_select;
    UWORD old_flags;
    int result = 10;
    existing = GetDiskObject((STRPTR)path);
    if (existing) {
        FreeDiskObject(existing);
        Printf("Keeping existing icon for %s\n", path);
        return 0;
    }
    if (kind < ICON_DOCK || kind > ICON_COMMAND) kind = ICON_DOCK;
    draw_icon(kind);
    normal_image.LeftEdge = normal_image.TopEdge = 0;
    normal_image.Width = ICON_W; normal_image.Height = ICON_H; normal_image.Depth = 2;
    normal_image.ImageData = normal_data; normal_image.PlanePick = 3; normal_image.PlaneOnOff = 0;
    normal_image.NextImage = NULL;
    selected_image = normal_image;
    selected_image.ImageData = selected_data;
    disk = GetDefDiskObject(WBTOOL);
    if (!disk) return result;
    old_render = disk->do_Gadget.GadgetRender;
    old_select = disk->do_Gadget.SelectRender;
    old_flags = disk->do_Gadget.Flags;
    disk->do_Gadget.LeftEdge = 0; disk->do_Gadget.TopEdge = 0;
    disk->do_Gadget.Width = ICON_W; disk->do_Gadget.Height = ICON_H;
    disk->do_Gadget.Flags = GFLG_GADGIMAGE | GFLG_GADGHIMAGE;
    disk->do_Gadget.Activation = GACT_RELVERIFY;
    disk->do_Gadget.GadgetType = GTYP_BOOLGADGET;
    disk->do_Gadget.GadgetRender = &normal_image;
    disk->do_Gadget.SelectRender = &selected_image;
    disk->do_CurrentX = NO_ICON_POSITION; disk->do_CurrentY = NO_ICON_POSITION;
    disk->do_StackSize = 16384;
    if (PutDiskObject((STRPTR)path, disk)) {
        Printf("Created %s.info\n", path);
        result = 0;
    }
    disk->do_Gadget.GadgetRender = old_render;
    disk->do_Gadget.SelectRender = old_select;
    disk->do_Gadget.Flags = old_flags;
    FreeDiskObject(disk);
    return result;
}

int main(int argc, char **argv)
{
    int result = 0, i;
    IconBase = OpenLibrary("icon.library", 36);
    if (!IconBase) return 20;
    if (argc > 1) {
        int kind = argc > 2 ? atoi(argv[2]) : ICON_DOCK;
        result = make_one(argv[1], kind);
    } else {
        for (i = 0; i < (int)(sizeof(targets) / sizeof(targets[0])); ++i)
            if (make_one(targets[i].path, targets[i].kind) != 0) result = 10;
    }
    CloseLibrary(IconBase);
    return result;
}
