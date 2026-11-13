/* $Id$

   This is the Porter stemming algorithm, coded up in ANSI C by the
   author. It may be be regarded as canonical, in that it follows the
   algorithm presented in

   Porter, 1980, An algorithm for suffix stripping, Program, Vol. 14,
   no. 3, pp 130-137,

   only differing from it at the points maked --DEPARTURE-- below.

   See also http://www.muscat.com/~martin/stem.html

   The algorithm as described in the paper could be exactly replicated
   by adjusting the points of DEPARTURE, but this is barely necessary,
   because (a) the points of DEPARTURE are definitely improvements, and
   (b) no encoding of the Porter stemmer I have seen is anything like
   as exact as this version, even with the points of DEPARTURE!

   You can compile it on Unix with 'gcc -O3 -o stem stem.c' after which
   'stem' takes a list of inputs and sends the stemmed equivalent to
   stdout.

   The algorithm as encoded here is particularly fast.

   Release 1
*/

#include <config.h>
#include <SWI-Prolog.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <wctype.h>
#include <wchar.h>

/* The main part of the stemming algorithm starts here. b is a buffer
   holding a word to be stemmed. The letters are in b[k0], b[k0+1] ...
   ending at b[k]. In fact k0 = 0 in this demo program. k is readjusted
   downwards as the stemming progresses. Zero termination is not in fact
   used in the algorithm.

   Note that only lower case sequences are stemmed. Forcing to lower case
   should be done before stem(...) is called.
*/

typedef struct vars
{ char *b;		/* work to be stemmed */
  int   k, k0, j;	/* j is a general offset into the string */
} vars;

/* cons(i) is true <=> vs->b[i] is a consonant. */

static bool
cons(int i, vars *vs)
{ int neg = false;			/* y: cons(i) = !cons(i-1) */

  for(;;)
  { switch (vs->b[i])
    { case 'a': case 'e': case 'i': case 'o': case 'u':
	return neg;
      case 'y':
	if ( i == vs->k0 )
	  return !neg;
	neg = !neg;
	i--;
	break;
      default:
	return !neg;
    }
  }
}

/* cons_next(i, prev) is cons(i), given prev is cons(i-1).  Used to
   compute cons() for a sequence in linear time. */

static int
cons_next(int i, int prev, vars *vs)
{ switch (vs->b[i])
  { case 'a': case 'e': case 'i': case 'o': case 'u': return false;
    case 'y': return (i==vs->k0) ? true : !prev;
    default: return true;
  }
}

/* m() measures the number of consonant sequences between vs->k0 and vs->j. if c is
   a consonant sequence and v a vowel sequence, and <..> indicates arbitrary
   presence,

      <c><v>       gives 0
      <c>vc<v>     gives 1
      <c>vcvc<v>   gives 2
      <c>vcvcvc<v> gives 3
      ....
*/

static int
m(vars *vs)
{ int n = 0;
  int prev = true;

  for(int i = vs->k0; i <= vs->j; i++)
  { int c = cons_next(i, prev, vs);

    if ( c && !prev )			/* vowel followed by consonant */
      n++;
    prev = c;
  }

  return n;
}

/* vowelinstem() is true <=> vs->k0,...vs->j contains a vowel */

static bool
vowelinstem(vars *vs)
{ int prev = true;

  for(int i = vs->k0; i <= vs->j; i++)
  { if ( !(prev=cons_next(i, prev, vs)) )
      return true;
  }

  return false;
}

/* doublec(vs->j) is true <=> vs->j,(vs->j-1) contain a double consonant. */

static bool
doublec(int j, vars *vs)
{  if (j < vs->k0+1) return false;
   if (vs->b[j] != vs->b[j-1]) return false;
   return cons(j, vs);
}

/* cvc(i) is true <=> i-2,i-1,i has the form consonant - vowel - consonant
   and also if the second c is not w,x or y. this is used when trying to
   restore an e at the end of a short word. e.g.

      cav(e), lov(e), hop(e), crim(e), but
      snow, box, tray.

*/

static bool
cvc(int i, vars *vs)
{  if (i < vs->k0+2 || !cons(i, vs) || cons(i-1, vs) || !cons(i-2, vs))
     return false;
   {  int ch = vs->b[i];
      if (ch == 'w' || ch == 'x' || ch == 'y') return false;
   }
   return true;
}

/* ends(s) is true <=> vs->k0,...vs->k ends with the string s. */

static bool
ends(char * s, vars *vs)
{  int length = s[0];
   if (s[length] != vs->b[vs->k]) return false; /* tiny speed-up */
   if (length > vs->k-vs->k0+1) return false;
   if (memcmp(vs->b+vs->k-length+1,s+1,length) != 0) return false;
   vs->j = vs->k-length;
   return true;
}

/* setto(s) sets (vs->j+1),...vs->k to the characters in the string s, readjusting
   vs->k. */

static void
setto(char * s, vars *vs)
{  int length = s[0];
   memmove(vs->b+vs->j+1,s+1,length);
   vs->k = vs->j+length;
}

/* r(s) is used further down. */

static void
r(char * s, vars *vs)
{ if (m(vs) > 0)
    setto(s, vs);
}

/* step1ab() gets rid of plurals and -ed or -ing. e.g.

       caresses  ->  caress
       ponies    ->  poni
       ties      ->  ti
       caress    ->  caress
       cats      ->  cat

       feed      ->  feed
       agreed    ->  agree
       disabled  ->  disable

       matting   ->  mat
       mating    ->  mate
       meeting   ->  meet
       milling   ->  mill
       messing   ->  mess

       meetings  ->  meet

*/

static void
step1ab(vars *vs)
{  if (vs->b[vs->k] == 's')
   {  if (ends("\04" "sses", vs)) vs->k -= 2; else
      if (ends("\03" "ies", vs)) setto("\01" "i", vs); else
      if (vs->b[vs->k-1] != 's') vs->k--;
   }
   if (ends("\03" "eed", vs)) { if (m(vs) > 0) vs->k--; } else
   if ((ends("\02" "ed", vs) || ends("\03" "ing", vs)) && vowelinstem(vs))
   {  vs->k = vs->j;
      if (ends("\02" "at", vs)) setto("\03" "ate", vs); else
      if (ends("\02" "bl", vs)) setto("\03" "ble", vs); else
      if (ends("\02" "iz", vs)) setto("\03" "ize", vs); else
      if (doublec(vs->k, vs))
      {  vs->k--;
         {  int ch = vs->b[vs->k];
            if (ch == 'l' || ch == 's' || ch == 'z') vs->k++;
         }
      }
      else if (m(vs) == 1 && cvc(vs->k, vs)) setto("\01" "e", vs);
  }
}

/* step1c() turns terminal y to i when there is another vowel in the stem. */

static void
step1c(vars *vs)
{ if (ends("\01" "y", vs) && vowelinstem(vs))
    vs->b[vs->k] = 'i';
}


/* step2() maps double suffices to single ones. so -ization ( = -ize plus
   -ation) maps to -ize etc. note that the string before the suffix must give
   m() > 0. */

static void step2(vars *vs)
{ if ( vs->k <= 0 )
    return;

  switch (vs->b[vs->k-1])
  {
    case 'a': if (ends("\07" "ational", vs)) { r("\03" "ate", vs); break; }
              if (ends("\06" "tional", vs)) { r("\04" "tion", vs); break; }
              break;
    case 'c': if (ends("\04" "enci", vs)) { r("\04" "ence", vs); break; }
              if (ends("\04" "anci", vs)) { r("\04" "ance", vs); break; }
              break;
    case 'e': if (ends("\04" "izer", vs)) { r("\03" "ize", vs); break; }
              break;
    case 'l': if (ends("\03" "bli", vs)) { r("\03" "ble", vs); break; } /*-DEPARTURE-*/

 /* To match the published algorithm, replace this line with
    case 'l': if (ends("\04" "abli", vs)) { r("\04" "able", vs); break; } */

              if (ends("\04" "alli", vs)) { r("\02" "al", vs); break; }
              if (ends("\05" "entli", vs)) { r("\03" "ent", vs); break; }
              if (ends("\03" "eli", vs)) { r("\01" "e", vs); break; }
              if (ends("\05" "ousli", vs)) { r("\03" "ous", vs); break; }
              break;
    case 'o': if (ends("\07" "ization", vs)) { r("\03" "ize", vs); break; }
              if (ends("\05" "ation", vs)) { r("\03" "ate", vs); break; }
              if (ends("\04" "ator", vs)) { r("\03" "ate", vs); break; }
              break;
    case 's': if (ends("\05" "alism", vs)) { r("\02" "al", vs); break; }
              if (ends("\07" "iveness", vs)) { r("\03" "ive", vs); break; }
              if (ends("\07" "fulness", vs)) { r("\03" "ful", vs); break; }
              if (ends("\07" "ousness", vs)) { r("\03" "ous", vs); break; }
              break;
    case 't': if (ends("\05" "aliti", vs)) { r("\02" "al", vs); break; }
              if (ends("\05" "iviti", vs)) { r("\03" "ive", vs); break; }
              if (ends("\06" "biliti", vs)) { r("\03" "ble", vs); break; }
              break;
    case 'g': if (ends("\04" "logi", vs)) { r("\03" "log", vs); break; } /*-DEPARTURE-*/

 /* To match the published algorithm, delete this line */
  }
}

/* step3() deals with -ic-, -full, -ness etc. similar strategy to step2. */

static void
step3(vars *vs)
{ switch (vs->b[vs->k])
  {
    case 'e': if (ends("\05" "icate", vs)) { r("\02" "ic", vs); break; }
              if (ends("\05" "ative", vs)) { r("\00" "", vs); break; }
              if (ends("\05" "alize", vs)) { r("\02" "al", vs); break; }
              break;
    case 'i': if (ends("\05" "iciti", vs)) { r("\02" "ic", vs); break; }
              break;
    case 'l': if (ends("\04" "ical", vs)) { r("\02" "ic", vs); break; }
              if (ends("\03" "ful", vs)) { r("\00" "", vs); break; }
              break;
    case 's': if (ends("\04" "ness", vs)) { r("\00" "", vs); break; }
              break;
  }
}

/* step4() takes off -ant, -ence etc., in context <c>vcvc<v>. */

static void step4(vars *vs)
{ if ( vs->k <= 0 )
    return;
  switch (vs->b[vs->k-1])
    {  case 'a': if (ends("\02" "al", vs)) break; return;
       case 'c': if (ends("\04" "ance", vs)) break;
                 if (ends("\04" "ence", vs)) break;
		 return;
       case 'e': if (ends("\02" "er", vs)) break;
		 return;
       case 'i': if (ends("\02" "ic", vs)) break;
		 return;
       case 'l': if (ends("\04" "able", vs)) break;
                 if (ends("\04" "ible", vs)) break;
		 return;
       case 'n': if (ends("\03" "ant", vs)) break;
                 if (ends("\05" "ement", vs)) break;
                 if (ends("\04" "ment", vs)) break;
                 if (ends("\03" "ent", vs)) break;
		 return;
       case 'o': if (ends("\03" "ion", vs) && vs->j > 0 &&
		     (vs->b[vs->j] == 's' || vs->b[vs->j] == 't'))
		   break;
                 if (ends("\02" "ou", vs)) break;
		 return;
                 /* takes care of -ous */
       case 's': if (ends("\03" "ism", vs)) break;
		 return;
       case 't': if (ends("\03" "ate", vs)) break;
                 if (ends("\03" "iti", vs)) break;
		 return;
       case 'u': if (ends("\03" "ous", vs)) break;
		 return;
       case 'v': if (ends("\03" "ive", vs)) break;
		 return;
       case 'z': if (ends("\03" "ize", vs)) break;
		 return;
       default: return;
    }
    if (m(vs) > 1) vs->k = vs->j;
}

/* step5() removes a final -e if m() > 1, and changes -ll to -l if
   m() > 1. */

static void step5(vars *vs)
{  vs->j = vs->k;
   if (vs->b[vs->k] == 'e')
   {  int a = m(vs);
      if (a > 1 || (a == 1 && !cvc(vs->k-1, vs))) vs->k--;
   }
   if (vs->b[vs->k] == 'l' && doublec(vs->k, vs) && m(vs) > 1) vs->k--;
}

/* In stem(p,i,vs->j), p is a char pointer, and the string to be stemmed is from
   p[i] to p[vs->j] inclusive. Typically i is zero and vs->j is the offset to the last
   character of a string, (p[vs->j+1] == '\0'). The stemmer adjusts the
   characters p[i] ... p[vs->j] and returns the new end-point of the string, vs->k.
   Stemming never increases word length, so i <= vs->k <= vs->j. To turn the stemmer
   into a module, declare 'stem' as extern, and delete the remainder of this
   file.
*/

static int
stem(char * p, int i, int j)
{ vars vs;

  vs.b = p; vs.k = j; vs.k0 = i;
  if ( vs.k <= vs.k0+1) return vs.k; /*-DEPARTURE-*/

   /* With this line, strings of length 1 or 2 don't go through the
      stemming process, although no mention is made of this in the
      published algorithm. Remove the line to match the published
      algorithm. */

  step1ab(&vs); step1c(&vs); step2(&vs); step3(&vs); step4(&vs); step5(&vs);
  return vs.k;
}

/*--------------------stemmer definition ends here------------------------*/

/* SWI-Prolog hooks */

static size_t unaccent(const char *in, size_t len,
		       char *out, size_t size, int *changes);

/* unaccent_lower() unaccents and downcases len bytes from in.  The
   result is in buf if it fits, else in malloc'ed memory.  Returns the
   result and its length in *lenp.
*/

static char *
unaccent_lower(const char *in, size_t len, char *buf, size_t size,
	       size_t *lenp)
{ int changes;
  size_t l = unaccent(in, len, buf, size, &changes);
  char *s = buf;

  if ( l > size )
  { if ( !(s = malloc(l)) )
      return NULL;
    unaccent(in, len, s, l, &changes);
  }
  for(size_t i=0; i<l; i++)
    s[i] = (char)tolower(s[i]&0xff);

  *lenp = l;
  return s;
}

/* stem_chars() stems s[0..len) in place, returning the new length.
   Words that do not fit an int are not stemmed. */

static size_t
stem_chars(char *s, size_t len)
{ if ( len == 0 || len > INT_MAX )
    return len;

  return (size_t)stem(s, 0, (int)(len-1)) + 1;
}

static foreign_t
pl_stem(term_t t_in, term_t t_stem)
{ char *word, *s;
  size_t len, l;
  char buf[1024];
  int rc;

  if ( !PL_get_nchars(t_in, &len, &word, CVT_ALL|CVT_EXCEPTION) )
  { if ( PL_is_number(t_in) )
      return PL_unify(t_in, t_stem);
    return false;
  }

  if ( !(s = unaccent_lower(word, len, buf, sizeof(buf), &l)) )
    return PL_resource_error("memory");
  rc = PL_unify_atom_nchars(t_stem, stem_chars(s, l), s);
  if ( s != buf )
    free(s);

  return rc;
}


		 /*******************************
		 *	       ACCENTS		*
		 *******************************/

/* See also http://www.ahinea.com/en/tech/accented-translate.html
*/

static char *unaccent_def[] =
{ "A",	/* 192 */ "A", /* 193 */ "A",	/* 194 */ "A",	/* 195 */
  "A",	/* 196 */ "A", /* 197 */ "AE",	/* 198 */ "C",	/* 199 */
  "E",	/* 200 */ "E", /* 201 */ "E",	/* 202 */ "E",	/* 203 */
  "I",	/* 204 */ "I", /* 205 */ "I",	/* 206 */ "I",	/* 207 */
  "D",	/* 208 */ "N", /* 209 */ "O",	/* 210 */ "O",	/* 211 */
  "O",	/* 212 */ "O", /* 213 */ "O",	/* 214 */ NULL,	/* 215 */
  NULL,	/* 216 */ "U", /* 217 */ "U",	/* 218 */ "U",	/* 219 */
  "U",	/* 220 */ "Y", /* 221 */ NULL,	/* 222 */ "ss",	/* 223 */
  "a",	/* 224 */ "a", /* 225 */ "a",	/* 226 */ "a",	/* 227 */
  "a",	/* 228 */ "a", /* 229 */ "ae",	/* 230 */ "c",	/* 231 */
  "e",	/* 232 */ "e", /* 233 */ "e",	/* 234 */ "e",	/* 235 */
  "i",	/* 236 */ "i", /* 237 */ "i",	/* 238 */ "i",	/* 239 */
  "d",	/* 240 */ "n", /* 241 */ "o",	/* 242 */ "o",	/* 243 */
  "o",	/* 244 */ "o", /* 245 */ "o",	/* 246 */ NULL,	/* 247 */
  NULL,	/* 248 */ "u", /* 249 */ "u",	/* 250 */ "u",	/* 251 */
  "u",	/* 252 */ "y", /* 253 */ NULL,	/* 254 */ "y",	/* 255 */
};


/* unaccent() writes the unaccented version of in[0..len) to out,
   writing at most size bytes.  Returns the length of the result, which
   does not fit if this is more than size.  *changes is set to the
   number of replaced characters.  The output is not 0-terminated. */

static size_t
unaccent(const char *in, size_t len, char *out, size_t size, int *changes)
{ char *to = out, *toe = &out[size];
  const char *ein = &in[len];

  *changes = 0;

  for( ; in < ein; in++)
  { int c = (*in)&0xff;
    char *m;

    if ( c < 192 || !(m=unaccent_def[c-192]) )
    { if ( to < toe )
	*to = c;
      to++;
    } else
    { (*changes)++;

      while(*m)
      { if ( to < toe )
	  *to = *m;
	to++;
	m++;
      }
    }
  }

  return (size_t)(to-out);
}


static foreign_t
pl_unaccent(term_t from, term_t to)
{ char buf[1024];
  char *f;
  size_t len, fl;
  int changes;

  if ( !PL_get_nchars(from, &fl, &f, CVT_ALL|CVT_EXCEPTION) )
    return false;

  len = unaccent(f, fl, buf, sizeof(buf), &changes);
  if ( changes == 0 )
  { return PL_unify(to, from);
  } else if ( len <= sizeof(buf) )
  { return PL_unify_atom_nchars(to, len, buf);
  } else
  { char *t = malloc(len);
    int rc;

    if ( !t )
      return PL_resource_error("memory");
    unaccent(f, fl, t, len, &changes);
    rc = PL_unify_atom_nchars(to, len, t);
    free(t);
    return rc;
  }
}


		 /*******************************
		 *	     TOKENISE		*
		 *******************************/

typedef enum
{ TOK_INT,
  TOK_FLOAT,
  TOK_WORD,
  TOK_PUNCT,
  TOK_UNKNOWN
} toktype;

#undef isdigit
#define issign(c) ((c) == '-' || (c) == '+' )
#define isdigit(c) ((c) >= '0' && (c) <= '9')

static bool
tokenizeA(const char *in, size_t len,
	  int (*call)(const char *s,
		      size_t len,
		      toktype type,
		      void *closure),
	  void *closure)
{ const unsigned char *s = (const unsigned char*)in;
  const unsigned char *se = &s[len];
  toktype type;

  while(s<se)
  { const unsigned char *st;		/* start token */

    while(s<se && iswspace(*s))		/* skip blanks */
      s++;
    if ( s >= se )
      break;

    st = s;
    type = TOK_UNKNOWN;

    if ( *s == '-' && se-s > 1 && isdigit(s[1]) )
    { s += 2;
      type = TOK_INT;
    } else if ( isdigit(*s) )
    { s++;
      type = TOK_INT;
    }

    if ( type == TOK_INT )
    { while(s<se && isdigit(*s))
	s++;
      if ( s+2 <= se && *s == '.' && isdigit(s[1]) )
      { s += 2;
	type = TOK_FLOAT;
	while(s<se && isdigit(*s))
	  s++;
      }
      if ( s+2 <= se &&
	   (*s == 'e' || *s == 'E') &&
	   (isdigit(s[1]) || (s+3 <= se && issign(s[1]) && isdigit(s[2]))) )
      { s += 2;
	type = TOK_FLOAT;
	while(s<se && isdigit(*s))
	  s++;
      }

      if ( !(*call)((const char*)st, s-st, type, closure) )
      { if ( PL_exception(0) )
	  return false;
	while(s<se && iswalnum(*s))
	  s++;
	if ( !(*call)((const char*)st, s-st, TOK_WORD, closure) )
	  return false;
      }
    } else if ( iswalnum(*s) )
    { while(s<se && iswalnum(*s))
	s++;
      if ( !(*call)((const char*)st, s-st, TOK_WORD, closure) )
	return false;
    } else
    { s++;
      if ( !(*call)((const char*)st, 1, TOK_PUNCT, closure) )
	return false;
    }
  }

  return true;
}


static bool
tokenizeW(const wchar_t *in, size_t len,
	  int (*call)(const wchar_t *s,
		      size_t len,
		      toktype type,
		      void *closure),
	 void *closure)
{ const wchar_t *s = (const wchar_t*)in;
  const wchar_t *se = &s[len];
  toktype type;

  while(s<se)
  { const wchar_t *st;			/* start token */

    while(s<se && iswspace(*s))		/* skip blanks */
      s++;
    if ( s >= se )
      break;

    st = s;
    type = TOK_UNKNOWN;

    if ( *s == '-' && se-s > 1 && isdigit(s[1]) )
    { s += 2;
      type = TOK_INT;
    } else if ( isdigit(*s) )
    { s++;
      type = TOK_INT;
    }

    if ( type == TOK_INT )
    { while(s<se && isdigit(*s))
	s++;
      if ( s+2 <= se && *s == '.' && isdigit(s[1]) )
      { s += 2;
	type = TOK_FLOAT;
	while(s<se && isdigit(*s))
	  s++;
      }
      if ( s+2 <= se &&
	   (*s == 'e' || *s == 'E') &&
	   (isdigit(s[1]) || (s+3 <= se && issign(s[1]) && isdigit(s[2]))) )
      { s += 2;
	type = TOK_FLOAT;
	while(s<se && isdigit(*s))
	  s++;
      }

      if ( !(*call)((const wchar_t*)st, s-st, type, closure) )
      { if ( PL_exception(0) )
	  return false;
	while(s<se && iswalnum(*s))
	  s++;
	if ( !(*call)((const wchar_t*)st, s-st, TOK_WORD, closure) )
	  return false;
      }

    } else if ( iswalnum(*s) )
    { while(s<se && iswalnum(*s))
	s++;
      if ( !(*call)((const wchar_t*)st, s-st, TOK_WORD, closure) )
	return false;
    } else
    { s++;
      if ( !(*call)((const wchar_t*)st, 1, TOK_PUNCT, closure) )
	return false;
    }
  }

  return true;
}


typedef struct
{ term_t head;
  term_t tail;
  term_t tmp;
} list;


static int
unify_tokenA(const char *s, size_t len, toktype type, void *closure)
{ list *l = closure;

  switch(type)
  { case TOK_INT:
    case TOK_FLOAT:
    { char buf[100];
      char *a, *o;
      const char *i;
      int rc;

      if ( len+1 > sizeof(buf) )
      { if ( !(a = malloc(len+1)) )
	  return PL_resource_error("memory");
      } else
      { a = buf;
      }

      for(i=s,o=a; len-- > 0; )
	*o++ = (char)*i++;
      *o = '\0';

      rc = PL_chars_to_term(a, l->tmp);
      if ( a != buf )
	free(a);
      if ( !rc )
	return rc;
      break;
    }
    default:
      if ( !PL_put_atom_nchars(l->tmp, len, s) )
	return false;
    break;
  }

  return ( PL_unify_list(l->tail, l->head, l->tail) &&
	   PL_unify(l->head, l->tmp) );
}


static int
unify_tokenW(const wchar_t *s, size_t len, toktype type, void *closure)
{ list *l = closure;

  switch(type)
  { case TOK_INT:
    case TOK_FLOAT:
    { char buf[100];
      char *a, *o;
      const wchar_t *i;
      int rc;

      if ( len+1 > sizeof(buf) )
      { if ( !(a = malloc(len+1)) )
	  return PL_resource_error("memory");
      } else
      { a = buf;
      }

      for(i=s,o=a; len-- > 0; )
	*o++ = (char)*i++;
      *o = '\0';

      rc = PL_chars_to_term(a, l->tmp);

      if ( a != buf )
	free(a);
      if ( !rc )
	return rc;
      break;
    }
    default:
      if ( !PL_put_variable(l->tmp) ||
	   !PL_unify_wchars(l->tmp, PL_ATOM, len, s) )
	return false;
      break;
  }

  return ( PL_unify_list(l->tail, l->head, l->tail) &&
	   PL_unify(l->head, l->tmp) );
}



static foreign_t
pl_tokenize(term_t text, term_t tokens)
{ char *s;
  wchar_t *ws;
  size_t len;
  list l;

  l.tail = PL_copy_term_ref(tokens);
  l.head = PL_new_term_ref();
  l.tmp  = PL_new_term_ref();

  if ( PL_get_nchars(text, &len, &s, CVT_ALL) )
  { if ( !tokenizeA(s, len, unify_tokenA, &l) )
      return false;
  } else if ( PL_get_wchars(text, &len, &ws, CVT_ALL|CVT_EXCEPTION) )
  { if ( !tokenizeW(ws, len, unify_tokenW, &l) )
      return false;
  } else
    return false;

  return PL_unify_nil(l.tail);
}


static int
unify_stem(const char *s, size_t len, toktype type, void *closure)
{ list *list = closure;

  if ( type == TOK_PUNCT )
    return true;
  if ( type == TOK_INT || type == TOK_FLOAT )
    return unify_tokenA(s, len, type, closure);

  if ( PL_unify_list(list->tail, list->head, list->tail) )
  { char tmp[1024];
    char *buf;
    size_t l;
    int rc;

    if ( !(buf = unaccent_lower(s, len, tmp, sizeof(tmp), &l)) )
      return PL_resource_error("memory");
    rc = PL_unify_atom_nchars(list->head, stem_chars(buf, l), buf);
    if ( buf != tmp )
      free(buf);

    return rc;
  }

  return false;
}



static foreign_t
pl_atom_to_stem_list(term_t text, term_t stems)
{ char *s;
  size_t len;
  list l;

  if ( !PL_get_nchars(text, &len, &s, CVT_ALL) )
    return false;

  l.tail = PL_copy_term_ref(stems);
  l.head = PL_new_term_ref();
  l.tmp  = PL_new_term_ref();

  if ( !tokenizeA(s, len, unify_stem, &l) )
    return false;

  return PL_unify_nil(l.tail);
}




		 /*******************************
		 *	      INSTALL		*
		 *******************************/

install_t
install_porter_stem()
{ PL_register_foreign("porter_stem",       2, pl_stem,     0);
  PL_register_foreign("unaccent_atom",	   2, pl_unaccent, 0);
  PL_register_foreign("tokenize_atom",	   2, pl_tokenize, 0);
  PL_register_foreign("atom_to_stem_list", 2, pl_atom_to_stem_list, 0);
}


/* end of SWI-Prolog */
