/*  Part of SWI-Prolog

    Author:        Giorgos Stoilos and Jan Wielemaker

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, if you link this library with other files,
    compiled with a Free Software compiler, to produce an executable, this
    library does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

:- module(isub,
	  [ isub/4			% +Text1, +Text2, +Normalize, -Distance
	  ]).

:- use_foreign_library(foreign(isub)).

/** <module> isub: a string similarity measure

The library(isub) implements a similarity measure between strings, i.e.,
something similar to the _|Levenshtein distance|_.  This method is based
on the length of common substrings.

@author Giorgos Stoilos
@see	_|A string metric for ontology alignment|_ by Giorgos Stoilos,
	2005.
*/

%%	isub(+Text1:atomic, +Text2:atomic, +Normalize:bool,
%%	     -Similarity:float) is det.
%
%	Similarity is a measure for the distance between Text1 and
%	Text2.  E.g.
%
%	  ==
%	  ?- isub('E56.Language', 'languange', true, D).
%	  D = 0.711348.
%	  ==
%
%	If Normalize is =true=, isub/4   applies string normalization as
%	implemented by the original authors: Text1  and Text2 are mapped
%	to lowercase and the characters  "._   "  are removed. Lowercase
%	mapping is done  with  the   C-library  function  towlower(). In
%	general, the required normalization is   domain dependent and is
%	better left to the caller.  See e.g., unaccent_atom/2.
%
%	@param	Similarity is a float in the range [0.0..1.0], where 1.0
%		means _|most similar|_

:- multifile sandbox:safe_primitive/1.

sandbox:safe_primitive(isub:isub(_,_,_,_)).
