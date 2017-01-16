#ifndef _XDESKTOP_H
#define _XDESKTOP_H

xcb_connection_t *dpy;
xcb_ewmh_connection_t *ewmh;
xcb_screen_t *screen;
int default_screen;
xcb_window_t root;
bool running;

bool setup(void);
void output_current_desktop();
void output_total_desktops();
bool desktop_changed(xcb_generic_event_t *);
void hold(int sig);

#endif
