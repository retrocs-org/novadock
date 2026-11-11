#ifndef ND_EYES_H
#define ND_EYES_H
/* Portable, integer-only pupil positioning for the bundled Eyes tile.
 * Input and output are absolute screen coordinates. The pupil is kept inside
 * the supplied rectangular travel limits; caller draws the eye and pupil.
 */
void nd_eyes_pupil(int eye_x, int eye_y, int mouse_x, int mouse_y,
                   int travel_x, int travel_y, int sensitivity,
                   int *pupil_x, int *pupil_y);
/* Returns 1 if the two pupil positions differ visibly; no redraw otherwise. */
int nd_eyes_moved(int old_x, int old_y, int new_x, int new_y);
#endif
