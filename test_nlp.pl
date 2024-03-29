/*  Part of SWI-Prolog

    Author:        Jan Wielemaker
    E-mail:        J.Wielemaker@vu.nl
    WWW:           http://www.swi-prolog.org
    Copyright (c)  2016-2023, VU University Amsterdam
                              CWI, Amsterdam
                              SWI-Prolog Solutions b.v.
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

:- module(test_nlp,
          [ test_nlp/0
          ]).
:- use_module(library(plunit)).
:- autoload(library(double_metaphone),[double_metaphone/2]).
:- autoload(library(porter_stem),
	    [porter_stem/2,tokenize_atom/2,atom_to_stem_list/2]).
:- autoload(library(snowball)).

test_nlp :-
    run_tests([ stem,
                metaphone,
                snowball
              ]).

:- begin_tests(stem).

test(stem, [true(X==walk)]) :-
    porter_stem(walks, X).
test(stem, [true(X==walk)]) :-
    porter_stem(walk, X).
test(tokens, [true(X==[hello, world, !])]) :-
    tokenize_atom('hello world!', X).
test(stem_list, [true(X==[hello, world])]) :-
    atom_to_stem_list('hello worlds!', X).

:- end_tests(stem).

:- begin_tests(metaphone).

test(metaphone, [true(X=='ARLT')]) :-
    double_metaphone(world, X).

:- end_tests(metaphone).


:- begin_tests(snowball).

:- if(exists_source('../sgml/iso_639')).
:- use_module('../sgml/iso_639').

test(snowball_cache, Pairs1 == Pairs2) :-
    X = wandelen,
    findall(Code-Stem, stem(Code, X, Stem), Pairs1),
    findall(Code-Stem, stem(Code, X, Stem), Pairs2),
    length(Pairs1, Len),
    assertion(Len > 10).

stem(Code, For, Stem) :-
    iso_639(Code, _Lang),
    catch(snowball(Code, For, Stem),
          error(domain_error(snowball_algorithm, Code), _),
          fail).

:- endif.

:- end_tests(snowball).
