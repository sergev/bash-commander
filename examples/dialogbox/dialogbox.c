/*
 * Display dialog boxes from shell scripts.
 * Copyright (C) 2007 Serge Vakulenko.
 * Usage:
 *	dialogbox [<options>...] <box type>
 *
 * Dialog boxes:
 * 	--infobox "text"
 * 	--msgbox "text"
 * 	--yesno "text"
 * 	--inputbox "text" "value"
 * 	--menu "text" "variants"
 *
 * Options:
 * 	--title "string"
 * 	--ok-label "string"
 * 	--no-label "string"
 * 	--extra-label "string"
 *	--clear-before
 *	--clear-after
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "config.h"

const char copyright[] =
	"Copyright (C) 2007 Serge Vakulenko\n"
	"This is free software, covered by the GNU General Public License.";

typedef enum {
	SHOWUSAGE,
	SHOWVERSION,
	INFOBOX,
	MSGBOX,
	YESNO,
	INPUTBOX,
	MENU,
	TITLE,
	OK_LABEL,
	CANCEL_LABEL,
	EXTRA_LABEL,
	CLEAR_BEFORE,
	CLEAR_AFTER,
} boxtype_t;

/* Table of options. */
static struct option longopts[] = {
	/* option	     has arg		integer code */
	{ "help",		0,	0,	SHOWUSAGE,	},
	{ "version",		0,	0,	SHOWVERSION,	},
	{ "infobox",		1,	0,	INFOBOX,	},
	{ "msgbox",		1,	0,	MSGBOX,		},
	{ "yesno",		1,	0,	YESNO,		},
	{ "inputbox",		1,	0,	INPUTBOX,	},
	{ "menu",		1,	0,	MENU,		},
	{ "title",		1,	0,	TITLE,		},
	{ "ok-label",		1,	0,	OK_LABEL,	},
	{ "cancel-label",	1,	0,	CANCEL_LABEL,	},
	{ "extra-label",	1,	0,	EXTRA_LABEL,	},
	{ "clear-before",	0,	0,	CLEAR_BEFORE,	},
	{ "clear-after",	0,	0,	CLEAR_AFTER,	},
	{ 0,			0,	0,	0,		},
};

boxtype_t boxtype = 0;
char *title = "";
char *ok_label = 0;
char *cancel_label = 0;
char *extra_label = 0;
int clear_before = 0;
int clear_after = 0;

void usage ()
{
	fprintf (stderr, "%s version %s, %s\n", PACKAGE, VERSION, copyright);
	fprintf (stderr, "\n");
	fprintf (stderr, "Display dialog boxes from shell scripts.\n");
	fprintf (stderr, "\n");

	fprintf (stderr, "Usage:\n");
	fprintf (stderr, "\t%s [options] --infobox \"text\"\n", PACKAGE);
	fprintf (stderr, "\t%s [options] --msgbox \"text\"\n", PACKAGE);
	fprintf (stderr, "\t%s [options] --yesno \"text\"\n", PACKAGE);
	fprintf (stderr, "\t%s [options] --inputbox \"text\" \"value\"\n", PACKAGE);
	fprintf (stderr, "\t%s [options] --menu \"text\" \"variants\"\n", PACKAGE);
	fprintf (stderr, "\n");

	fprintf (stderr, "Options:\n");
	fprintf (stderr, "\t--title \"string\"\n");
	fprintf (stderr, "\t--ok-label \"string\"\n");
	fprintf (stderr, "\t--cancel-label \"string\"\n");
	fprintf (stderr, "\t--extra-label \"string\"\n");
	fprintf (stderr, "\t--clear-before\n");
	fprintf (stderr, "\t--clear-after\n");
	exit (-1);
}

void clear ()
{
	printf ("*** clear\n");
}

void infobox ()
{
	printf ("*** infobox\n");
	printf ("    title = '%s'\n", title);
	printf ("    ok = '%s'\n", ok_label);
	if (extra_label)
		printf ("    extra = '%s'\n", extra_label);
	printf ("    cancel = '%s'\n", cancel_label);
}

void msgbox ()
{
	printf ("*** msgbox\n");
	printf ("    title = '%s'\n", title);
	printf ("    ok = '%s'\n", ok_label);
	if (extra_label)
		printf ("    extra = '%s'\n", extra_label);
	printf ("    cancel = '%s'\n", cancel_label);
}

void yesno ()
{
	printf ("*** yesno\n");
	printf ("    title = '%s'\n", title);
	printf ("    ok = '%s'\n", ok_label);
	if (extra_label)
		printf ("    extra = '%s'\n", extra_label);
	printf ("    cancel = '%s'\n", cancel_label);
}

void inputbox (char *value)
{
	printf ("*** inputbox '%s'\n", value);
	printf ("    title = '%s'\n", title);
	printf ("    ok = '%s'\n", ok_label);
	if (extra_label)
		printf ("    extra = '%s'\n", extra_label);
	printf ("    cancel = '%s'\n", cancel_label);
}

void menu (char *values)
{
	printf ("*** menu '%s'\n", values);
	printf ("    title = '%s'\n", title);
	printf ("    ok = '%s'\n", ok_label);
	if (extra_label)
		printf ("    extra = '%s'\n", extra_label);
	printf ("    cancel = '%s'\n", cancel_label);
}

int main (int argc, char **argv)
{
	int c;

	for (;;) {
		c = getopt_long (argc, argv, "", longopts, 0);
		if (c < 0)
			break;
		switch (c) {
		case SHOWUSAGE:
			usage ();
			break;
		case SHOWVERSION:
			printf ("Version: %s\n", VERSION);
			return 0;
		case INFOBOX:
		case MSGBOX:
		case YESNO:
		case INPUTBOX:
		case MENU:
			boxtype = c;
			break;
		case TITLE:
			title = optarg;
			break;
		case OK_LABEL:
			ok_label = optarg;
			break;
		case CANCEL_LABEL:
			cancel_label = optarg;
			break;
		case EXTRA_LABEL:
			extra_label = optarg;
			break;
		case CLEAR_BEFORE:
			clear_before = 1;
			break;
		case CLEAR_AFTER:
			clear_after = 1;
			break;
		}
	}
	if (! boxtype)
		usage ();

	if (! ok_label)
		ok_label = (boxtype == YESNO) ? "Yes" : "Ok";
	if (! cancel_label)
		cancel_label = (boxtype == YESNO) ? "No" : "Cancel";

	if (clear_before)
		clear ();

	switch (boxtype) {
	case INFOBOX:
		infobox ();
		break;
	case MSGBOX:
		msgbox ();
		break;
	case YESNO:
		yesno ();
		break;
	case INPUTBOX:
		if (optind != argc-1)
			usage ();
		inputbox (argv[optind]);
		break;
	case MENU:
		if (optind != argc-1)
			usage ();
		menu (argv[optind]);
		break;
	default:
		usage ();
	}
	if (clear_after)
		clear ();
	return 0;
}
