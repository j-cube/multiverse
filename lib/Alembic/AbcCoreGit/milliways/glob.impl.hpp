/*
 * milliways - B+ trees and key-value store C++ library
 *
 * Author: Marco Pantaleoni <marco.pantaleoni@gmail.com>
 * Copyright (C) 2016-2017 Marco Pantaleoni. All rights reserved.
 *
 * Distributed under the Apache License, Version 2.0
 * See the NOTICE file distributed with this work for
 * additional information regarding copyright ownership.
 * The author licenses this file to you under the Apache
 * License, Version 2.0 (the "License"); you may not use
 * this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * This file is based on the original `glob.c` by Christopher Seiwald,
 * which is:
 *
 * Copyright 1994 Christopher Seiwald.  All rights reserved.
 *
 * Modifications are copyright by Marco Pantaleoni and subject to the
 * global product license (Apache License, Version 2.0).
 *
 * The original license message follows:
 *
 * License is hereby granted to use this software and distribute it
 * freely, as long as this copyright notice is retained and modifications 
 * are clearly marked.
 *
 * ALL WARRANTIES ARE HEREBY DISCLAIMED.
 */

/*
 * glob.c - match a string against a simple pattern
 *
 * Understands the following patterns:
 *
 *	*	any number of characters
 *	?	any single character
 *	[a-z]	any single character in the range a-z
 *	[^a-z]	any single character not in the range a-z
 *	\x	match x
 *	
 * External functions:
 *
 *	glob() - match a string against a simple pattern
 *
 * Internal functions:
 *
 *	globchars() - build a bitlist to check for character group match
 */

#ifndef MILLIWAYS_GLOB_H
#include "glob.h"
#endif

#ifndef MILLIWAYS_GLOB_IMPL_H
#define MILLIWAYS_GLOB_IMPL_H

namespace milliways {

#define CHECK_BIT( tab, bit ) ( tab[ (bit)/8 ] & (1<<( (bit)%8 )) )
#define BITLISTSIZE 16	/* bytes used for [chars] in compiled expr */

namespace {
	inline int glob_impl(const char *c, const char *s);
	inline void globchars(const char *s, const char *e, char *b);
} /* end of unnamed namespace */

/*
 * glob() - match a string against a simple pattern
 */

inline bool glob(const char* pattern, const char* str)
{
	int r = glob_impl(pattern, str);
	return (r == 0) ? true : false;
}

inline bool glob(const std::string& pattern, const std::string& str)
{
	int r = glob_impl(pattern.c_str(), str.c_str());
	return (r == 0) ? true : false;
}

namespace {

inline int glob_impl(const char *c, const char *s)
{
	char bitlist[ BITLISTSIZE ];
	const char *here;

	for ( ;; )
    switch( *c++ )
	{
	case '\0':
		return *s ? -1 : 0;

	case '?':
		if( !*s++ )
		    return 1;
		break;

	case '[':
		/* scan for matching ] */

		here = c;
		do if( !*c++ )
			return 1;
		while( here == c || *c != ']' );
		c++;

		/* build character class bitlist */

		globchars( here, c, bitlist );

		if( !CHECK_BIT( bitlist, *(unsigned char *)s ) )
			return 1;
		s++;
		break;

	case '*':
		here = s;

		while( *s ) 
			s++;

		/* Try to match the rest of the pattern in a recursive */
		/* call.  If the match fails we'll back up chars, retrying. */

		while( s != here )
		{
			int r;

			/* A fast path for the last token in a pattern */

			r = *c ? glob_impl(c, s) : (*s ? -1 : 0);

			if( !r )
				return 0;
			else if( r < 0 )
				return 1;

			--s;
		}
		break;

	case '\\':
		/* Force literal match of next char. */

		if( !*c || *s++ != *c++ )
		    return 1;
		break;

	default:
		if( *s++ != c[-1] )
		    return 1;
		break;
	}
}


/*
 * globchars() - build a bitlist to check for character group match
 */

inline void globchars(const char *s, const char *e, char *b)
{
	int neg = 0;

	memset( b, '\0', BITLISTSIZE  );

	if (( *s == '^') || ( *s == '!'))
		neg++, s++;

	while( s < e )
	{
		int c;

		if( s+2 < e && s[1] == '-' )
		{
			for( c = s[0]; c <= s[2]; c++ )
				b[ c/8 ] |= (1<<(c%8));
			s += 3;
		} else {
			c = *s++;
			b[ c/8 ] |= (1<<(c%8));
		}
	}
			
	if( neg )
	{
		int i;
		for( i = 0; i < BITLISTSIZE; i++ )
			b[ i ] ^= 0377;
	}

	/* Don't include \0 in either $[chars] or $[^chars] */

	b[0] &= 0376;
}

} /* end of unnamed namespace */

} /* end of namespace milliways */

#endif /* MILLIWAYS_GLOB_IMPL_H */
