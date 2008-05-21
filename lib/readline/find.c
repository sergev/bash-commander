/* Copyright (C) 1987, 1989, 1992 Free Software Foundation, Inc.
 
   This file is part of the GNU Readline Library, a library for
   reading lines of text with interactive input and history editing.

   The GNU Readline Library is free software; you can redistribute it
   and/or modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2, or
   (at your option) any later version.

   The GNU Readline Library is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   The GNU General Public License is often shipped with GNU software, and
   is generally kept in a file called COPYING or LICENSE.  If you do not
   have a copy of the license, write to the Free Software Foundation,
   59 Temple Place, Suite 330, Boston, MA 02111 USA. */

/*
 * Find.c
 * 
 * Shaun Patterson
 * shaunpattersm@gmail.com
 * May 20th, 2008
 *
 * Based off "bash find" utility
 *
 * rl_find () is mapped to "graphical-search-history"
 *  See funmap.c for function map
 *
 *
 * Sample inputrc remap:
 *
 * "\C-f":graphical-search-history
 *
 */

#include <stdio.h>
#if defined (HAVE_TERMCAP_H)
#   include <termcap.h>
#endif
#if defined (COMMANDER)
#   include "commander.h"
#endif

#include "rldefs.h"
#include "readline.h"
#include "history.h"
#include "rlprivate.h"

#include "xmalloc.h"

/* List node */
typedef struct __rl_find_element
{
	char *str;

	int result_index;
	struct _rl_find_element *next;
	struct _rl_find_element *prev;
} _rl_find_element;

/* Find parameters */
typedef struct __rl_find_cxt 
{
	char *selected_string;
	char *final_prompt;
	
	_rl_find_element *result_list;
	int result_list_selection;
	int result_list_size;
} _rl_find_cxt;


/* Local variables */
int _rl_find_done = 0;    /* Find has finished */
_rl_find_cxt *cxt;        /* A place to store find parameters */

int line_x, line_y;       /* Current drawing location */


/* Initialization and Cleanup */
static _rl_find_cxt *_rl_find_init PARAMS((void));
static int _rl_find_fini PARAMS((_rl_find_cxt *));

/* Override keymap functions */
static Keymap _rl_find_remap PARAMS((Keymap));

/* Key handlers */
static int _rl_find_select_cmd PARAMS((int,int));
static int _rl_find_scroll_up PARAMS((int,int));
static int _rl_find_scroll_down PARAMS((int,int));
static int _rl_find_break PARAMS((int,int));

/* Display */
static void _rl_find_display_start PARAMS((void));
static void _rl_find_display_stop PARAMS((_rl_find_cxt *));
static void _rl_find_display_matches PARAMS((_rl_find_cxt *));

/* Result list */
static void _rl_find_get_matches PARAMS((_rl_find_cxt *));
static void _rl_find_reset_matches PARAMS((_rl_find_cxt *));
static _rl_find_element* _rl_find_results_add PARAMS((_rl_find_element*, char*));
static void _rl_find_results_remove_all PARAMS((_rl_find_element*));
static int _rl_find_results_check_dup PARAMS((_rl_find_element*, char*));

/* Graphics */

/* TO DO --- abstract all drawing code out of
 *  commander.c and into graphics util package */
static void enable_graphics PARAMS((int));
static void clear_screen PARAMS((void));
static void refresh_screen PARAMS((void));
static void start_drawing PARAMS((void));
static void stop_drawing PARAMS((void));
static void print_string PARAMS((char*));
static void print_highlighted_string PARAMS((char*));
static void new_line PARAMS((void));




/*
 * Find recent command
 */
int rl_find (nothing, key) 
  int nothing, key;
{
	int c;
	Keymap map;

	/* Initialize find context */
  cxt = _rl_find_init ();
	_rl_find_done = 0;


	/* Create a copy of the current keymap 
	 *  and override certain functions (up, down, enter, etc) */
	map = _rl_find_remap (rl_get_keymap ());

	/* Set up display */
	_rl_find_display_start ();

	while (_rl_find_done == 0) {

		/* Find the matches and display */
		_rl_find_get_matches (cxt);
		_rl_find_display_matches (cxt);

		/* Get next key */
		RL_SETSTATE(RL_STATE_MOREINPUT);
		c = rl_read_key ();
		RL_UNSETSTATE(RL_STATE_MOREINPUT);
  	
		
		/* Note, not sure of the best way to handle Ctrl+C 
		 *  On Ctrl+C, discard_keymap and find_fini
		 *  do not get called */
		/* TODO -- Ctrl+C does not caught. Why? Anybody? */
		_rl_dispatch (c, map);

  }
	_rl_find_display_stop (cxt);

	/* Free copy of the keymap */
	rl_discard_keymap (map);

  return (_rl_find_fini (cxt));
}




/*---------------------------------------------------
 * Initialization and Cleanup
 *--------------------------------------------------*/

/* Set up a find parameters */
static _rl_find_cxt *_rl_find_init () {
  _rl_find_cxt *cxt;
  
  cxt = (_rl_find_cxt *)xmalloc (sizeof (_rl_find_cxt));
 

	/* Set up result list */
	cxt->result_list = NULL;
	cxt->result_list_size = 0;
	cxt->result_list_selection = 0;
	cxt->selected_string = NULL;

  return cxt;
}

/* Cleanup */
static int _rl_find_fini (cxt) 
  _rl_find_cxt *cxt;
{
	/* Reset & free matches */
	_rl_find_reset_matches (cxt);  

	/* Free final prompt */
	FREE (cxt->final_prompt);

	return 1;
}




/*
 * Remap the following key-function to custom functions:
 *
 * rl_get_previous_history ==> _rl_find_scroll_up
 * rl_get_next_history     ==> _rl_find_scroll_down
 * rl_newline              ==> _rl_find_select_cmd
 *
 * Note: Recursively creates and remaps all sub-keymaps.  
 *  rl_discard_key is used to free all of these 
 */
static Keymap _rl_find_remap (original_map) 
	Keymap original_map;
{
	Keymap map;
	int i = 0;

	/* Create a copy of the keymap */
	map = rl_copy_keymap (original_map);

	/* Lookup rl_get_previous_history */
	for (i = 0; i < KEYMAP_SIZE; i++) {

		if (map [i].type == ISKMAP) {
			/* Found a key map, remap keys here too */
			map [i].function = KEYMAP_TO_FUNCTION (_rl_find_remap (FUNCTION_TO_KEYMAP (map, i)));
		} 
		else if(map [i].type == ISFUNC) {

			/* Remap functions */
			if (map [i].function == rl_get_previous_history) {
				rl_bind_key_in_map (i, _rl_find_scroll_up, map);
			} 
			else if (map [i].function == rl_get_next_history) {
				rl_bind_key_in_map (i, _rl_find_scroll_down, map);
			} 
			else if (map [i].function == rl_newline) 
			{
				rl_bind_key_in_map (i, _rl_find_select_cmd, map);
			}
			else if (map [i].function == rl_complete) {
				/* Make a more robust tab completion for this? */
			}
			else if (map [i].function == rl_possible_completions) {
				/* Override the complete function perhaps? */
			}
			/* TODO: I thought this would capture Ctrl+C... */
			else if (map [i].function == (rl_command_func_t *)0x0) {
				rl_bind_key_in_map (i, _rl_find_break, map);
			}
			else {
			}

		}
		else {
			/* Macro */
			/* Not sure about this yet */
		}
	}

	return map;
}



/*---------------------------------------------------
 * Key functions
 *--------------------------------------------------*/

/* Select the selected command */
static int _rl_find_select_cmd (ignored, key) 
	int ignored;
	int key;
{
	if (cxt->selected_string) {
		/* Set prompt to current selection */
		cxt->final_prompt = strdup (cxt->selected_string);
	} else {
		/* No search selected/found, make prompt current search */
		cxt->final_prompt = strdup (rl_line_buffer);
	}

	/* Set exit flag */
	_rl_find_done = 1;

	return key;
}

/* Scroll up */
static int _rl_find_scroll_up (ignored, key) 
	int ignored;
	int key;
{
	cxt->result_list_selection--;

	/* Wrap around */
	if (cxt->result_list_selection < 0) {
		cxt->result_list_selection = cxt->result_list_size;
	}

	return key;
}

/* Scroll down */
static int _rl_find_scroll_down (ignored, key)
	int ignored;
	int key;
{
	cxt->result_list_selection++;

	/* Wrap around */
	if (cxt->result_list_selection > cxt->result_list_size) {
		cxt->result_list_selection = 0;
	}

	return key;
}

/* Ctrl+C'd */
/* TODO: Not working... */
static int _rl_find_break (ignored, key)
	int ignored;
	int key;
{
	_rl_find_done = 1;

	return key;
}


/*---------------------------------------------------
 * Display 
 *--------------------------------------------------*/
static void _rl_find_display_start () 
{
	enable_graphics (1);
	clear_screen ();
	refresh_screen ();
}

static void _rl_find_display_stop (cxt) 
	_rl_find_cxt *cxt;
{
	enable_graphics (0);

	/* Set the prompt to the selected line */
	rl_replace_line ("", 0);
	rl_insert_text (cxt->final_prompt);
	rl_refresh_line (0,0);
}


/* Display all search matches */
static void _rl_find_display_matches (cxt)
	_rl_find_cxt *cxt;
{
	_rl_find_element *current_element = cxt->result_list;

	start_drawing ();
	clear_screen ();
	print_string ("Find: ");
	print_string (rl_line_buffer);
	new_line ();

	/* Loop through the result list */
	current_element = cxt->result_list;
	while (current_element != NULL) {

		/* Highlight current selection */
		if (current_element->result_index == cxt->result_list_selection) {
			print_highlighted_string (current_element->str);
			cxt->selected_string = current_element->str;
		} else {
			print_string (current_element->str);
		}

		current_element = (_rl_find_element *)current_element->next;

		/* Skip to next line */
		new_line ();
	}
	refresh_screen ();

	stop_drawing ();
}




/*---------------------------------------------------
 * History and Result List Functions
 *--------------------------------------------------*/

/*
 * Get matches from the history list
 * 
 * Use a simple linked list to prevent adding
 *  duplicates
 */
static void _rl_find_get_matches (cxt) 
	_rl_find_cxt *cxt;
{
	/* History */
	HISTORY_STATE *hstate = NULL;
	HIST_ENTRY **hlist = NULL;
	int history_pos = 0;

	/* Search string */
	char *search_string = rl_line_buffer;

	/* Screen height (in lines) */
	int screen_height = cmdr_get_lines ();
	
	
	/* Free any old matches and start over */
	_rl_find_reset_matches (cxt);

	/* Get history information and start from
	 *  end of the history to the start */
	hlist = history_list ();
	hstate = history_get_history_state ();
	history_pos = hstate->length-1;

	/* Find as many results as the screen can show */
	while (hlist[history_pos] && history_pos > 0 && cxt->result_list_size < screen_height - 3) {
		char *current_line = hlist[history_pos]->line;

		/* Match to the whole search line */
		if (STREQN (search_string, current_line, strlen (search_string))) {
			/* Line match found */

			/* Do not add duplicate lines into the result list */
			if (_rl_find_results_check_dup (cxt->result_list, current_line) == 0) {
				/* Add increases result_list_size by 1 */
				cxt->result_list = _rl_find_results_add (cxt->result_list, strdup (current_line));
			}
		}
		history_pos--;
	
	}

	/* Only reset the selection if size decreased below */
	if (cxt->result_list_selection > cxt->result_list_size) {
		cxt->result_list_selection = cxt->result_list_size;
	}	
	
}

/* Clear out any matches in the result list */
static void _rl_find_reset_matches (cxt) 
	_rl_find_cxt *cxt;
{
	/* Free list */
	_rl_find_results_remove_all (cxt->result_list);

	/* Reset selection info */
	cxt->result_list_size = 0;
	cxt->selected_string = NULL;
}



/* Add element */
static _rl_find_element * _rl_find_results_add (list, result) 
	_rl_find_element *list;
	char *result; 
{
	_rl_find_element *current_element = list;

	if (current_element != NULL) {
		_rl_find_element *prev = NULL;

		/* Find end */
		while (current_element->next != NULL)
			current_element = (_rl_find_element *)current_element->next;
		
		/* Create new node */
		current_element->next = (struct _rl_find_element *)xmalloc (sizeof (_rl_find_element));

		/* Set up position is list */
		prev = current_element;
		current_element = (_rl_find_element *)current_element->next;
		current_element->next = NULL;
		current_element->prev = (struct _rl_find_element *)prev;

		/* Set data */
		current_element->str = result;
		current_element->result_index = prev->result_index + 1;
		
	
		cxt->result_list_size++;

		return list;

	}	else {
		/* New list */

		current_element = (_rl_find_element *)xmalloc (sizeof (_rl_find_element));
		current_element->next = NULL;
		current_element->prev = NULL;
		current_element->str = result;
		current_element->result_index = 0;

		return current_element;
	}
}

static void _rl_find_results_remove_all (list)
	_rl_find_element *list;
{
	_rl_find_element *current_element = list;

	if (current_element == NULL) {
		return;
	}

	_rl_find_element *tempp;
	while (current_element->next != NULL) {
		tempp = (_rl_find_element *)current_element->next;
		free (current_element->str);
		free (current_element);
		current_element = tempp;
	}

	cxt->result_list = NULL;
	cxt->result_list_size = 0;
}

/* Check for to see if result is already in the list */
static int _rl_find_results_check_dup (list, result)
	_rl_find_element *list;
	char *result;
{
	_rl_find_element *temp = list;
		
	if (temp == NULL) {
		return 0;
	}

	/* Found match on first item */
	if (strcmp (temp->str, result) == 0) {
		return 1;
	}

	while (temp->next != NULL) {
		if (strcmp (temp->str, result) == 0) {
			return 1;
		}
		temp = (_rl_find_element *)temp->next;
	}

	return 0;
}


/*---------------------------------------------------
 * Graphics
 * TODO: Abstract drawing code out of commander.c
 *--------------------------------------------------*/

static void enable_graphics (enable) 
	int enable;
{
	if (enable) {
		cmdr_reset_terminal ();
		cmdr_set_visual_mode (enable);
	} else {
		cmdr_set_visual_mode (enable);
	}
}

extern char *cmdr_term_clear;

static void clear_screen () 
{
	fputs (cmdr_term_clear, rl_outstream);
	fflush (rl_outstream);

	line_x = 0;
	line_y = 0;
}

static void refresh_screen () {
	fflush (rl_outstream);
}

static void start_drawing () {
	cmdr_term_graphics (1);
}

static void stop_drawing () {
	cmdr_term_graphics (0);
}


extern char *cmdr_color_reverse;
extern char *cmdr_color_normal;

static void print_string (line) 
	char *line;
{
	fputs (line, rl_outstream);
}

static void print_highlighted_string (line)
	char *line;
{
	fputs (cmdr_color_reverse, rl_outstream);
	fputs (line, rl_outstream);
	fputs (cmdr_color_normal, rl_outstream);
}

static void new_line () {
	line_y++;
	cmdr_term_goto (line_y, line_x);
}

