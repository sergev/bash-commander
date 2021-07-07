/* commander.h -- File commander mode. */

/* Copyright (C) 2007 Serge Vakulenko <vak@cronyx.ru>

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

#ifndef _COMMANDER_H_
#define _COMMANDER_H_

#if defined (HAVE_SYS_STAT_H)
#   include <sys/stat.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _fileinfo {
  struct stat st;		/* stat() information */
  int link;			/* is it a symbolic link */
  int executable;		/* is it executable (for me) */
  int tagged;			/* tagged (selected) */
  char type;			/* file type symbol */
  char name [1];		/* file name, up to NAME_MAX+1 */
} FILEINFO;

typedef struct _dirinfo {
  char path [PATH_MAX+1];	/* directory full name */
  char *short_path;		/* short printable directory name */
  struct stat st;		/* stat() information */
  int top;			/* top file in the window */
  int current;			/* current file */
  int nallocated;		/* number of allocated entries in tab */
  int nfiles;			/* number of files in tab */
  int nregular;			/* number of regular files */
  int ntagged;			/* number of tagged files */
  off_t bytes;			/* number of bytes used */
  off_t tagged_bytes;		/* number of bytes tagged */
  int refresh;			/* refresh needed */
  int base_column;		/* left column for displaying the panel */
  FILEINFO *tab [1];		/* array of files */
} DIRINFO;

/* Enable and disable commander. */
extern int cmdr_init __P((char*));
extern void cmdr_disable __P((void));

/* Switch between visual mode and line mode of commander. */
extern void cmdr_activate __P((int active));

/* Reinitialize on change of TERM or LANG variables. */
void cmdr_reset_terminal __P((void));
void cmdr_reset_graphics __P((void));

void cmdr_term_goto __P((int,int));
void cmdr_term_graphics __P((int));
void cmdr_hor_line __P((int));
void cmdr_reset_graphics __P((void));
int cmdr_get_lines __P((void));
void cmdr_set_lines_and_columns __P((int, int));

#ifdef __cplusplus
}
#endif

#endif /* _COMMANDER_H_ */
