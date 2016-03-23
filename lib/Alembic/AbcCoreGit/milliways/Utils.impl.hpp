/*
 * milliways - B+ trees and key-value store C++ library
 *
 * Author: Marco Pantaleoni <marco.pantaleoni@gmail.com>
 * Copyright (C) 2016 Marco Pantaleoni. All rights reserved.
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

#ifndef MILLIWAYS_UTILS_H
#include "Utils.h"
#endif

#ifndef MILLIWAYS_UTILS_IMPL_H
//#define MILLIWAYS_UTILS_IMPL_H

#include <iostream>
#include <sstream>
#include <iomanip>

#include <ctype.h>
#include <stdio.h>
#include <arpa/inet.h>

namespace milliways {

inline void hexdump(void* ptr, int buflen)
{
	unsigned char *buf = (unsigned char*) ptr;
	int i, j;
	for (i = 0; i < buflen; i += 16)
	{
		fprintf(stderr, "%06x: ", i);
		for (j = 0; j < 16; j++)
			if (i+j < buflen)
				fprintf(stderr, "%02x ", buf[i+j]);
			else
				fprintf(stderr, "   ");
		printf(" ");
		for (j = 0; j < 16; j++)
			if (i+j < buflen)
				fprintf(stderr, "%c", isprint(buf[i+j]) ? buf[i+j] : '.');
		fprintf(stderr, "\n");
	}
}

inline std::ostream& hexdump(std::ostream& out, const void* ptr, int buflen)
{
	const unsigned char *buf = (const unsigned char*) ptr;
	int i, j;

	long oldw;
	char oldfill;

	for (i = 0; i < buflen; i += 16)
	{
		oldw = out.width();
		oldfill = out.fill();
		out << std::setfill('0') << std::setw(6) << std::hex << i <<
				std::setfill(oldfill) << std::setw(oldw) << std::dec << ": ";
		// fprintf(stderr, "%06x: ", i);
		for (j = 0; j < 16; j++)
		{
			if (i + j < buflen)
			{
				out << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) (buf[i + j]) <<
						std::setfill(oldfill) << std::setw(oldw) << std::dec << " ";
				// fprintf(stderr, "%02x ", buf[i + j]);
			} else {
				out << "   ";
				// fprintf(stderr, "   ");
			}
		}
		out << " ";
		// printf(" ");
		for (j = 0; j < 16; j++)
		{
			if (i + j < buflen)
			{
				out << (char)(isprint(buf[i + j]) ? buf[i + j] : '.');
				// fprintf(stderr, "%c", isprint(buf[i + j]) ? buf[i + j] : '.');
			}
		}
		out << std::endl;
		// fprintf(stderr, "\n");
	}

	return out;
}

inline std::string s_hexdump(const void* ptr, int buflen)
{
	std::ostringstream ss;
	hexdump(ss, ptr, buflen);
	return ss.str();
}

} /* end of namespace milliways */

#endif /* MILLIWAYS_UTILS_IMPL_H */
