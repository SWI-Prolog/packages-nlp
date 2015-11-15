/* Copyright 2004-2011 by the National and Technical University of Athens

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This C-version is created by  Jan   Wielemaker,  starting from an almost
literal translation from the Java code.  From   there  I  did some minor
reorganisation, for example to get   score_inplace(), which modifies the
argument strings, but this doesn't matter for the Prolog interface if as
we must ask for BUF_MALLOC anyway.

Some timing: AMD Athlon 5400+

Comparing "E56.Language" <-> "languange" (0.711348), 1,000,000 times:

    Java (OpenJDK 1.6.0_20):	1.89 sec
    C (Gcc -O2)			0.72 sec
    Through Prolog		1.05 sec
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
 * @author Giorgos Stoilos
 *
 * This class implements the string matching method proposed in the paper
 * "A String Metric For Ontology Alignment", published in ISWC 2005
 *
 * Jérōme Euzenat: added normalization
 */

#include <config.h>
#define _GNU_SOURCE			/* get wcsdup */
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>
#include "isub.h"

#if defined(__sun) || __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 1070
#undef HAVE_WCSDUP
#endif

#ifndef HAVE_WCSDUP
static wchar_t *
my_wcsdup(const wchar_t *in)
{ wchar_t *copy = malloc((wcslen(in)+1)*sizeof(wchar_t));

  if ( copy )
    return wcscpy(copy, in);

  return NULL;
}
#define wcsdup(ws) my_wcsdup(ws)
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define DEBUG(g) (void)0

static void
toLowerCase(wchar_t *s)
{ wchar_t *q;

  for(q=s; *q; q++)
  { wint_t c = *q;

    if ( iswupper(c) )
    { *q = towlower(c);
    }
  }
}


static void
normalizeString(wchar_t *s, wint_t remo)
{ wchar_t *q, *o;

  for(o=q=s; *q; q++)
  { if ( *q != remo )
      *o++ = *q;
  }

  *o = 0;
}


static size_t
common_prefix_length(const wchar_t *s1, const wchar_t *s2)
{ size_t i;
  size_t n = MIN(wcslen(s1), wcslen(s2));

  for (i = 0; i < n; i++)
  { if (s1[i] != s2[i])
      break;
  }

  return i;
}


double
score_inplace(wchar_t *s1, wchar_t *s2, int normaliseStrings )
{ int l1, l2, L1, L2;
  double common = 0.0;
  size_t common_prefix_len;
  int best = 2;

  if ( normaliseStrings )
  { toLowerCase(s1);
    toLowerCase(s2);

    normalizeString(s1, '.');
    normalizeString(s2, '.');
    normalizeString(s1, '_');
    normalizeString(s2, '_');
    normalizeString(s1, ' ');
    normalizeString(s2, ' ');
  }

  common_prefix_len = common_prefix_length(s1, s2);
  l1 = (int)wcslen(s1);	// length of s
  l2 = (int)wcslen(s2);	// length of t
  L1 = l1;
  L2 = l2;
  if ((L1 == 0) && (L2 == 0))
    return 1.0;

    // Modification JE: giorgos put -1 instead of 0
  if ((L1 == 0) || (L2 == 0))
    return 0;

  while ( l1 > 0 && l2 > 0 && best != 0)
  { int i = 0;			// iterates through s1
    int j = 0;			// iterates through s2
    int startS2 = 0;
    int endS2 = 0;
    int startS1 = 0;
    int endS1 = 0;
    int p = 0;

    best = 0;			// the best subs length so far

    for (i = 0; (i < l1) && (l1 - i > best); i++)
    { j = 0;

      while (l2 - j > best)
      { int k = i;

	for (; (j < l2) && (s1[k] != s2[j]); j++)
	  ;
	if (j != l2)
	{			// we have found a starting point
	  p = j;
	  for ( j++, k++;
		(j < l2) && (k < l1) && (s1[k] == s2[j]);
		j++, k++ );
	  if (k - i > best)
	  { best = k - i;
	    startS1 = i;
	    endS1 = k;
	    startS2 = p;
	    endS2 = j;
	  }
	}
      }
    }

    DEBUG(wprintf(L"%ls(%d) %d..%d; %ls(%d) %d..%d -->",
		  s1, l1, startS1, endS1,
		  s2, l2, startS2, endS2));

    memmove(&s1[startS1], &s1[endS1], (l1+1-endS1)*sizeof(wchar_t));
    memmove(&s2[startS2], &s2[endS2], (l2+1-endS2)*sizeof(wchar_t));
    l1 -= endS1-startS1;
    l2 -= endS2-startS2;

    if (best > 2)
      common += best;
    else
      best = 0;
  }

  { double scaledCommon = (double) (2.0 * common) / (double)(L1 + L2);
    double commonality = scaledCommon;
    double dissimilarity = 0.0;
    double rest1 = (double)L1 - common;
    double rest2 = (double)L2 - common;
    double unmatchedS1 = rest1 / (double)L1;
    double unmatchedS2 = rest2 / (double)L2;
    double result;

		/**
		 * Hamacher Product
		 */
    double suma = unmatchedS1 + unmatchedS2;
    double product = unmatchedS1 * unmatchedS2;
    double p = 0.6;		//For 1 it coincides with the algebraic product

    double winklerImprovementVal =
	      (double)MIN(4,common_prefix_len)*0.1*(1.0-commonality);

    if ((suma - product) == 0)
      dissimilarity = 0;
    else
      dissimilarity = (product) / (p + (1 - p) * (suma - product));

    // Modification JE: returned normalization (instead of [-1 1])
    result = commonality - dissimilarity + winklerImprovementVal;

    return (result + 1) / 2;
  }
}


double
score(const wchar_t *st1, const wchar_t *st2, int normalizeString)
{ wchar_t *s1 = wcsdup(st1);
  wchar_t *s2 = wcsdup(st2);
  double rc;

  rc = score_inplace(s1, s2, normalizeString);
  free(s1);
  free(s2);

  return rc;
}


#ifdef STAND_ALONE

int
main(int argc, char **argv)
{ mbstate_t state;
  wchar_t ws1[1024];
  wchar_t ws2[1024];
  const char *s1 = argv[1];
  const char *s2 = argv[2];
  size_t l1, l2;

  memset(&state, 0, sizeof(state));

  l1 = mbsrtowcs(ws1, &s1, 1024, &state);
  l2 = mbsrtowcs(ws2, &s2, 1024, &state);

  double total=0.0;
  int i;
  for(i=0; i<1000000; i++)
    total += score(ws1, ws2);

  total = score(ws1, ws2);
  wprintf(L"%ls %ls: %f\n", ws1, ws2, total);

  return 0;
}

#endif
