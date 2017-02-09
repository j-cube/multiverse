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

#ifndef MILLIWAYS_GLOB_H
#define MILLIWAYS_GLOB_H

#include <iostream>
#include <fstream>
#include <string>
#include <functional>

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

namespace milliways {

inline bool glob(const char* pattern, const char* str);
inline bool glob(const std::string& pattern, const std::string& str);

} /* end of namespace milliways */

#include "glob.impl.hpp"

#endif /* MILLIWAYS_GLOB_H */
