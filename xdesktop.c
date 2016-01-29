#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include "helpers.h"
#include "xdesktop.h"

unsigned int cur_desktop;
xcb_atom_t cur_desktop_atom;

int main(int argc, char *argv[])
{
	bool get = true;
	bool snoop = false;
	char opt;

	while ((opt = getopt(argc, argv, "hvsfc:")) != -1) {
		switch (opt) {
			case 'h':
				printf("xdesktop [-h|-v|-s|-c DESKTOP]\n");
				return EXIT_SUCCESS;
				break;
			case 'v':
				printf("%s\n", VERSION);
				return EXIT_SUCCESS;
				break;
			case 's':
				snoop = true;
				break;
			case 'c':
				get = false;
				break;
		}
	}

	setup();

	if (get) {
		output_desktop();

		if (snoop) {
			const uint32_t values[] = { XCB_EVENT_MASK_PROPERTY_CHANGE};
			xcb_change_window_attributes (dpy, screen->root, XCB_CW_EVENT_MASK, values);

			xcb_intern_atom_cookie_t ac = xcb_intern_atom(dpy, 0, strlen("_NET_CURRENT_DESKTOP"), "_NET_CURRENT_DESKTOP");
			cur_desktop_atom = xcb_intern_atom_reply(dpy, ac, NULL)->atom;

			signal(SIGINT, hold);
			signal(SIGHUP, hold);
			signal(SIGTERM, hold);
			fd_set descriptors;
			int fd = xcb_get_file_descriptor(dpy);
			running = true;
			xcb_flush(dpy);
			while (running) {
				FD_ZERO(&descriptors);
				FD_SET(fd, &descriptors);
				if (select(fd + 1, &descriptors, NULL, NULL, NULL) > 0) {
					xcb_generic_event_t *evt;
					while ((evt = xcb_poll_for_event(dpy)) != NULL) {
						if (desktop_changed(evt))
							output_desktop();
						free(evt);
					}
				}
				if (xcb_connection_has_error(dpy)) {
					warn("The server closed the connection.\n");
					running = false;
				}
			}
		}
	}
	else {
		xcb_intern_atom_cookie_t ac = xcb_intern_atom(dpy, 0, strlen("_NET_CURRENT_DESKTOP"), "_NET_CURRENT_DESKTOP");
		cur_desktop_atom = xcb_intern_atom_reply(dpy, ac, NULL)->atom;

		xcb_client_message_event_t ev;

		ev.response_type = XCB_CLIENT_MESSAGE;
		ev.sequence = 0;
		ev.format = 32;
		ev.window = screen->root;
		ev.type = cur_desktop_atom;
		ev.data.data32[0] = 1;
		ev.data.data32[1] = XCB_CURRENT_TIME;

		xcb_send_event(dpy, 0, screen->root, XCB_EVENT_MASK_NO_EVENT, (char *)&ev); 
	}

	xcb_ewmh_connection_wipe(ewmh);
	free(ewmh);
	xcb_disconnect(dpy);
	return EXIT_SUCCESS;
}

void setup(void)
{
	dpy = xcb_connect(NULL, &default_screen);
	if (xcb_connection_has_error(dpy))
		err("Can't open display.\n");
	screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
	if (screen == NULL)
		err("Can't acquire screen.\n");
	ewmh = malloc(sizeof(xcb_ewmh_connection_t));
	if (xcb_ewmh_init_atoms_replies(ewmh, xcb_ewmh_init_atoms(dpy, ewmh), NULL) == 0)
		err("Can't initialize EWMH atoms.\n");
}

void output_desktop(void)
{
	xcb_ewmh_get_current_desktop_reply(ewmh, xcb_ewmh_get_current_desktop(ewmh, default_screen), &cur_desktop, NULL);

	printf("%i\n", cur_desktop);
	fflush(stdout);
}

bool desktop_changed(xcb_generic_event_t *evt)
{
	if ((evt->response_type & ~0x80) == XCB_PROPERTY_NOTIFY){
		xcb_property_notify_event_t *pne = (xcb_property_notify_event_t*) evt;
		if(pne->atom == cur_desktop_atom)
			return true;
	}
	return false;
}

void hold(int sig)
{
	if (sig == SIGHUP || sig == SIGINT || sig == SIGTERM)
		running = false;
}
