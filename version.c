/* version.c -- distribution and version numbers. */

/* Copyright (C) 2023 Serge Vakulenko

   This file is part of Bash Commander.

   Bash Commander is heavily based on GNU Bash sources.

   Bash Commander is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash Commander is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include "patchlevel.h"

const char * const dist_version = DISTVERSION;
const int patch_level = PATCHLEVEL;
const int build_version = BUILDVERSION;
const char * const release_status = "release";

const char * const bash_copyright = "Copyright (C) 2023 Serge Vakulenko";
const char * const bash_license = "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n";

/* If == 31, shell compatible with bash-3.1, == 32 with bash-3.2, and so on */
int shell_compatibility_level = DEFAULT_COMPAT_LEVEL;

/* Functions for getting, setting, and displaying the shell version. */

/* Give version information about this shell. */
char *
shell_version_string ()
{
  static char tt[32] = { '\0' };

  if (tt[0] == '\0')
    {
      snprintf (tt, sizeof (tt), "%s.%d.%d", dist_version, patch_level, build_version);
    }
  return tt;
}

void
show_shell_version (int extended)
{
  printf ("Bash Commander, version %s (%s)\n", shell_version_string (), CONF_MACHTYPE);
  if (extended)
    {
      printf ("%s\n", bash_copyright);
      printf ("%s\n", bash_license);
      printf ("%s\n", "This is free software; you are free to change and redistribute it.");
      printf ("%s\n", "There is NO WARRANTY, to the extent permitted by law.");
    }
}
