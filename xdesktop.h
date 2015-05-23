#ifndef _XDESKTOP_H
#define _XDESKTOP_H

xcb_connection_t *dpy;
xcb_ewmh_connection_t *ewmh;
int default_screen;
bool running;

void hold(int);
void setup(void);
void output_desktop();
void desktop_changed(xcb_generic_event_t *);

#endif
