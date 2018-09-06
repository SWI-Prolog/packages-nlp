cmake_minimum_required(VERSION 2.8.12)
project(libstemmer)

set(STEMMER_SOURCE
    src_c/stem_UTF_8_danish.c
    src_c/stem_UTF_8_dutch.c
    src_c/stem_UTF_8_english.c
    src_c/stem_UTF_8_finnish.c
    src_c/stem_UTF_8_french.c
    src_c/stem_UTF_8_german.c
    src_c/stem_UTF_8_hungarian.c
    src_c/stem_UTF_8_italian.c
    src_c/stem_UTF_8_norwegian.c
    src_c/stem_UTF_8_porter.c
    src_c/stem_UTF_8_portuguese.c
    src_c/stem_UTF_8_romanian.c
    src_c/stem_UTF_8_russian.c
    src_c/stem_UTF_8_spanish.c
    src_c/stem_UTF_8_swedish.c
    src_c/stem_UTF_8_turkish.c
    runtime/api.c
    runtime/utilities.c
    libstemmer/libstemmer_utf8.c)

add_library(libstemmer STATIC ${STEMMER_SOURCE})
set_property(TARGET libstemmer PROPERTY
	     POSITION_INDEPENDENT_CODE ON)
