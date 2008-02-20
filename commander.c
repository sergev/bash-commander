/*
 * Readline commander: embedded file browser.
 * Copyright (C) 2007 Serge Vakulenko <vak@cronyx.ru>
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
#if defined (HAVE_CONFIG_H)
#include <config.h>
#endif

#include <stdio.h>

#if defined (HAVE_UNISTD_H)
#   include <unistd.h>
#endif
#if defined (HAVE_ERRNO_H)
#   include <errno.h>
#endif
#if defined (HAVE_DIRENT_H)
#   include <dirent.h>
#endif
#if defined (HAVE_TERMCAP_H)
#  include <termcap.h>
#else
char *tgoto(const char *cap, int col, int row);
#endif

#include "shell.h"
#include "commander.h"
#include "execute_cmd.h"
#include "builtins/common.h"
#include <readline/readline.h>

/* The visible cursor position. */
extern int _rl_last_c_pos, _rl_last_v_pos;

/* Number of lines currently on screen minus 1. */
extern int _rl_vis_botlin;

Keymap cmdr_line_keymap, cmdr_visual_keymap;
int cmdr_visual_mode;
int cmdr_lines, cmdr_columns;

char *cmdr_term_ku, *cmdr_term_kd, *cmdr_term_kr, *cmdr_term_kl;
char *cmdr_term_kh, *cmdr_term_at7, *cmdr_term_kP, *cmdr_term_kN;
char *cmdr_term_kI;
char *cmdr_term_cm, *cmdr_term_ac, *cmdr_term_as, *cmdr_term_ae;
char *cmdr_term_me, *cmdr_term_mr, *cmdr_term_md;
char *cmdr_term_clear, *cmdr_term_clreol;

char *cmdr_term_kf1, *cmdr_term_kf2, *cmdr_term_kf3, *cmdr_term_kf4;
char *cmdr_term_kf5, *cmdr_term_kf6, *cmdr_term_kf7, *cmdr_term_kf8;
char *cmdr_term_kf9, *cmdr_term_kf10, *cmdr_term_kf11, *cmdr_term_kf12;
char *cmdr_term_kf13, *cmdr_term_kf14, *cmdr_term_kf15, *cmdr_term_kf16;
char *cmdr_term_kf17, *cmdr_term_kf18, *cmdr_term_kf19, *cmdr_term_kf20;

char cmdr_term_vertline, cmdr_term_horline;
char cmdr_term_corner[9];
int cmdr_term_utf8;

/* Color escape sequences. */
char *cmdr_color_normal, *cmdr_color_reverse, *cmdr_color_bold;
char *cmdr_color_bold_reverse, *cmdr_color_dim;

/* Show hidden files (dot names). */
int cmdr_show_hidden;

/* Nonzero, when some text is saved in kill ring. */
int cmdr_saved_line;

/* Original bash function, mapped to Tab key. */
rl_command_func_t *cmdr_original_tab_func;

/* Left and right directory panels. */
DIRINFO *cmdr_panel [2];
int cmdr_current_panel;

void cmdr_set_visual_mode __P((int enable_visual));

/*
 * Count a number of characters in multibyte string.
 */
static int
mbstrlen (str)
    const char *str;
{
  int mbytes, chars;

  chars = 0;
  for (;;)
    {
      mbytes = mblen (str, MB_CUR_MAX);
      if (mbytes <= 0)
	break;
      str += mbytes;
      ++chars;
    }
  return chars;
}

/*
 * Parse parameter string up to ':', unescaping characters.
 * Return an allocated copy.
 */
static char *
parse_string (strp)
    char **strp;
{
  char *copy, *p;

  p = strchr (*strp, ':');
  if (p)
    copy = xmalloc (p - *strp + 1);
  else
    copy = xmalloc (strlen (*strp) + 1);

  for (p=copy; **strp && **strp != ':'; ++*strp)
    {
      if (**strp == '\\')
	{
	  ++*strp;
	  switch (**strp)
	    {
	    case '0': case '1': case '2': case '3':
	    case '4': case '5': case '6': case '7':
	      /* Octal sequence */
	      *p = **strp - '0';
	      if ((*strp)[1] >= '0' && (*strp)[1] <= '7')
		{
		  ++*strp;
		  *p = (*p << 3) + **strp - '0';
		  if ((*strp)[1] >= '0' && (*strp)[1] <= '7')
		    {
		      ++*strp;
		      *p = (*p << 3) + **strp - '0';
		    }
		}
	      ++p;
	      break;
	    case 'a':		/* Bell */
	      *p++ = '\a';
	      break;
	    case 'b':		/* Backspace */
	      *p++ = '\b';
	      break;
	    case 'e':		/* Escape */
	      *p++ = 27;
	      break;
	    case 'f':		/* Form feed */
	      *p++ = '\f';
	      break;
	    case 'n':		/* Newline */
	      *p++ = '\n';
	      break;
	    case 'r':		/* Carriage return */
	      *p++ = '\r';
	      break;
	    case 't':		/* Tab */
	      *p++ = '\t';
	      break;
	    case 'v':		/* Vtab */
	      *p++ = '\v';
	      break;
	    case '?':		/* Delete */
              *p++ = 127;
	      break;
	    case '_':		/* Space */
	      *p++ = ' ';
	      break;
	    case '\0':		/* End of string */
	      --*strp;
	      break;
	    default:		/* Escaped character like \ ^ : = */
	      *p++ = **strp;
	      break;
	    }
	}
      else
	*p++ = **strp;
    }
  *p = 0;
  return copy;
}

/*
 * Parse parameter string and fetch color escape sequences:
 * no (normal) - for file names and lengths
 * nr (normal reverse) - for cursor of file panel
 * bo (bold) - for selected (tagged) file names
 * br (bold reverse) - for cursor on selected (tagged) file name
 * di (dim) - for graphics and directory names
 *
 * Default:
 *    no=\33[m:nr=\33[7m:bo=\33[1m:br=\33[1m\33[7m
 *
 * Recommended for white background:
 *    no=\33[m:nr=\33[0;30;46m:bo=\33[0;33m:br=\33[0;33;46m:di=\33[0;36m
 *
 * Recommended for black background:
 *    no=\33[m:nr=\33[0;37;43m:bo=\33[1;37m:br=\33[1;37;43m:di=\33[0;36m
 */
static void
parse_colors (char *param)
{
  int label;
  char *next;

  /* Default normal = termcap "me". */
  if (cmdr_color_normal)
    free (cmdr_color_normal);
  cmdr_color_normal = strdup (cmdr_term_me ? cmdr_term_me : "");

  /* Default reverse = termcap "mr". */
  if (cmdr_color_reverse)
    free (cmdr_color_reverse);
  cmdr_color_reverse = strdup (cmdr_term_mr ? cmdr_term_mr : "");

  /* Default bold = termcap "md". */
  if (cmdr_color_bold)
    free (cmdr_color_bold);
  cmdr_color_bold = strdup (cmdr_term_md ? cmdr_term_md : "");

  /* Default bold reverse = termcap "md"+"mr". */
  if (cmdr_color_bold_reverse)
    free (cmdr_color_bold_reverse);
  cmdr_color_bold_reverse = xmalloc (strlen (cmdr_color_bold) +
				     strlen (cmdr_color_reverse) + 1);
  strcpy (cmdr_color_bold_reverse, cmdr_color_bold);
  strcat (cmdr_color_bold_reverse, cmdr_color_reverse);

  /* Default dim is empty. */
  if (cmdr_color_dim)
    free (cmdr_color_dim);
  cmdr_color_dim = strdup ("");

  /* Parse parameter string. */
  label = 0;
  while (*param)
    {
      if (*param == ':')
	{
	  label = 0;
	  ++param;
	}
      else if (! label)
	{
	  /* First label character. */
	  label = (unsigned char) *param++;
	}
      else if (label < 256)
	{
	  /* Second label character. */
	  label |= (unsigned char) *param++ << 8;
	}
      else
	{
	  /* Equal sign after label. */
	  if (*param++ == '=')
	    switch (label)
	      {
	      case 'n'|'o'<<8:
		free (cmdr_color_normal);
		cmdr_color_normal = parse_string (&param);
		break;
	      case 'n'|'r'<<8:
		free (cmdr_color_reverse);
		cmdr_color_reverse = parse_string (&param);
		break;
	      case 'b'|'o'<<8:
		free (cmdr_color_bold);
		cmdr_color_bold = parse_string (&param);
		break;
	      case 'b'|'r'<<8:
		free (cmdr_color_bold_reverse);
		cmdr_color_bold_reverse = parse_string (&param);
		break;
	      case 'd'|'i'<<8:
		free (cmdr_color_dim);
		cmdr_color_dim = parse_string (&param);
		break;
	      }
	  else
	    {
	      /* Incorrect syntax. */
	      next = strchr (param, ':');
	      param = next ? next : param + strlen (param);
	    }
	  label = 0;
	}
    }
}

/*
 * Move cursor to given line and column.
 */
void
cmdr_term_goto (line, col)
    int line, col;
{
  char *str;

  str = tgoto (cmdr_term_cm, col, line);
  if (str)
    fputs (str, rl_outstream);
}

/*
 * Enable or disable graphics mode of terminal.
 * No action in UTF-8 mode.
 */
void
cmdr_term_graphics (enable)
    int enable;
{
  char *str;

  if (cmdr_term_utf8)
    return;
  str = enable ? cmdr_term_as : cmdr_term_ae;
  if (str)
    fputs (str, rl_outstream);
}

/*
 * Display a horizontal graphics line of given length.
 * A terminal must be in graphics mode.
 */
void
cmdr_hor_line (len)
    int len;
{
  int i;

  for (i=0; i<len; ++i)
    {
      if (cmdr_term_utf8)
	{
	  putc (0xe2, rl_outstream);
	  putc (0x94, rl_outstream);
	}
      putc (cmdr_term_horline, rl_outstream);
    }
}

/*
 * Display a vertical graphics line of length 1.
 * A terminal must be in graphics mode.
 */
void
cmdr_vert_line ()
{
  if (cmdr_term_utf8)
    {
      putc (0xe2, rl_outstream);
      putc (0x94, rl_outstream);
    }
  putc (cmdr_term_vertline, rl_outstream);
}

/*
 * Display a graphics corner by number 1..9.
 * A terminal must be in graphics mode.
 * Corners are numbered in order of keypad keys:
 *               7 8 9 <- upper right
 *               4 5 6
 * lower left -> 1 2 3
 */
void
cmdr_corner (num)
    int num;
{
  if (cmdr_term_utf8)
    {
      putc (0xe2, rl_outstream);
      putc (0x94, rl_outstream);
    }
  putc (cmdr_term_corner [num-1], rl_outstream);
}

/*
 * Prepare terminal data for drawing graphics.
 * This must be done after TERM or LANG variables changed.
 */
void
cmdr_reset_graphics ()
{
  char *lang, *p;

/*fprintf (stderr, "cmdr_reset_graphics() called\r\n");*/
  cmdr_term_utf8 = 0;
  cmdr_term_vertline = '|';
  cmdr_term_horline = '-';
  memcpy (cmdr_term_corner, "+++++++++", 9);

  p = cmdr_term_ac;
  if (! p)
    return;

  lang = get_locale_var ("LANG");
/*fprintf (stderr, "LANG = '%s'\r\n", lang);*/
  if (lang && (strstr (lang, "utf8") || strstr (lang, "utf-8") ||
      strstr (lang, "UTF8") || strstr (lang, "UTF-8")))
    {
      cmdr_term_utf8 = 1;
      p = "q\200x\202m\224v\264j\230t\234n\274u\244l\214w\254k\220";
    }

  for (; *p; p += 2)
    switch (*p)
      {
      case 'q': cmdr_term_horline   = p[1]; break; /* horizontal line */
      case 'x': cmdr_term_vertline  = p[1]; break; /* vertical line */
      case 'm': cmdr_term_corner[0] = p[1]; break; /* lower left corner */
      case 'v': cmdr_term_corner[1] = p[1]; break; /* tee pointing up */
      case 'j': cmdr_term_corner[2] = p[1]; break; /* lower right corner */
      case 't': cmdr_term_corner[3] = p[1]; break; /* tee pointing right */
      case 'n': cmdr_term_corner[4] = p[1]; break; /* large plus or crossover */
      case 'u': cmdr_term_corner[5] = p[1]; break; /* tee pointing left */
      case 'l': cmdr_term_corner[6] = p[1]; break; /* upper left corner */
      case 'w': cmdr_term_corner[7] = p[1]; break; /* tee pointing down */
      case 'k': cmdr_term_corner[8] = p[1]; break; /* upper right corner */
      }
}

/*
 * Return integer code, used for comparing file types.
 * Directories first: dot-dot, then dot, then others.
 * Special files follow, regular files at end.
 */
static int
compare_file_level (fi)
    FILEINFO *fi;
{
  switch (fi->st.st_mode & S_IFMT)
    {
    case S_IFDIR:
      if (fi->name[0] == '.')
	{
	  if (fi->name[1] == '.' && ! fi->name[2])
	    return 0;
	  if (! fi->name[1])
	    return 1;
	}
      return 2;
    case S_IFCHR:  return 10;
    case S_IFBLK:  return 20;
#ifdef S_IFIFO
    case S_IFIFO:  return 30;
#endif
#ifdef S_IFSOCK
    case S_IFSOCK: return 40;
#endif
#ifdef S_IFLNK
    case S_IFLNK:  return 50;
#endif
    case S_IFREG:  return 90;
    default:	   return 80;
    }
}

/*
 * Compare files. Used in call to qsort.
 * Return -1, 0, 1 iff a is less, equal or greater than b.
 */
int
cmdr_compare_files (arg1, arg2)
    const void *arg1, *arg2;
{
  FILEINFO *a = *(FILEINFO**) arg1;
  FILEINFO *b = *(FILEINFO**) arg2;
  int alevel, blevel;

  /* Compare file types. */
  alevel = compare_file_level (a);
  blevel = compare_file_level (b);
  if (alevel != blevel)
	  return alevel - blevel;

  /* Hidden files first. */
  alevel = (a->name[0] != '.');
  blevel = (b->name[0] != '.');
  if (alevel != blevel)
	  return alevel - blevel;

  if (! S_ISDIR (a->st.st_mode))
    {
      /* Files by extension. */
      char *aext, *bext;
      int cmp;

      aext = strrchr (a->name + 1, '.');
      bext = strrchr (b->name + 1, '.');
      if (aext)
	{
	  if (! bext)
	    return 1;
	  cmp = strcmp (aext, bext);
	  if (cmp)
	    return cmp;
	}
      else if (bext)
	return -1;
    }

  /* Files or directories by name. */
  return strcmp (a->name, b->name);
}

void
cmdr_free_directory (dir)
    DIRINFO *dir;
{
  int i;

  for (i=0; i<dir->nfiles; ++i)
    free (dir->tab [i]);
  free (dir);
}

/*
 * Set top file by dev and inode number.
 */
void
cmdr_restore_top (dir, st)
    DIRINFO *dir;
    struct stat *st;
{
  int i;

  for (i=0; i<dir->nfiles; ++i)
    if (dir->tab[i]->st.st_dev == st->st_dev &&
	dir->tab[i]->st.st_ino == st->st_ino)
      {
	dir->top = dir->current = i;
	break;
      }
}

/*
 * Set current file by dev and inode number.
 */
void
cmdr_restore_current (dir, st)
    DIRINFO *dir;
    struct stat *st;
{
  int i;

  for (i=0; i<dir->nfiles; ++i)
    if (dir->tab[i]->st.st_dev == st->st_dev &&
	dir->tab[i]->st.st_ino == st->st_ino)
      {
	dir->current = i;
	break;
      }
}

/*
 * Restore file tags.
 */
void
cmdr_restore_tagged (dir, olddir)
    DIRINFO *dir, *olddir;
{
  FILEINFO **fip;
  int i;

  for (i=0; i<olddir->nfiles; ++i)
    if (olddir->tab[i]->tagged)
      {
	fip = (FILEINFO**) bsearch (&olddir->tab[i], dir->tab, dir->nfiles,
		       sizeof(FILEINFO*), cmdr_compare_files);
	if (fip && ! (*fip)->tagged)
	  {
	    (*fip)->tagged = 1;
	    ++dir->ntagged;
	    if (S_ISREG ((*fip)->st.st_mode))
	      dir->tagged_bytes += (*fip)->st.st_size;
	  }
      }
}

/*
 * When all files have mode rwxrwxrwx, then probably this is
 * a mounted DOS volume. Clear executable flag.
 */
void
cmdr_fix_dos_volume (dir)
    DIRINFO *dir;
{
  int i, mode;

  /* Skip dotdot. */
  for (i=1; i<dir->nfiles; ++i)
    {
      /* Files on a DOS volume have modes 777 or 700. */
      mode = dir->tab[i]->st.st_mode & 0777;
      if (mode != 0777 && mode != 0700)
	{
	return;
	}
    }

  /* DOS volume - clear executable flag. */
  for (i=1; i<dir->nfiles; ++i)
    if (S_ISREG (dir->tab[i]->st.st_mode))
      {
	dir->tab[i]->executable = 0;
	dir->tab[i]->type = ' ';
      }
}

/*
 * Read directory and return newly allocated DIRINFO structure.
 * When path is null, use name of olddir.
 * When olddir is not null, use it's contents for tagging files,
 * and try to restore top and current file pointers.
 * Deallocate olddir after use.
 */
DIRINFO *cmdr_read_directory (path, olddir, oldcur)
    char *path;
    DIRINFO *olddir;
    struct stat *oldcur;
{
  DIRINFO *dir;
  DIR *dirfd;
  struct dirent *de;
  FILEINFO *fi;
  char filepath [PATH_MAX+1], *home;
  int panel_height, i;

  dir = (DIRINFO*)xmalloc (sizeof (DIRINFO) + sizeof (FILEINFO*) * 100);
  memset (dir, 0, sizeof (DIRINFO));
  dir->nallocated = 100;

  home = get_string_value ("HOME");
  if (! home)
    home = "/";

  if (path)
    {
      if (access (path, R_OK) < 0)
	path = home;
      strcpy (dir->path, path);
    }
  else if (! getcwd (dir->path, sizeof (dir->path)))
    {
      chdir (home);
      strcpy (dir->path, home);
    }
  stat (dir->path, &dir->st);

  dirfd = opendir (dir->path);
  if (! dirfd)
    {
      int err = errno;

      /* Return to line mode. */
      cmdr_set_visual_mode (0);
      fputs (cmdr_term_clear, rl_outstream);
      cmdr_term_goto (cmdr_lines - _rl_vis_botlin - 2, 0);

      /* Print error and jump to bash command loop. */
      fprintf (rl_outstream, "%s: %s\r\n", dir->path, strerror (err));
      free (dir);
      jump_to_top_level (DISCARD);
    }

  while ((de = readdir (dirfd)))
    {
      if (! de->d_ino)
	continue;

      /* Skip dot entry. */
      if (de->d_name[0] == '.' && ! de->d_name[1])
	continue;

      /* Skip all hidden files except parent dir entry */
      if (! cmdr_show_hidden && de->d_name[0] == '.' &&
	  (de->d_name[1] != '.' || de->d_name[2]))
	continue;

      /* Allocate fileinfo structure. */
      fi = xmalloc (sizeof (*fi) + strlen (de->d_name));
      memset (fi, 0, sizeof (*fi));

      /* Get stat information. */
      strcpy (filepath, dir->path);
      i = strlen (filepath);
      if (i > 0 && filepath[i-1] != '/')
        strcat (filepath, "/");
      strcat (filepath, de->d_name);
      if (lstat (filepath, &fi->st) < 0)
	{
	  free (fi);
	  continue;
	}
      if (S_ISLNK (fi->st.st_mode))
	{
	  fi->link = 1;
          stat (filepath, &fi->st);
	}

      /* Skip .. in root directory */
      if (de->d_name[0] == '.' && de->d_name[1] == '.' &&
	  de->d_name[2] == 0 && fi->st.st_ino == dir->st.st_ino &&
	  fi->st.st_dev == dir->st.st_dev)
	{
	  free (fi);
	  continue;
	}

      strcpy (fi->name, de->d_name);
      if (S_ISREG (fi->st.st_mode))
	{
	  ++dir->nregular;
	  dir->bytes += fi->st.st_size;
	}
      if ((fi->st.st_mode & 0111) && access (filepath, X_OK) >= 0)
	fi->executable = 1;

      switch (fi->st.st_mode & S_IFMT)
	{
	case S_IFREG:  fi->type = fi->executable ? '*' : ' '; break;
	case S_IFDIR:  fi->type = fi->executable ? '/' : '&'; break;
	case S_IFCHR:  fi->type = '$'; break;
	case S_IFBLK:  fi->type = '#'; break;
#ifdef S_IFIFO
	case S_IFIFO:  fi->type = '='; break;
#endif
#ifdef S_IFSOCK
	case S_IFSOCK: fi->type = '!'; break;
#endif
#ifdef S_IFLNK
	case S_IFLNK:  fi->type = '~'; break;
#endif
	default:       fi->type = '?'; break;
	}

      /* Put file info into directory list. */
      if (dir->nfiles >= dir->nallocated)
	{
	  dir->nallocated += dir->nallocated;
	  dir = (DIRINFO*)xrealloc (dir, sizeof (DIRINFO) +
				    sizeof(FILEINFO*) * dir->nallocated);
	}
      dir->tab [dir->nfiles++] = fi;
    }
  closedir (dirfd);

  /* Sort: directories first, regular files by extension. */
  qsort (dir->tab, dir->nfiles, sizeof(FILEINFO*), cmdr_compare_files);

  /* Restore top and current files */
  if (olddir)
    {
      dir->base_column = olddir->base_column;
      if (strcmp (dir->path, olddir->path) == 0)
	{
    	  cmdr_restore_top (dir, &olddir->tab[olddir->top]->st);
	  cmdr_restore_current (dir, &olddir->tab[olddir->current]->st);
	}
      cmdr_restore_tagged (dir, olddir);
      cmdr_free_directory (olddir);
    }
  else if (oldcur)
    cmdr_restore_current (dir, oldcur);

  if (dir->top > dir->current)
    dir->top = dir->current;

  panel_height = cmdr_lines - _rl_vis_botlin - 4;
  if (dir->top + panel_height <= dir->current)
    dir->top = dir->current - panel_height + 1;

  /* Compute tagged files and bytes */
  for (i=0; i<dir->nfiles; ++i)
    if (dir->tab[i]->tagged)
      {
	++dir->ntagged;
	if (S_ISREG (dir->tab[i]->st.st_mode))
	  dir->tagged_bytes += dir->tab[i]->st.st_size;
      }

  cmdr_fix_dos_volume (dir);
  return dir;
}

void
cmdr_print_file_size (buf, bytes)
    char *buf;
    off_t bytes;
{
  off_t kbytes;
  long mbytes, gbytes;

  if (bytes <= 99999)
    {
      sprintf (buf, "%ld", (long) bytes);
      return;
    }
  kbytes = bytes >> 10;
  if (kbytes <= 999)
    {
      sprintf (buf, "%ld k", (long) kbytes);
      return;
    }
  mbytes = kbytes >> 10;
  if (mbytes <= 9)
    {
      sprintf (buf, "%ld.%ld M", mbytes,
	(long) ((kbytes*10) >> 10) % 10);
      return;
    }
  if (mbytes <= 999)
    {
      sprintf (buf, "%ld M", mbytes);
      return;
    }
  gbytes = mbytes >> 10;
  if (gbytes <= 9)
    {
      sprintf (buf, "%ld.%ld G", gbytes, ((mbytes*10) >> 10) % 10);
      return;
    }
  sprintf (buf, "%ld G", gbytes);
}

void
cmdr_draw_file (fi, line, column)
    FILEINFO *fi;
    int line, column;
{
  char *extension, *p;
  int panel_width, ext_len, max_name_len, filename_space;

  if (line)
    cmdr_term_goto (line, column);
  if (fi->type != ' ' || fi->link || fi->tagged)
    {
      if (line)
    	fputs (cmdr_color_dim, rl_outstream);
      fputc (fi->type, rl_outstream);
      if (fi->tagged)
	{
	  fputc ('>', rl_outstream);
    	  if (line)
	    fputs (cmdr_color_bold, rl_outstream);
	}
      else
	{
	  fputc (fi->link ? '@' : ' ', rl_outstream);
    	  if (line)
  	    fputs (cmdr_color_normal, rl_outstream);
	}
    }
  else
    {
      if (line)
  	fputs (cmdr_color_normal, rl_outstream);
      fputs ("  ", rl_outstream);
    }

  /* Right align extension (not for directories). */
  panel_width = cmdr_columns / 2 - 3;
  max_name_len = panel_width - 3 - 8;
  if (S_ISDIR (fi->st.st_mode))
    extension = 0;
  else
    {
      extension = strrchr (fi->name + 1, '.');
      if (extension)
	{
	  ext_len = mbstrlen (extension);
	  if (max_name_len - ext_len >= 4)
	    max_name_len -= ext_len;
	  else
	    extension = 0;
	}
    }
  if (! extension)
    extension = fi->name + strlen (fi->name);

  /* Print file name. */
  filename_space = max_name_len;
  p = fi->name;
  while (p < extension)
    {
#if defined (HANDLE_MULTIBYTE)
      int mbytes;
      wchar_t c;
      char buf [MB_CUR_MAX+1];

      mbytes = mbtowc (&c, p, MB_CUR_MAX);
      if (mbytes <= 0)
	break;
      p += mbytes;
      mbytes = wctomb (buf, c);
      if (mbytes <= 0)
	break;
      buf[mbytes] = 0;
#else
      char c;

      c = *p++;
#endif
      if (--filename_space == 0 && p < extension)
	{
	  /* Add trailing tilde when not enough space. */
	  fputc ('~', rl_outstream);
	  break;
	}
#if defined (HANDLE_MULTIBYTE)
      fputs (buf, rl_outstream);
#else
      fputc (c, rl_outstream);
#endif
    }
  while (filename_space-- > 0)
    fputc (' ', rl_outstream);

  if (*extension)
    fputs (extension, rl_outstream);

  /* Print file size or type. */
  fputc (' ', rl_outstream);
  if (S_ISREG (fi->st.st_mode))
    {
      char buf [8];

      cmdr_print_file_size (buf, fi->st.st_size);
      fprintf (rl_outstream, "%6s", buf);
    }
  else
    {
      if (line)
	fputs (cmdr_color_dim, rl_outstream);
      switch (fi->st.st_mode & S_IFMT)
	{
	case S_IFDIR:  fputs (" <DIR>", rl_outstream); break;
	case S_IFCHR:  fputs (" <CHR>", rl_outstream); break;
	case S_IFBLK:  fputs (" <BLK>", rl_outstream); break;
#ifdef S_IFIFO
	case S_IFIFO:  fputs ("<FIFO>", rl_outstream); break;
#endif
#ifdef S_IFSOCK
	case S_IFSOCK: fputs ("<SOCK>", rl_outstream); break;
#endif
#ifdef S_IFLNK
	case S_IFLNK:  fputs ("<LINK>", rl_outstream); break;
#endif
	default:       fputs (" <\?\?\?>", rl_outstream); break;
	}
      if (line)
	fputs (cmdr_color_normal, rl_outstream);
    }
}

void
cmdr_draw_panel_contents (dir, clean_flag)
    DIRINFO *dir;
    int clean_flag;
{
  int panel_height, panel_width, i, n;

  panel_height = cmdr_lines - _rl_vis_botlin - 4;
  for (i=dir->top; i<dir->nfiles; ++i)
    {
      if (i >= dir->top + panel_height)
	break;
      cmdr_draw_file (dir->tab [i], i - dir->top + 1, dir->base_column + 2);
    }

  /* Clear the rest of panel. */
  if (clean_flag)
    {
      panel_width = cmdr_columns / 2 - 3;
      while (i < dir->top + panel_height)
	{
	  cmdr_term_goto (i - dir->top + 1, dir->base_column + 2);
	  for (n=2; n<panel_width; ++n)
	    fputc (' ', rl_outstream);
  	  ++i;
  	}
    }
}

/*
 * Draw directory name.
 * On exit, set dim color.
 */
void
cmdr_draw_panel_name (dir, use_reverse_color, clean_flag)
    DIRINFO *dir;
    int use_reverse_color, clean_flag;
{
  int panel_width;

  panel_width = cmdr_columns / 2 - 3;
  if (clean_flag)
    {
      /* Clear directory name. */
      fputs (cmdr_color_dim, rl_outstream);
      cmdr_term_goto (0, dir->base_column+1);
      cmdr_term_graphics (1);
      cmdr_hor_line (panel_width);
      cmdr_term_graphics (0);
    }
  cmdr_term_goto (0, dir->base_column + 1 +
		     (panel_width - mbstrlen (dir->short_path)) / 2);
  fputs (use_reverse_color ? cmdr_color_reverse : cmdr_color_dim, rl_outstream);
  fputs (dir->short_path, rl_outstream);
  if (use_reverse_color)
    fputs (*cmdr_color_dim ? cmdr_color_dim : cmdr_color_normal, rl_outstream);
}

/*
 * Status line: ### k/M/G/bytes in ### files
 * Dim color must be set before.
 * On return, normal color is selected.
 */
void
cmdr_draw_status (dir, clean_flag)
    DIRINFO *dir;
    int clean_flag;
{
  char buf [40];
  int panel_width, panel_height;

  panel_width = cmdr_columns / 2 - 3;
  if (clean_flag)
    {
      /* Clear status. */
      panel_height = cmdr_lines - _rl_vis_botlin - 4;
      fputs (cmdr_color_dim, rl_outstream);
      cmdr_term_goto (panel_height + 1, dir->base_column + 1);
      cmdr_term_graphics (1);
      cmdr_hor_line (panel_width);
      cmdr_term_graphics (0);
    }

  if (dir->ntagged)
    {
      cmdr_print_file_size (buf, dir->tagged_bytes);
      if (dir->tagged_bytes <= 99999)
	strcat (buf, " ");
      sprintf (buf + strlen (buf), "bytes in %d tags", dir->ntagged);
      fputs (cmdr_color_bold, rl_outstream);
    }
  else
    {
      cmdr_print_file_size (buf, dir->bytes);
      if (dir->bytes <= 99999)
	strcat (buf, " ");
      sprintf (buf + strlen (buf), "bytes in %d files", dir->nregular);
    }
  cmdr_term_goto (cmdr_lines - _rl_vis_botlin - 3,
    dir->base_column + 1 + (panel_width - strlen (buf)) / 2);
  fputs (buf, rl_outstream);
  fputs (cmdr_color_normal, rl_outstream);
}

void
cmdr_draw_panel (dir, clean_flag)
    DIRINFO *dir;
    int clean_flag;
{
  int panel_width;

  /* Make short path name, to fit into panel width. */
  panel_width = cmdr_columns / 2 - 3;
  if (! dir->short_path)
    {
      dir->short_path = dir->path;
      while (mbstrlen (dir->short_path) > panel_width)
	{
	  char *slash = strchr (dir->short_path, '/');
	  if (! slash)
	    break;
	  dir->short_path = slash + 1;
	}
    }

  /* Draw directory name. */
  cmdr_draw_panel_name (dir, dir == cmdr_panel[cmdr_current_panel], clean_flag);

  /* Draw status line. */
  cmdr_draw_status (dir, clean_flag);

  /* Draw files. */
  cmdr_draw_panel_contents (dir, clean_flag);
}

void
cmdr_draw_cursor (dir)
    DIRINFO *dir;
{
  FILEINFO *cf;

  cf = dir->tab [dir->current];
  cmdr_term_goto (dir->current - dir->top + 1, dir->base_column + 1);
  fputs (cf->tagged ?
	 cmdr_color_bold_reverse : cmdr_color_reverse, rl_outstream);
  fputc (' ', rl_outstream);
  cmdr_draw_file (cf, 0, 0);
  fputc (' ', rl_outstream);
  fputs (cmdr_color_normal, rl_outstream);
}

void
cmdr_undraw_cursor (dir)
    DIRINFO *dir;
{
  FILEINFO *cf;

  cf = dir->tab [dir->current];
  cmdr_term_goto (dir->current - dir->top + 1, dir->base_column + 1);
  fputc (' ', rl_outstream);
  cmdr_draw_file (cf, dir->current - dir->top + 1, dir->base_column + 2);
  fputc (' ', rl_outstream);
}

/*
 * Draw file panels.
 */
void
cmdr_draw_panels()
{
  int panel_height, panel_width, k;

  /* Draw panel borders. */
  panel_height = cmdr_lines - _rl_vis_botlin - 2;
  panel_width = cmdr_columns / 2 - 3;
  cmdr_term_goto (0, 0);
  cmdr_term_graphics (1);
  fputs (cmdr_color_dim, rl_outstream);
  cmdr_corner (7);
  cmdr_hor_line (panel_width);
  cmdr_corner (9);
  putc (' ', rl_outstream);
  cmdr_corner (7);
  cmdr_hor_line (panel_width);
  cmdr_corner (9);

  for (k=1; k<panel_height-1; ++k)
    {
      cmdr_term_goto (k, 0);
      cmdr_vert_line ();
      cmdr_term_goto (k, panel_width + 1);
      cmdr_vert_line ();
      putc (' ', rl_outstream);
      cmdr_vert_line ();
      cmdr_term_goto (k, panel_width + panel_width + 4);
      cmdr_vert_line ();
    }

  cmdr_term_goto (panel_height-1, 0);
  cmdr_corner (1);
  cmdr_hor_line (panel_width);
  cmdr_corner (3);
  putc (' ', rl_outstream);
  cmdr_corner (1);
  cmdr_hor_line (panel_width);
  cmdr_corner (3);
  cmdr_term_graphics (0);
  fputs (cmdr_color_normal, rl_outstream);
  fputs ("\r\n", rl_outstream);
  fflush (rl_outstream);

  /* Reread directories. */
  if (! cmdr_panel[cmdr_current_panel])
    {
      cmdr_panel[cmdr_current_panel] = cmdr_read_directory (0, 0, 0);
      cmdr_panel[cmdr_current_panel]->base_column =
	  (cmdr_current_panel == 0) ? 0 : cmdr_columns / 2;
    }
  else if (cmdr_panel[cmdr_current_panel]->refresh)
    cmdr_panel[cmdr_current_panel] = cmdr_read_directory (0,
					cmdr_panel[cmdr_current_panel], 0);
  if (! cmdr_panel[1-cmdr_current_panel])
    {
      char *home = get_string_value ("HOME");

      if (! home)
	home = "/";
      cmdr_panel[1-cmdr_current_panel] = cmdr_read_directory (home, 0, 0);
      cmdr_panel[1-cmdr_current_panel]->base_column =
	  (cmdr_current_panel == 0) ? cmdr_columns / 2 : 0;
    }
  else if (cmdr_panel[1-cmdr_current_panel]->refresh)
    cmdr_panel[1-cmdr_current_panel] =
      cmdr_read_directory (cmdr_panel[1-cmdr_current_panel]->path,
			   cmdr_panel[1-cmdr_current_panel], 0);

  /* Draw directories ans current file cursor. */
  cmdr_draw_panel (cmdr_panel[0], 0);
  cmdr_draw_panel (cmdr_panel[1], 0);
  cmdr_draw_cursor (cmdr_panel[cmdr_current_panel]);
}

int
cmdr_cursor_up (count, key)
    int count, key;
{
  DIRINFO *dir;

  dir = cmdr_panel [cmdr_current_panel];
  if (dir->current <= 0)
    return 0;

  cmdr_undraw_cursor (dir);
  --dir->current;
  if (dir->current < dir->top)
    {
      --dir->top;
      cmdr_draw_panel_contents (dir, 0);
    }

  cmdr_draw_cursor (dir);
  cmdr_term_goto (cmdr_lines - 2 - _rl_vis_botlin + _rl_last_v_pos,
    _rl_cursor_col());
  return 0;
}

int
cmdr_cursor_down (count, key)
    int count, key;
{
  DIRINFO *dir;
  int panel_height;

  dir = cmdr_panel [cmdr_current_panel];
  if (dir->current >= dir->nfiles - 1)
    return 0;

  if (dir->current >= 0)
    cmdr_undraw_cursor (dir);
  ++dir->current;

  panel_height = cmdr_lines - _rl_vis_botlin - 4;
  if (dir->current >= dir->top + panel_height)
    {
      ++dir->top;
      cmdr_draw_panel_contents (dir, 0);
    }

  cmdr_draw_cursor (dir);
  cmdr_term_goto (cmdr_lines - 2 - _rl_vis_botlin + _rl_last_v_pos,
    _rl_cursor_col());
  return 0;
}

int
cmdr_cursor_left (count, key)
    int count, key;
{
  DIRINFO *dir;
  int panel_height;

  dir = cmdr_panel [cmdr_current_panel];
  if (dir->top <= 0)
    return 0;

  cmdr_undraw_cursor (dir);
  panel_height = cmdr_lines - _rl_vis_botlin - 4;
  dir->top -= panel_height;
  dir->current -= panel_height;
  if (dir->top < 0)
    {
      dir->current -= dir->top;
      dir->top = 0;
    }

  cmdr_draw_panel_contents (dir, 0);

  cmdr_draw_cursor (dir);
  cmdr_term_goto (cmdr_lines - 2 - _rl_vis_botlin + _rl_last_v_pos,
    _rl_cursor_col());
  return 0;
}

int
cmdr_cursor_right (count, key)
    int count, key;
{
  DIRINFO *dir;
  int panel_height;

  dir = cmdr_panel [cmdr_current_panel];
  panel_height = cmdr_lines - _rl_vis_botlin - 4;
  if (dir->top + panel_height >= dir->nfiles)
    return 0;

  cmdr_undraw_cursor (dir);
  dir->top += panel_height;
  dir->current += panel_height;
  if (dir->current >= dir->nfiles)
    dir->current = dir->nfiles - 1;

  cmdr_draw_panel_contents (dir, 1);

  cmdr_draw_cursor (dir);
  cmdr_term_goto (cmdr_lines - 2 - _rl_vis_botlin + _rl_last_v_pos,
    _rl_cursor_col());
  return 0;
}

int
cmdr_cursor_home (count, key)
    int count, key;
{
  DIRINFO *dir;

  dir = cmdr_panel [cmdr_current_panel];
  if (dir->current <= 0)
    return 0;

  cmdr_undraw_cursor (dir);
  dir->current = 0;
  if (dir->top != 0)
    {
      dir->top = 0;
      cmdr_draw_panel_contents (dir, 0);
    }

  cmdr_draw_cursor (dir);
  cmdr_term_goto (cmdr_lines - 2 - _rl_vis_botlin + _rl_last_v_pos,
    _rl_cursor_col());
  return 0;
}

int
cmdr_cursor_end (count, key)
    int count, key;
{
  DIRINFO *dir;
  int panel_height;

  dir = cmdr_panel [cmdr_current_panel];
  if (dir->current >= dir->nfiles-1)
    return 0;

  cmdr_undraw_cursor (dir);
  dir->current = dir->nfiles - 1;

  panel_height = cmdr_lines - _rl_vis_botlin - 4;
  if (dir->top + panel_height < dir->nfiles)
    {
      dir->top = dir->nfiles - panel_height;
      cmdr_draw_panel_contents (dir, 0);
    }

  cmdr_draw_cursor (dir);
  cmdr_term_goto (cmdr_lines - 2 - _rl_vis_botlin + _rl_last_v_pos,
    _rl_cursor_col());
  return 0;
}

int
cmdr_cursor_pgup (count, key)
    int count, key;
{
  DIRINFO *dir;
  int panel_height;

  dir = cmdr_panel [cmdr_current_panel];
  if (dir->current <= 0)
    return 0;

  cmdr_undraw_cursor (dir);
  if (dir->top > 0)
    {
      panel_height = cmdr_lines - _rl_vis_botlin - 4;
      dir->top -= panel_height;
      dir->current -= panel_height;
      if (dir->top < 0)
	{
	  dir->current -= dir->top;
	  dir->top = 0;
	}
      cmdr_draw_panel_contents (dir, 0);
    }
  else
    dir->current = 0;

  cmdr_draw_cursor (dir);
  cmdr_term_goto (cmdr_lines - 2 - _rl_vis_botlin + _rl_last_v_pos,
    _rl_cursor_col());
  return 0;
}

int
cmdr_cursor_pgdn (count, key)
    int count, key;
{
  DIRINFO *dir;
  int panel_height;

  dir = cmdr_panel [cmdr_current_panel];
  if (dir->current >= dir->nfiles - 1)
    return 0;

  cmdr_undraw_cursor (dir);

  panel_height = cmdr_lines - _rl_vis_botlin - 4;
  if (dir->top + panel_height < dir->nfiles)
    {
      dir->top += panel_height;
      dir->current += panel_height;
      if (dir->current >= dir->nfiles)
	dir->current = dir->nfiles - 1;

      cmdr_draw_panel_contents (dir, 1);
    }
  else
    dir->current = dir->nfiles - 1;

  cmdr_draw_cursor (dir);
  cmdr_term_goto (cmdr_lines - 2 - _rl_vis_botlin + _rl_last_v_pos,
    _rl_cursor_col());
  return 0;
}

void
cmdr_insert_text (text)
    char *text;
{
  char buf[10];
#if defined (HANDLE_MULTIBYTE)
  wchar_t c;
  int mbytes;
#else
  char c;
#endif

  buf[0] = '\\';
  for (;;)
    {
#if defined (HANDLE_MULTIBYTE)
      mbytes = mbtowc (&c, text, MB_CUR_MAX);
      if (mbytes <= 0)
	break;
      text += mbytes;
      mbytes = wctomb (buf+1, c);
      if (mbytes <= 0)
	break;
      buf[mbytes+1] = 0;
#else
      c = *text++;
      if (! c)
	break;
      buf[1] = c;
      buf[2] = 0;
#endif
      switch (c)
	{
	case ' ': case '\\': case '?': case '*': case '|':
	case '&': case '[':  case ']': case '(': case ')':
	case '$': case '\'': case '"': case '<': case '>':
	case ';': case '`':  case '!':
	  /* Escape ugly characters. */
	  rl_insert_text (buf);
	  break;
	default:
	  rl_insert_text (buf + 1);
	  break;
	}
    }
  rl_insert_text (" ");
}

int
cmdr_insert_filename (count, key)
    int count, key;
{
  DIRINFO *dir;
  int i;

  /* Insertion is allowed only at end. */
  if (rl_point != rl_end)
    return 0;

  dir = cmdr_panel [cmdr_current_panel];
  if (! dir->ntagged)
    cmdr_insert_text (dir->tab[dir->current]->name);
  else
    {
      /* Insert a list of all tagged files. */
      for (i=0; i<dir->nfiles; ++i)
	if (dir->tab[i]->tagged)
	  cmdr_insert_text (dir->tab[i]->name);
    }
  return 0;
}

/*
 * Build list of arguments for calling commander functions.
 */
WORD_LIST *cmdr_build_arglist (funcname)
    char *funcname;
{
  WORD_LIST *arglist, *tail;
  DIRINFO *dir;
  int i;

  dir = cmdr_panel[cmdr_current_panel];
  arglist = make_word_list (make_word (funcname), (WORD_LIST *)NULL);

  /* Current file name. */
  arglist->next = make_word_list (make_word (dir->tab[dir->current]->name),
    (WORD_LIST *)NULL);
  tail = arglist->next;

  /* Current directory path. */
  tail->next = make_word_list (make_word (dir->path), (WORD_LIST *)NULL);
  tail = tail->next;

  /* Another directory path. */
  tail->next = make_word_list (make_word (cmdr_panel[1-cmdr_current_panel]->path),
			       (WORD_LIST *)NULL);
  tail = tail->next;

  /* Add list of tagged files and directories. */
  for (i=0; i<dir->nfiles; ++i)
    if (dir->tab[i]->tagged)
      {
	tail->next = make_word_list (make_word (dir->tab[i]->name),
				     (WORD_LIST *)NULL);
	tail = tail->next;
      }
  return arglist;
}

/*
 * When user types one of function keys F1..F20,
 * a shell function with name "commander_f#" is executed.
 * Arguments are passed to function:
 * $1 - current file name
 * $2 - current directory name
 * $3 - another directory name
 * $4... - list of tagged files, if any
 */
int
cmdr_function (fnum)
    int fnum;
{
  char funcname [40];
  SHELL_VAR *f;
  WORD_LIST *arglist;
  int fval;

  sprintf (funcname, "commander_f%d", fnum);
  f = find_function (funcname);
  if (! f)
    return 0;

  /* Execute function. */
  arglist = cmdr_build_arglist (funcname);
  fval = execute_shell_function (f, arglist);
  dispose_words (arglist);
  if (fval > 0)
    {
      /* When returned nonzero, we need to reread directories. */
      cmdr_panel[0]->refresh = 1;
      cmdr_panel[1]->refresh = 1;
    }

  /* Redraw the screen. */
  fputs (cmdr_term_clear, rl_outstream);
  cmdr_draw_panels();
  cmdr_term_goto (cmdr_lines - _rl_vis_botlin - 2, 0);
  rl_forced_update_display ();
  return 0;
}

int cmdr_function_1 (count, key) int count, key; { return cmdr_function (1); }
int cmdr_function_2 (count, key) int count, key; { return cmdr_function (2); }
int cmdr_function_3 (count, key) int count, key; { return cmdr_function (3); }
int cmdr_function_4 (count, key) int count, key; { return cmdr_function (4); }
int cmdr_function_5 (count, key) int count, key; { return cmdr_function (5); }
int cmdr_function_6 (count, key) int count, key; { return cmdr_function (6); }
int cmdr_function_7 (count, key) int count, key; { return cmdr_function (7); }
int cmdr_function_8 (count, key) int count, key; { return cmdr_function (8); }
int cmdr_function_9 (count, key) int count, key; { return cmdr_function (9); }
int cmdr_function_10 (count, key) int count, key; { return cmdr_function (10); }
int cmdr_function_11 (count, key) int count, key; { return cmdr_function (11); }
int cmdr_function_12 (count, key) int count, key; { return cmdr_function (12); }
int cmdr_function_13 (count, key) int count, key; { return cmdr_function (13); }
int cmdr_function_14 (count, key) int count, key; { return cmdr_function (14); }
int cmdr_function_15 (count, key) int count, key; { return cmdr_function (15); }
int cmdr_function_16 (count, key) int count, key; { return cmdr_function (16); }
int cmdr_function_17 (count, key) int count, key; { return cmdr_function (17); }
int cmdr_function_18 (count, key) int count, key; { return cmdr_function (18); }
int cmdr_function_19 (count, key) int count, key; { return cmdr_function (19); }
int cmdr_function_20 (count, key) int count, key; { return cmdr_function (20); }

/*
 * Kill the current input line, saving it in kill ring.
 */
void
cmdr_save_readline ()
{
  cmdr_saved_line = 1;
  if (rl_end > 0)
    {
      rl_kill_full_line (0, 0);
      ++cmdr_saved_line;
    }
}

/*
 * Yank text from kill ring into the current input line.
 */
void
cmdr_restore_readline ()
{
  if (cmdr_saved_line > 1)
    rl_execute_next (CTRL('Y'));
  cmdr_saved_line = 0;
  cmdr_term_goto (cmdr_lines - 2, 0);
}

/*
 * When in visual mode - switch to next panel.
 * When in line mode and command line is empty - switch
 * to next panel and enable visual mode.
 */
int
cmdr_next_panel (count, key)
    int count, key;
{
  char *newwd;

  if (! cmdr_visual_mode)
    {
      /* Line mode: <Tab> pressed. */
      if (rl_end != 0)
	{
	  /* Line mode: <Tab> pressed on nonempty command line.
	   * Call the original bash function. */
	  return cmdr_original_tab_func (count, key);
	}

      /* Current directory probably changed -- reread it. */
      cmdr_panel[cmdr_current_panel] = cmdr_read_directory (0,
				       cmdr_panel[cmdr_current_panel], 0);
    }

  /* Switch panels. */
  newwd = cmdr_panel[1-cmdr_current_panel]->path;
  if (chdir (newwd) < 0)
    return 0;
  if (the_current_working_directory)
    free (the_current_working_directory);
  the_current_working_directory = savestring (newwd);
  bind_variable ("PWD", newwd, 0);
  cmdr_current_panel ^= 1;

  if (! cmdr_visual_mode)
    {
      /* Line mode: <Tab> pressed on empty command line.
       * Switch to next panel and enable visual mode. */
      rl_execute_next (CTRL('M'));
      return 0;
    }

  /* Panel mode: redraw cursor and panel names. */
  cmdr_undraw_cursor (cmdr_panel [1-cmdr_current_panel]);
  cmdr_draw_panel_name (cmdr_panel [1-cmdr_current_panel], 0, 0);
  cmdr_draw_panel_name (cmdr_panel [cmdr_current_panel], 1, 0);
  cmdr_draw_cursor (cmdr_panel [cmdr_current_panel]);

  /* Move to screen bottom. */
  cmdr_term_goto (cmdr_lines - _rl_vis_botlin - 2, 0);
  fputs (cmdr_term_clreol, rl_outstream);

  /* After changing directory, the prompt must be recomputed, as it may
   * contain PWD.  To perform this, we simulate an empty command.
   * Save the contents of current line and restore it later. */
  cmdr_save_readline ();
  rl_execute_next (CTRL('M'));
  return 0;
}

/*
 * Enter directory.
 */
void
cmdr_enter_directory (name)
    char *name;
{
  DIRINFO *olddir;

  if (chdir (name) < 0)
    return;
  olddir = cmdr_panel [cmdr_current_panel];
  cmdr_undraw_cursor (olddir);

  cmdr_panel[cmdr_current_panel] = cmdr_read_directory (0, 0, &olddir->st);
  cmdr_panel[cmdr_current_panel]->base_column = olddir->base_column;
  cmdr_free_directory (olddir);
  bind_variable ("PWD", cmdr_panel[cmdr_current_panel]->path, 0);

  if (the_current_working_directory)
    free (the_current_working_directory);
  the_current_working_directory =
    savestring (cmdr_panel[cmdr_current_panel]->path);

  cmdr_draw_panel (cmdr_panel [cmdr_current_panel], 1);
  cmdr_draw_cursor (cmdr_panel [cmdr_current_panel]);
  cmdr_term_goto (cmdr_lines - 2, 0);
  fputs (cmdr_term_clreol, rl_outstream);
}

/*
 * Start file.
 * Run function "commander_start_file".
 * It must set a variable COMMANDER_LINE.
 * Put it's value to command line and execute.
 */
void
cmdr_start_file ()
{
  char *funcname;
  WORD_LIST *arglist;
  SHELL_VAR *f, *cmd;
  int fval;

  funcname = "commander_start_file";
  f = find_function (funcname);
  if (! f)
    {
nothing_to_run:
      /* Move to screen bottom. */
      cmdr_term_goto (cmdr_lines - _rl_vis_botlin - 2, 0);
      fputs (cmdr_term_clreol, rl_outstream);
      return;
    }

  /* Execute function. */
  arglist = cmdr_build_arglist (funcname);
  unbind_variable ("COMMANDER_LINE");
  fval = execute_shell_function (f, arglist);
  dispose_words (arglist);

  cmd = find_variable ("COMMANDER_LINE");
  if (! cmd || ! cmd->value || ! *cmd->value)
    goto nothing_to_run;

  /* Move to screen bottom. */
  cmdr_term_goto (cmdr_lines - _rl_vis_botlin - 2, 0);
  rl_forced_update_display ();
  fputs (cmd->value, rl_outstream);
  fputs ("\r\n", rl_outstream);

  /* Execute command line. */
  parse_and_execute (savestring (cmd->value), "commander", SEVAL_NONINT);
  reset_parser ();
  unbind_variable ("COMMANDER_LINE");

  if (fval > 0)
    {
      /* When returned nonzero, remain in panel mode. */
      fputs (cmdr_term_clear, rl_outstream);
      cmdr_draw_panels();
      cmdr_term_goto (cmdr_lines - _rl_vis_botlin - 2, 0);
    }
  else
      cmdr_set_visual_mode (0);
}

/*
 * Size of terminal window changed -- redraw panels.
 */
void
cmdr_set_lines_and_columns (lines, cols)
     int lines, cols;
{
  if (! cmdr_visual_mode)
    return;
  if (cmdr_lines == lines && cmdr_columns == cols)
    return;

  cmdr_lines = lines;
  cmdr_columns = cols;
  if (cmdr_lines < 6 || cmdr_lines > 500 ||
      cmdr_columns < 50 || cmdr_columns > 1000)
    {
      /* Too small window, use line mode. */
      cmdr_visual_mode = 0;
      rl_set_keymap (cmdr_line_keymap);
      fputs (cmdr_term_clear, rl_outstream);
      rl_forced_update_display ();
      return;
    }

  /* Reinit on window resize. */
  if (cmdr_panel[1])
    cmdr_panel[1]->base_column = cmdr_columns / 2;
  fputs (cmdr_term_clear, rl_outstream);
  cmdr_draw_panels();
  cmdr_term_goto (cmdr_lines - _rl_vis_botlin - 2, 0);
  rl_forced_update_display ();
}

/*
 * Set visual mode.
 */
void
cmdr_set_visual_mode (enable_visual)
    int enable_visual;
{
  if (enable_visual)
    {
      /* Enable visual mode. */
      rl_get_screen_size (&cmdr_lines, &cmdr_columns);
      if (cmdr_lines < 6 || cmdr_lines > 500 ||
	  cmdr_columns < 50 || cmdr_columns > 1000)
	  return;
      cmdr_visual_mode = 1;
      rl_set_keymap (cmdr_visual_keymap);

      /* Reinit on window resize. */
      if (cmdr_panel[1])
        cmdr_panel[1]->base_column = cmdr_columns / 2;
      fputs (cmdr_term_clear, rl_outstream);
      cmdr_draw_panels();

      /* Move to screen bottom. */
      cmdr_term_goto (cmdr_lines - _rl_vis_botlin - 2, 0);
      return;
    }

  /* Disable visual mode. */
  cmdr_visual_mode = 0;
  rl_set_keymap (cmdr_line_keymap);
}

/*
 * Switch between visual and line modes.
 */
int
cmdr_switch_mode (count, key)
    int count, key;
{
  if (cmdr_visual_mode)
    {
      cmdr_set_visual_mode (0);
      cmdr_undraw_cursor (cmdr_panel [cmdr_current_panel]);
      cmdr_term_goto (cmdr_lines - _rl_vis_botlin - 2, 0);
    }
  else
    {
      if (! cmdr_term_cm)
	{
	  cmdr_reset_terminal ();
	  if (! cmdr_term_cm)
	    {
	      fprintf (stderr, "Cannot enable commander: cursor positioning not supported\r\n");
	      return 0;
	    }
	}
      cmdr_set_visual_mode (1);
    }

  rl_forced_update_display ();
  return 0;
}

/*
 * Set line mode and clear screen.
 */
int
cmdr_clear_screen (count, key)
    int count, key;
{
  if (! cmdr_visual_mode)
    return 0;

  fputs (cmdr_term_clear, rl_outstream);
  cmdr_set_visual_mode (0);
  rl_forced_update_display ();
  return 0;
}

/*
 * Toggle displaying of hidden files (dot names).
 */
int
cmdr_toggle_hidden (count, key)
    int count, key;
{
  cmdr_show_hidden ^= 1;
  cmdr_panel[0]->refresh = 1;
  cmdr_panel[1]->refresh = 1;
  fputs (cmdr_term_clear, rl_outstream);
  cmdr_draw_panels();
  cmdr_term_goto (cmdr_lines - _rl_vis_botlin - 2, 0);
  rl_forced_update_display ();
  return 0;
}

/*
 * Tag current file and move cursor down.
 */
int
cmdr_tag_file (count, key)
    int count, key;
{
  DIRINFO *dir;
  FILEINFO *fi;

  dir = cmdr_panel[cmdr_current_panel];
  fi = dir->tab[dir->current];

  /* Cannot tag dotdot. */
  if (fi->name[0] == '.' && fi->name[1] == '.' && ! fi->name[2])
    return cmdr_cursor_down (count, key);

  if (fi->tagged)
    {
      /* Clear tag */
      fi->tagged = 0;
      --dir->ntagged;
      if (S_ISREG (fi->st.st_mode))
	dir->tagged_bytes -= fi->st.st_size;
    }
  else
    {
      /* Set tag. */
      fi->tagged = 1;
      ++dir->ntagged;
      if (S_ISREG (fi->st.st_mode))
	dir->tagged_bytes += fi->st.st_size;
    }

  /* Draw status line. */
  cmdr_draw_status (dir, 1);

  if (dir->current >= dir->nfiles - 1)
    dir->current = dir->nfiles - 2;
  return cmdr_cursor_down (count, key);
}

/*
 * Return a new recursive copy of a given keymap.
 */
static Keymap
copy_keymap (map)
     Keymap map;
{
  int i;
  Keymap newmap;

  newmap = rl_make_bare_keymap ();
  for (i = 0; i < KEYMAP_SIZE; i++)
    {
      newmap[i].type = map[i].type;
      if (map[i].type == ISKMAP)
	newmap[i].function =
	  (rl_command_func_t*) copy_keymap ((Keymap) map[i].function);
      else
	newmap[i].function = map[i].function;
    }
  return (newmap);
}

/*
 * Get termcap entry. Strip delay sequences $<...>.
 */
static char *
get_termcap (char *entry)
{
  char *value, *p;

  value = rl_get_termcap (entry);
  if (! value)
    return 0;
  p = strchr (value, '$');
  if (p)
    *p = 0;
  return value;
}

/*
 * Read terminal description data.
 */
void
cmdr_reset_terminal ()
{
/*fprintf (stderr, "cmdr_reset_terminal() called, TERM='%s'\r\n", get_locale_var ("TERM"));*/
  if (! cmdr_line_keymap)
    return;

  cmdr_term_cm = get_termcap ("cm");
  cmdr_term_ku = get_termcap ("ku");
  cmdr_term_kd = get_termcap ("kd");
  cmdr_term_kr = get_termcap ("kr");
  cmdr_term_kl = get_termcap ("kl");
  cmdr_term_kh = get_termcap ("kh");
  cmdr_term_at7 = get_termcap ("@7");
  cmdr_term_kP = get_termcap ("kP");
  cmdr_term_kN = get_termcap ("kN");
  cmdr_term_kI = get_termcap ("kI");
  cmdr_term_clear = get_termcap ("cl");
  cmdr_term_clreol = get_termcap ("ce");
  cmdr_term_ac = get_termcap ("ac");
  cmdr_term_as = get_termcap ("as");
  cmdr_term_ae = get_termcap ("ae");
  cmdr_term_me = get_termcap ("me");
  cmdr_term_mr = get_termcap ("mr");
  cmdr_term_md = get_termcap ("md");
  cmdr_term_kf1 = get_termcap ("k1");
  cmdr_term_kf2 = get_termcap ("k2");
  cmdr_term_kf3 = get_termcap ("k3");
  cmdr_term_kf4 = get_termcap ("k4");
  cmdr_term_kf5 = get_termcap ("k5");
  cmdr_term_kf6 = get_termcap ("k6");
  cmdr_term_kf7 = get_termcap ("k7");
  cmdr_term_kf8 = get_termcap ("k8");
  cmdr_term_kf9 = get_termcap ("k9");
  cmdr_term_kf10 = get_termcap ("k;");
  cmdr_term_kf11 = get_termcap ("F1");
  cmdr_term_kf12 = get_termcap ("F2");
  cmdr_term_kf13 = get_termcap ("F3");
  cmdr_term_kf14 = get_termcap ("F4");
  cmdr_term_kf15 = get_termcap ("F5");
  cmdr_term_kf16 = get_termcap ("F6");
  cmdr_term_kf17 = get_termcap ("F7");
  cmdr_term_kf18 = get_termcap ("F8");
  cmdr_term_kf19 = get_termcap ("F9");
  cmdr_term_kf20 = get_termcap ("FA");
  cmdr_reset_graphics ();

  /* Page Up, Page Down, F1-F20 in line mode - ignore. */
  rl_bind_keyseq_in_map (cmdr_term_kP, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kN, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf1, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf2, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf3, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf4, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf5, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf6, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf7, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf8, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf9, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf10, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf11, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf12, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf13, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf14, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf15, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf16, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf17, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf18, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf19, 0, cmdr_line_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf20, 0, cmdr_line_keymap);

  /* Cursor arrows - browsing */
  rl_bind_keyseq_in_map (cmdr_term_ku, cmdr_cursor_up, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kd, cmdr_cursor_down, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kr, cmdr_cursor_right, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kl, cmdr_cursor_left, cmdr_visual_keymap);

  /* Home, End, Page Up, Page Down - browsing */
  rl_bind_keyseq_in_map (cmdr_term_kh, cmdr_cursor_home, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_at7, cmdr_cursor_end, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kP, cmdr_cursor_pgup, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kN, cmdr_cursor_pgdn, cmdr_visual_keymap);

  /* Insert - tag/untag current file */
  rl_bind_keyseq_in_map (cmdr_term_kI, cmdr_tag_file, cmdr_visual_keymap);

  /* F1-F20 - run user-defined functions. */
  rl_bind_keyseq_in_map (cmdr_term_kf1, cmdr_function_1, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf2, cmdr_function_2, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf3, cmdr_function_3, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf4, cmdr_function_4, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf5, cmdr_function_5, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf6, cmdr_function_6, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf7, cmdr_function_7, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf8, cmdr_function_8, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf9, cmdr_function_9, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf10, cmdr_function_10, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf11, cmdr_function_11, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf12, cmdr_function_12, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf13, cmdr_function_13, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf14, cmdr_function_14, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf15, cmdr_function_15, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf16, cmdr_function_16, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf17, cmdr_function_17, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf18, cmdr_function_18, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf19, cmdr_function_19, cmdr_visual_keymap);
  rl_bind_keyseq_in_map (cmdr_term_kf20, cmdr_function_20, cmdr_visual_keymap);
}

/*
 * Enable commander mode.
 */
int
cmdr_init (params)
    char *params;
{
  char kseq[2];

  parse_colors (params);
  if (cmdr_line_keymap)
    return 0;
  cmdr_visual_mode = 0;

  /* Line mode keymap. */
  cmdr_line_keymap = rl_get_keymap();

  /* ^O - switch between visual and line modes */
  rl_bind_key_in_map (CTRL('O'), cmdr_switch_mode, cmdr_line_keymap);

  /* Tab - switch to visual mode move to next panel */
  kseq[0] = CTRL('I');
  kseq[1] = 0;
  cmdr_original_tab_func = rl_function_of_keyseq (kseq, cmdr_line_keymap,
    (int *)NULL);
  rl_bind_key_in_map (CTRL('I'), cmdr_next_panel, cmdr_line_keymap);

  /* Build keymap for visual mode. */
  cmdr_visual_keymap = copy_keymap (cmdr_line_keymap);

  /* Tab - move to next panel
   * ^J - insert current filename into command line
   * ^L - switch to line mode and clear screen
   * Insert, ^T - tag/untag current file
   * ^X-a - toggle displaying of hidden files */
  rl_bind_key_in_map (CTRL('J'), cmdr_insert_filename, cmdr_visual_keymap);
  rl_bind_key_in_map (CTRL('L'), cmdr_clear_screen, cmdr_visual_keymap);
  rl_bind_key_in_map (CTRL('T'), cmdr_tag_file, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\030a", cmdr_toggle_hidden, cmdr_visual_keymap);

#if defined (__MSDOS__)
  rl_bind_keyseq_in_map ("\033[0A", cmdr_cursor_up, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\033[0B", cmdr_cursor_left, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\033[0C", cmdr_cursor_right, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\033[0D", cmdr_cursor_down, cmdr_visual_keymap);
#endif

  rl_bind_keyseq_in_map ("\033[A", cmdr_cursor_up, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\033[B", cmdr_cursor_down, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\033[C", cmdr_cursor_right, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\033[D", cmdr_cursor_left, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\033[H", cmdr_cursor_home, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\033[F", cmdr_cursor_end, cmdr_visual_keymap);

  rl_bind_keyseq_in_map ("\033OA", cmdr_cursor_up, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\033OB", cmdr_cursor_down, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\033OC", cmdr_cursor_right, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\033OD", cmdr_cursor_left, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\033OH", cmdr_cursor_home, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\033OF", cmdr_cursor_end, cmdr_visual_keymap);

#if defined (__MINGW32__)
  rl_bind_keyseq_in_map ("\340H", cmdr_cursor_up, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\340P", cmdr_cursor_down, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\340M", cmdr_cursor_right, cmdr_visual_keymap);
  rl_bind_keyseq_in_map ("\340K", cmdr_cursor_left, cmdr_visual_keymap);
#endif

  cmdr_reset_terminal ();

  /* TODO: ^X-t, ^X-u - tag and untag files by pattern, e.g. *.txt
   * TODO: ^X-w - display current panel on the whole screen width
   *              in two columns. */
  return 0;
}

/*
 * Disable commander mode.
 */
void
cmdr_disable ()
{
  if (! cmdr_line_keymap)
    return;

  /* Discard visual keymap. */
  rl_unbind_function_in_map (cmdr_switch_mode, cmdr_line_keymap);
  rl_set_keymap (cmdr_line_keymap);
  rl_discard_keymap (cmdr_visual_keymap);
  cmdr_visual_keymap = 0;
  cmdr_line_keymap = 0;
  cmdr_visual_mode = 0;

  if (cmdr_panel[0])
    {
      cmdr_free_directory (cmdr_panel[0]);
      cmdr_panel[0] = 0;
    }
  if (cmdr_panel[1])
    {
      cmdr_free_directory (cmdr_panel[1]);
      cmdr_panel[1] = 0;
    }
}

/*
 * Switch between visual panel mode and line mode.
 * Argument `active' can have one of the following values:
 *  1 - activate visual panel mode. Move cursor to screen bottom.
 *      Reread directories.
 *  0 - switch back to line mode. Clear screen, redisplay entered line
 *      and print CR-LF.
 * -1 - the same but no CR-LF.
 */
void
cmdr_activate (active)
    int active;
{
  DIRINFO *dir;
  FILEINFO *fi;

  if (! cmdr_line_keymap)
    return;

  if (active > 0)
    {
      if (cmdr_visual_mode)
	{
    	  /* Key <Enter> pressed in visual panel mode. */
	  if (cmdr_saved_line)
	    {
	      /* Restore saved command line. This is needed for
	       * preserving command line when Tab is pressed. */
	      cmdr_restore_readline ();
	    }
	  else
	    {
	      /* Run file under cursor. */
	      dir = cmdr_panel [cmdr_current_panel];
	      fi = dir->tab [dir->current];
	      if (S_ISDIR (fi->st.st_mode))
		cmdr_enter_directory (fi->name);
	      else if (S_ISREG (fi->st.st_mode))
		cmdr_start_file (fi);
	    }
	}
      else
	{
	  /* Enable visual mode. */
	  if (! cmdr_term_cm)
	    {
	      cmdr_reset_terminal ();
	      if (! cmdr_term_cm)
		return;
	    }
	  cmdr_set_visual_mode (1);
	}
    }
  else
    {
      /* Some command was executed, so we need to refresh panels. */
      if (cmdr_panel[0])
	cmdr_panel[0]->refresh = 1;
      if (cmdr_panel[1])
	cmdr_panel[1]->refresh = 1;

      if (cmdr_visual_mode)
	{
	  /* Return to line mode. */
	  cmdr_set_visual_mode (0);
	  fputs (cmdr_term_clear, rl_outstream);

	  /* Move to screen bottom. */
	  cmdr_term_goto (cmdr_lines - _rl_vis_botlin - 2, 0);
	  rl_forced_update_display ();
	  if (active >= 0)
	    rl_crlf();
	}
    }

  fflush (stderr);
}
