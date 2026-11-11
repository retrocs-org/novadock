#include "nd_eyes.h"
/* Avoid floating point, sqrt, and large coordinate products on 68000. */
void nd_eyes_pupil(int eye_x, int eye_y, int mouse_x, int mouse_y,
                   int travel_x, int travel_y, int sensitivity,
                   int *pupil_x, int *pupil_y)
{
    long dx,dy,nx,ny,den;
    if (!pupil_x || !pupil_y) return;
    if (travel_x < 0) travel_x=0;
    if (travel_y < 0) travel_y=0;
    if (sensitivity < 0) sensitivity=0;
    if (sensitivity > 100) sensitivity=100;
    dx=(long)mouse_x-eye_x;
    dy=(long)mouse_y-eye_y;
    /* Normalize both axes. Clamping uses an inscribed diamond, so pupils
     * cannot touch the eye outline at any viewing direction. */
    nx=dx < 0 ? -dx : dx;
    ny=dy < 0 ? -dy : dy;
    den=nx+ny;
    if (!den) { *pupil_x=eye_x; *pupil_y=eye_y; return; }
    /* Division before multiplication keeps arithmetic within signed LONG
     * for every supported 68000 Workbench screen dimension. */
    nx=(long)travel_x * 100L * nx / den;
    ny=(long)travel_y * 100L * ny / den;
    nx=nx*sensitivity/10000L;
    ny=ny*sensitivity/10000L;
    *pupil_x=eye_x+(dx<0 ? -(int)nx : (int)nx);
    *pupil_y=eye_y+(dy<0 ? -(int)ny : (int)ny);
}
int nd_eyes_moved(int old_x, int old_y, int new_x, int new_y)
{
    return old_x!=new_x || old_y!=new_y;
}
