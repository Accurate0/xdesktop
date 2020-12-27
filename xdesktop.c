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
#include "xdesktop.h"

uint32_t old_desktop = -1;
uint32_t cur_desktop;
uint32_t tot_desktops;
xcb_atom_t cur_desktop_atom;

int main(int argc, char *argv[])
{
	dpy = NULL;
	ewmh = NULL;
	bool snoop = false;
	bool total = false;
	bool get = true;
	bool nextprev = false;
	char direction;
	unsigned int query = 0;
	int ret = EXIT_SUCCESS;
	char opt;

	signal(SIGINT, hold);
	signal(SIGHUP, hold);
	signal(SIGTERM, hold);

	while ((opt = getopt(argc, argv, "hvstg:pn")) != -1) {
		switch (opt) {
			case 'h':
				printf("xdesktop [-h|-v|-s|-t|-p|-n|-g DESKTOP]\n");
				goto end;
				break;
			case 'v':
				printf("%s\n", VERSION);
				goto end;
				break;
			case 's':
				snoop = true;
				break;
			case 't':
				total = true;
				break;
			case 'p':
			case 'n':
				get = false;
				nextprev = true;
				direction = opt;
				break;
			case 'g':
				get = false;
				query = atoi(optarg);
				break;
		}
	}

	if (!setup()) {
		ret = EXIT_FAILURE;
		goto end;
	}

	if (get) {
		if (total) {
			output_total_desktops();
		} else {
			output_current_desktop();

			if (snoop) {
				const uint32_t values[] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
				xcb_change_window_attributes (dpy, root, XCB_CW_EVENT_MASK, values);

				xcb_intern_atom_cookie_t ac = xcb_intern_atom(dpy, 0, strlen("_NET_CURRENT_DESKTOP"), "_NET_CURRENT_DESKTOP");
				cur_desktop_atom = xcb_intern_atom_reply(dpy, ac, NULL)->atom;

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
								output_current_desktop();
							free(evt);
						}
					}
					if (xcb_connection_has_error(dpy)) {
						warnx("The server closed the connection.\n");
						running = false;
					}
				}
			}
		}
	} else {
		xcb_ewmh_get_number_of_desktops_reply(ewmh, xcb_ewmh_get_number_of_desktops(ewmh, default_screen), &tot_desktops, NULL);
		tot_desktops = tot_desktops - 1;

		if (nextprev) {
			xcb_ewmh_get_current_desktop_reply(ewmh, xcb_ewmh_get_current_desktop(ewmh, default_screen), &cur_desktop, NULL);

			switch (direction) {
				case 'p':
					if (cur_desktop == 0) {
						query = tot_desktops;
					} else {
						query = cur_desktop - 1;
					}
					break;
				case 'n':
					if (cur_desktop == tot_desktops) {
						query = 0;
					} else {
						query = cur_desktop + 1;
					}
					break;
			}
		} else {
			query = query - 1;

			if (query > tot_desktops)
				err("You don't have a desktop %i.\n", query + 1);
		}

		xcb_ewmh_request_change_current_desktop(ewmh, default_screen, query, XCB_CURRENT_TIME);
		xcb_flush(dpy);
	}

end:
	if (ewmh != NULL) {
		xcb_ewmh_connection_wipe(ewmh);
	}
	if (dpy != NULL) {
		xcb_disconnect(dpy);
	}
	free(ewmh);
	return ret;
}

bool setup(void)
{
	dpy = xcb_connect(NULL, &default_screen);
	if (xcb_connection_has_error(dpy)) {
		warnx("can't open display.");
		return false;
	}
	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
	if (screen == NULL) {
		warnx("can't acquire screen.");
		return false;
	}
	root = screen->root;
	ewmh = malloc(sizeof(xcb_ewmh_connection_t));
	if (xcb_ewmh_init_atoms_replies(ewmh, xcb_ewmh_init_atoms(dpy, ewmh), NULL) == 0) {
		warnx("can't initialize EWMH atoms.");
		return false;
	}
	return true;
}

void output_current_desktop(void)
{
	xcb_ewmh_get_current_desktop_reply(ewmh, xcb_ewmh_get_current_desktop(ewmh, default_screen), &cur_desktop, NULL);
	cur_desktop = cur_desktop + 1;

	if(old_desktop != cur_desktop) {
		printf("%i\n", cur_desktop);
		old_desktop = cur_desktop;
	}

	fflush(stdout);
}

void output_total_desktops(void)
{
	xcb_ewmh_get_number_of_desktops_reply(ewmh, xcb_ewmh_get_number_of_desktops(ewmh, default_screen), &tot_desktops, NULL);

	printf("%i\n", tot_desktops);
	fflush(stdout);
}

bool desktop_changed(xcb_generic_event_t *evt)
{
	if (XCB_EVENT_RESPONSE_TYPE(evt) == XCB_PROPERTY_NOTIFY){
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
