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
	    [porter_stem/2,tokenize_atom/2,atom_to_stem_list/2,unaccent_atom/2]).
:- autoload(library(lists), [append/2, append/3]).
:- autoload(library(apply), [maplist/2]).
:- autoload(library(time), [call_with_time_limit/2]).
:- autoload(library(snowball)).
% Not autoload: the goal expansion of isub/4 must be in effect while the
% tests below are compiled.
:- use_module(library(isub), [isub/4]).

test_nlp :-
    run_tests([ stem,
                metaphone,
                snowball,
                isub
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
test(unaccent, X == ssssssation) :-
    porter_stem('\u00DF\u00DF\u00DFations', X).
test(unaccent, X == aebl) :-
    porter_stem('\u00C6bles', X).
test(unaccent, X == ecol) :-
    porter_stem('\u00C9COLES', X).
test(unaccent, X == [aebl]) :-
    atom_to_stem_list('\u00C6bles', X).
test(unaccent, X == 'AEbles') :-
    unaccent_atom('\u00C6bles', X).
test(unaccent, X == abc) :-
    unaccent_atom(abc, X).
test(unaccent_long, Len == 3000) :-
    length(Codes, 1500),
    maplist(=(0xC6), Codes),
    atom_codes(A, Codes),
    unaccent_atom(A, U),
    atom_length(U, Len).
test(stem_long) :-
    % Used to take quadratic time on sequences of y
    length(Ys, 1000000),
    maplist(=(0'y), Ys),
    append([`a`, Ys, `ing`], Codes),
    atom_codes(A, Codes),
    call_with_time_limit(10, porter_stem(A, S)),
    atom_length(S, Len),
    assertion(Len == 1000001).

:- end_tests(stem).

:- begin_tests(metaphone).

test(metaphone, [true(X=='ARLT')]) :-
    double_metaphone(world, X).

:- end_tests(metaphone).


:- begin_tests(isub).

%  isub/4 is overloaded: the third argument is either the `Normalize`
%  boolean or the similarity, in which case the fourth argument holds the
%  options.  Both forms are goal expanded.  Compare each expanded call
%  with the same call that cannot be expanded because its mode only
%  becomes clear at runtime.  An error while expanding used to drop the
%  clause holding the call.

test(normalize, Expanded =:= Runtime) :-
    isub(hello, hallo, false, Expanded),
    normalize(Normalize),
    isub(hello, hallo, Normalize, Runtime).

test(options, Expanded =:= Runtime) :-
    isub(hello, hallo, Expanded, [normalize(true)]),
    options(Options),
    isub(hello, hallo, Runtime, Options).

normalize(false).
options([normalize(true)]).

:- end_tests(isub).


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
