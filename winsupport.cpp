
#include "winsupport.h"

#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

char* strsep(char** stringp, const char* delim)
{
  char* start = *stringp;
  char* p;

  p = (start != NULL) ? strpbrk(start, delim) : NULL;

  if (p == NULL)
  {
    *stringp = NULL;
  }
  else
  {
    *p = '\0';
    *stringp = p + 1;
  }

  return start;
}


/*	$OpenBSD: strlcpy.c,v 1.8 2003/06/17 21:56:24 millert Exp $	*/

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$OpenBSD: strlcpy.c,v 1.8 2003/06/17 21:56:24 millert Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <string.h>

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

/*	$OpenBSD: strlcat.c,v 1.8 2001/05/13 15:40:15 deraadt Exp $	*/

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$OpenBSD: strlcat.c,v 1.8 2001/05/13 15:40:15 deraadt Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <string.h>

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
strlcat(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}

/*-
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

int
strcasecmp(const char *s1, const char *s2)
{
	const char *us1 = (const char *)s1, *us2 = (const char *)s2;

	while (tolower(*us1) == tolower(*us2)) {
		if (*us1++ == '\0')
			return (0);
		us2++;
	}
	return (tolower(*us1) - tolower(*us2));
}

int
strncasecmp(const char *s1, const char *s2, size_t n)
{

	if (n != 0) {
		const char *us1 = (const char *)s1;
		const char *us2 = (const char *)s2;

		do {
			if (tolower(*us1) != tolower(*us2))
				return (tolower(*us1) - tolower(*us2));
			if (*us1++ == '\0')
				break;
			us2++;
		} while (--n != 0);
	}
	return (0);
}


const void *
memmem(const void *l, size_t l_len, const void *s, size_t s_len)
{
        register const char *cur, *last;
        const char *cl = (const char *)l;
        const char *cs = (const char *)s;

        /* we need something to compare */
        if (l_len == 0 || s_len == 0)
                return NULL;

        /* "s" must be smaller or equal to "l" */
        if (l_len < s_len)
                return NULL;

        /* special case where s_len == 1 */
        if (s_len == 1)
                return memchr(l, (int)*cs, l_len);

        /* the last position where its possible to find "s" in "l" */
        last = cl + l_len - s_len;

        for (cur = cl; cur <= last; cur++)
                if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0)
                        return cur;

        return NULL;
}

#define	_CTYPE_A	0x00000100L		/* Alpha */
#define	_CTYPE_C	0x00000200L		/* Control */
#define	_CTYPE_D	0x00000400L		/* Digit */
#define	_CTYPE_G	0x00000800L		/* Graph */
#define	_CTYPE_L	0x00001000L		/* Lower */
#define	_CTYPE_P	0x00002000L		/* Punct */
#define	_CTYPE_S	0x00004000L		/* Space */
#define	_CTYPE_U	0x00008000L		/* Upper */
#define	_CTYPE_X	0x00010000L		/* X digit */
#define	_CTYPE_B	0x00020000L		/* Blank */
#define	_CTYPE_R	0x00040000L		/* Print */
#define	_CTYPE_I	0x00080000L		/* Ideogram */
#define	_CTYPE_T	0x00100000L		/* Special */
#define	_CTYPE_Q	0x00200000L		/* Phonogram */
#define	_CTYPE_SW0	0x20000000L		/* 0 width character */
#define	_CTYPE_SW1	0x40000000L		/* 1 width character */
#define	_CTYPE_SW2	0x80000000L		/* 2 width character */
#define	_CTYPE_SW3	0xc0000000L		/* 3 width character */
#define	_CTYPE_SWM	0xe0000000L		/* Mask for screen width data */
#define	_CTYPE_SWS	30			/* Bits to shift to get width */

#define	_CACHED_RUNES	(1 <<8 )	/* Must be a power of 2 */
#define	_CRMASK		(~(_CACHED_RUNES - 1))

typedef int __rune_t;

/*
* The lower 8 bits of runetype[] contain the digit value of the rune.
*/
typedef struct {
    __rune_t	__min;		/* First rune of the range */
    __rune_t	__max;		/* Last rune (inclusive) of the range */
    __rune_t	__map;		/* What first maps to in maps */
    unsigned long	*__types;	/* Array of types in range */
} _RuneEntry;

typedef struct {
    int		__nranges;	/* Number of ranges stored */
    _RuneEntry	*__ranges;	/* Pointer to the ranges */
} _RuneRange;

typedef struct {
    char		__magic[8];	/* Magic saying what version we are */
    char		__encoding[32];	/* ASCII name of this encoding */

    __rune_t(*__sgetrune)(const char *, size_t, char const **);
    int(*__sputrune)(__rune_t, char *, size_t, char **);
    __rune_t	__invalid_rune;

    unsigned long	__runetype[_CACHED_RUNES];
    __rune_t	__maplower[_CACHED_RUNES];
    __rune_t	__mapupper[_CACHED_RUNES];

    /*
    * The following are to deal with Runes larger than _CACHED_RUNES - 1.
    * Their data is actually contiguous with this structure so as to make
    * it easier to read/write from/to disk.
    */
    _RuneRange	__runetype_ext;
    _RuneRange	__maplower_ext;
    _RuneRange	__mapupper_ext;

    void		*__variable;	/* Data which depends on the encoding */
    int		__variable_len;	/* how long that data is */
} _RuneLocale;

#define	_RUNE_MAGIC_1	"RuneMag"	/* Indicates version 0 of RuneLocale */

const _RuneLocale _DefaultRuneLocale = {
    _RUNE_MAGIC_1,
    "NONE",
    NULL,
    NULL,
    0xFFFD,

    {	/*00*/	_CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    /*08*/	_CTYPE_C,
    _CTYPE_C | _CTYPE_S | _CTYPE_B,
    _CTYPE_C | _CTYPE_S,
    _CTYPE_C | _CTYPE_S,
    _CTYPE_C | _CTYPE_S,
    _CTYPE_C | _CTYPE_S,
    _CTYPE_C,
    _CTYPE_C,
    /*10*/	_CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    /*18*/	_CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    _CTYPE_C,
    /*20*/	_CTYPE_S | _CTYPE_B | _CTYPE_R,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    /*28*/	_CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    /*30*/	_CTYPE_D | _CTYPE_R | _CTYPE_G | _CTYPE_X | 0,
    _CTYPE_D | _CTYPE_R | _CTYPE_G | _CTYPE_X | 1,
    _CTYPE_D | _CTYPE_R | _CTYPE_G | _CTYPE_X | 2,
    _CTYPE_D | _CTYPE_R | _CTYPE_G | _CTYPE_X | 3,
    _CTYPE_D | _CTYPE_R | _CTYPE_G | _CTYPE_X | 4,
    _CTYPE_D | _CTYPE_R | _CTYPE_G | _CTYPE_X | 5,
    _CTYPE_D | _CTYPE_R | _CTYPE_G | _CTYPE_X | 6,
    _CTYPE_D | _CTYPE_R | _CTYPE_G | _CTYPE_X | 7,
    /*38*/	_CTYPE_D | _CTYPE_R | _CTYPE_G | _CTYPE_X | 8,
    _CTYPE_D | _CTYPE_R | _CTYPE_G | _CTYPE_X | 9,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    /*40*/	_CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_U | _CTYPE_X | _CTYPE_R | _CTYPE_G | _CTYPE_A | 10,
    _CTYPE_U | _CTYPE_X | _CTYPE_R | _CTYPE_G | _CTYPE_A | 11,
    _CTYPE_U | _CTYPE_X | _CTYPE_R | _CTYPE_G | _CTYPE_A | 12,
    _CTYPE_U | _CTYPE_X | _CTYPE_R | _CTYPE_G | _CTYPE_A | 13,
    _CTYPE_U | _CTYPE_X | _CTYPE_R | _CTYPE_G | _CTYPE_A | 14,
    _CTYPE_U | _CTYPE_X | _CTYPE_R | _CTYPE_G | _CTYPE_A | 15,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    /*48*/	_CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    /*50*/	_CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    /*58*/	_CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_U | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    /*60*/	_CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_L | _CTYPE_X | _CTYPE_R | _CTYPE_G | _CTYPE_A | 10,
    _CTYPE_L | _CTYPE_X | _CTYPE_R | _CTYPE_G | _CTYPE_A | 11,
    _CTYPE_L | _CTYPE_X | _CTYPE_R | _CTYPE_G | _CTYPE_A | 12,
    _CTYPE_L | _CTYPE_X | _CTYPE_R | _CTYPE_G | _CTYPE_A | 13,
    _CTYPE_L | _CTYPE_X | _CTYPE_R | _CTYPE_G | _CTYPE_A | 14,
    _CTYPE_L | _CTYPE_X | _CTYPE_R | _CTYPE_G | _CTYPE_A | 15,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    /*68*/	_CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    /*70*/	_CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    /*78*/	_CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_L | _CTYPE_R | _CTYPE_G | _CTYPE_A,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_P | _CTYPE_R | _CTYPE_G,
    _CTYPE_C,
    },
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
    'x', 'y', 'z', 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
    'x', 'y', 'z', 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
    },
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
    'X', 'Y', 'Z', 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
    'X', 'Y', 'Z', 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
    },
};

typedef	int		__ct_rune_t;	/* arg type for ctype funcs */

static __inline int
__sbmaskrune(__ct_rune_t _c, unsigned long _f)
{
    return (_c < 0 || _c >= 256) ? 0 :
        _DefaultRuneLocale.__runetype[_c] & _f;
}

static __inline int
__sbistype(__ct_rune_t _c, unsigned long _f)
{
    return (!!__sbmaskrune(_c, _f));
}

int ishexnumber(int c)
{
    return (__sbistype(c, _CTYPE_X));
}

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

void rwkwin_print_err(DWORD err) {
    CHAR str[8192];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  err,
                  0,
                  str,
                  8192,
                  NULL);
    printf("%s\n", str);
}


