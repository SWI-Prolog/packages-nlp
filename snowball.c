/*  Part of SWI-Prolog

    Author:        Jan Wielemaker
    E-mail:        J.Wielemaker@vu.nl
    WWW:           http://www.swi-prolog.org
    Copyright (c)  2010-2017, VU University Amsterdam
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in
       the documentation and/or other materials provided with the
       distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/

#include <config.h>
#include <SWI-Prolog.h>
#include <SWI-Stream.h>
#include "libstemmer_c/include/libstemmer.h"
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define STEMMER_BUCKETS (32)		/* cache CACHE_SIZE languages */

typedef struct stemmer
{ atom_t		language;
  struct stemmer       *next;
  struct sb_stemmer    *stemmer;
} stemmer;

typedef struct
{ stemmer	*stemmers[STEMMER_BUCKETS];
} stem_cache;

static pthread_key_t stem_key;
#ifndef __WINDOWS__
static pthread_once_t stem_key_once = PTHREAD_ONCE_INIT;
#endif

static void
stem_destroy_cache(void *buf)
{ stem_cache *cache = buf;
  int i;

  for(i=0; i<STEMMER_BUCKETS; i++)
  { stemmer *s = cache->stemmers[i];
    stemmer *n;

    for( ; s; s = n)
    { n = s->next;

      PL_unregister_atom(s->language);
      sb_stemmer_delete(s->stemmer);
      PL_free(s);
    }
  }

  PL_free(cache);
}

static void
stem_key_alloc(void)
{ pthread_key_create(&stem_key, stem_destroy_cache);
}

static stem_cache *
get_cache(void)
{ stem_cache *cache;

#ifndef __WINDOWS__
  pthread_once(&stem_key_once, stem_key_alloc);
#endif

  if ( (cache=(stem_cache*)pthread_getspecific(stem_key)) )
    return cache;
  if ( (cache = PL_malloc(sizeof(stem_cache))) )
    memset(cache, 0, sizeof(*cache));

  pthread_setspecific(stem_key, cache);

  return cache;
}


#define ATOM_HASH(a) ((unsigned int)(a>>7) & (STEMMER_BUCKETS-1))

static int
get_lang_stemmer(term_t t, struct sb_stemmer **stemmerp)
{ stem_cache *cache = get_cache();
  atom_t lang;
  int k;
  stemmer *s;
  struct sb_stemmer *st;
  const char *lname;

  if ( !PL_get_atom(t, &lang) )
    return PL_type_error("atom", t);

  k = ATOM_HASH(lang);
  for(s=cache->stemmers[k]; s; s=s->next)
  { if ( s->language == lang )
    { *stemmerp = s->stemmer;
      return TRUE;
    }
  }

  if ( !(lname=PL_atom_chars(lang)) ||
       !(st=sb_stemmer_new(lname, NULL)) )
  { if ( errno == ENOMEM )
      return PL_resource_error("memory");
    else
      return PL_domain_error("snowball_algorithm", t);
  }

  s = PL_malloc(sizeof(*s));
  s->language = lang;
  s->stemmer = st;
  PL_register_atom(lang);

  s->next = cache->stemmers[k];
  cache->stemmers[k] = s;

  *stemmerp = st;
  return TRUE;
}


static foreign_t
snowball(term_t lang, term_t in, term_t out)
{ struct sb_stemmer *stemmer = NULL;
  char *s;
  size_t len, olen;
  const sb_symbol *stemmed;

  if ( !get_lang_stemmer(lang, &stemmer) )
    return FALSE;
  if ( !PL_get_nchars(in, &len, &s,
		      CVT_ATOM|CVT_STRING|CVT_LIST|REP_UTF8|CVT_EXCEPTION) )
    return FALSE;

  if ( !(stemmed = sb_stemmer_stem(stemmer, (const sb_symbol*)s, (int)len)) )
    return PL_resource_error("memory");
  olen = sb_stemmer_length(stemmer);

  return PL_unify_chars(out, PL_ATOM|REP_UTF8, olen, (const char*)stemmed);
}


static foreign_t
snowball_algorithms(term_t list)
{ term_t tail = PL_copy_term_ref(list);
  term_t head = PL_new_term_ref();
  const char **algos = sb_stemmer_list();
  int i;

  for(i=0; algos[i]; i++)
  { if ( !PL_unify_list(tail, head, tail) ||
	 !PL_unify_atom_chars(head, algos[i]) )
      return FALSE;
  }

  return PL_unify_nil(tail);
}

install_t
install_snowball(void)
{ assert(sizeof(sb_symbol) == sizeof(char));

  PL_register_foreign("snowball", 3, snowball, 0);
  PL_register_foreign("snowball_algorithms", 1, snowball_algorithms, 0);

#ifdef __WINDOWS__
  stem_key_alloc();
#endif
}
