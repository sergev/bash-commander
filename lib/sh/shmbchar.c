/* Copyright (C) 2001, 2006, 2009, 2010, 2012 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


#include <config.h>

#if defined (HANDLE_MULTIBYTE)
#include <stdlib.h>
#include <limits.h>

#include <shmbutil.h>
#include <shmbchar.h>

#if IS_BASIC_ASCII

/* Bit table of characters in the ISO C "basic character set".  */
const unsigned int is_basic_table [UCHAR_MAX / 32 + 1] =
{
  0x00001a00,           /* '\t' '\v' '\f' */
  0xffffffef,           /* ' '...'#' '%'...'?' */
  0xfffffffe,           /* 'A'...'Z' '[' '\\' ']' '^' '_' */
  0x7ffffffe            /* 'a'...'z' '{' '|' '}' '~' */
  /* The remaining bits are 0.  */
};

#endif /* IS_BASIC_ASCII */

size_t
mbstrlen (s)
     const char *s;
{
  size_t clen, nc;
  mbstate_t mbs = { 0 }, mbsbak = { 0 };
  int f;

  nc = 0;
  while (*s && (clen = (f = is_basic (*s)) ? 1 : mbrlen(s, MB_CUR_MAX, &mbs)) != 0)
    {
      if (MB_INVALIDCH(clen))
	{
	  clen = 1;	/* assume single byte */
	  mbs = mbsbak;
	}

      if (f == 0)
	mbsbak = mbs;

      s += clen;
      nc++;
    }
  return nc;
}

/* Return pointer to first multibyte char in S, or NULL if none. */
char *
mbsmbchar (s)
     const char *s;
{
  char *t;
  size_t clen;
  mbstate_t mbs = { 0 };

  for (t = (char *)s; *t; t++)
    {
      if (is_basic (*t))
	continue;

      clen = mbrlen (t, MB_CUR_MAX, &mbs);

      if (clen == 0)
        return 0;
      if (MB_INVALIDCH(clen))
	continue;

      if (clen > 1)
	return t;
    }
  return 0;
}

int
sh_mbsnlen(src, srclen, maxlen)
     const char *src;
     size_t srclen;
     int maxlen;
{
  int count;
  int sind;
  DECLARE_MBSTATE;

  for (sind = count = 0; src[sind]; )
    {
      count++;		/* number of multibyte characters */
      ADVANCE_CHAR (src, srclen, sind);
      if (sind > maxlen)
        break;
    }

  return count;
}
#endif
