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

unsigned int cur_desktop, num_desktops;

int main(int argc, char *argv[])
{
	char opt;

	while ((opt = getopt(argc, argv, "hvsef:t:")) != -1) {
		switch (opt) {
			case 'h':
				printf("xdesktop [-h|-v|-s\n");
				return EXIT_SUCCESS;
				break;
			case 'v':
				printf("%s\n", VERSION);
				return EXIT_SUCCESS;
				break;
//			case 's':
//				snoop = true;
//				break;
		}
	}

	setup();

	output_desktop();

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
	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
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

void hold(int sig)
{
	if (sig == SIGHUP || sig == SIGINT || sig == SIGTERM)
		running = false;
}
