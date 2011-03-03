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

#include <SWI-Prolog.h>
#include "isub.h"

static foreign_t
pl_isub(term_t t1, term_t t2, term_t normalize, term_t sim)
{ wchar_t *s1=NULL, *s2=NULL;
  int rc;
  int normaliseStrings;

  if ( !PL_get_wchars(t1, NULL, &s1, CVT_ATOMIC|CVT_EXCEPTION|BUF_MALLOC) ||
       !PL_get_wchars(t2, NULL, &s2, CVT_ATOMIC|CVT_EXCEPTION|BUF_MALLOC) ||
       !PL_get_bool_ex(normalize, &normaliseStrings) )
  { rc = FALSE;
    goto out;
  }

  rc = PL_unify_float(sim, score_inplace(s1, s2, normaliseStrings));

out:
  if ( s1 ) PL_free(s1);
  if ( s2 ) PL_free(s2);

  return rc;
}


install_t
install_isub()
{ PL_register_foreign("isub", 4, pl_isub, 0);
}
