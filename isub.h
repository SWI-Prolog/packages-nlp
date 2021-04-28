#ifndef ISUB_H_INCLUDED
#define ISUB_H_INCLUDED

#include <wchar.h>

double isub_score_inplace(wchar_t *s1, wchar_t *s2, int options, int substring_threshold);
double isub_score(const wchar_t *st1, const wchar_t *st2, int options, int substring_threshold);

#endif /*ISUB_H_INCLUDED*/
