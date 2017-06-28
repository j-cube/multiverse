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

#ifndef MILLIWAYS_BTREECOMMON_H
#define MILLIWAYS_BTREECOMMON_H

#include <iostream>
#include <fstream>
#include <string>
#include <functional>

#include <stdint.h>
#include <assert.h>

#include "BlockStorage.h"

/* ----------------------------------------------------------------- *
 *   CONFIG                                                          *
 * ----------------------------------------------------------------- */

#ifndef MILLIWAYS_DEFAULT_BLOCK_SIZE
#define MILLIWAYS_DEFAULT_BLOCK_SIZE 4096
#endif /* MILLIWAYS_DEFAULT_BLOCK_SIZE */

#ifndef MILLIWAYS_DEFAULT_B_FACTOR
#define MILLIWAYS_DEFAULT_B_FACTOR 64
#endif /* MILLIWAYS_DEFAULT_B_FACTOR */

#ifndef MILLIWAYS_DEFAULT_BLOCK_CACHE_SIZE
#define MILLIWAYS_DEFAULT_BLOCK_CACHE_SIZE 8192
#endif /* MILLIWAYS_DEFAULT_BLOCK_CACHE_SIZE */

#ifndef MILLIWAYS_DEFAULT_NODE_CACHE_SIZE
#define MILLIWAYS_DEFAULT_NODE_CACHE_SIZE 1024
#endif /* MILLIWAYS_DEFAULT_NODE_CACHE_SIZE */


/* ----------------------------------------------------------------- */

#ifndef UNUSED
#define UNUSED(expr) do { (void)(expr); } while (0)
#endif

namespace milliways {

typedef block_id_t node_id_t;

static const node_id_t NODE_ID_INVALID = static_cast<node_id_t>(BLOCK_ID_INVALID);

inline bool node_id_valid(node_id_t node_id) { return (node_id != NODE_ID_INVALID); }

template < int B_, typename KeyTraits, typename TTraits, class Compare >
class BTreeNode;

template < int B_, typename KeyTraits, typename TTraits, class Compare >
class BTree;

template < int B_, typename KeyTraits, typename TTraits, class Compare >
class BTreeNodeManager;

template < int B_, typename KeyTraits, typename TTraits, class Compare >
class BTreeStorage;

template < int B_, typename KeyTraits, typename TTraits, class Compare >
class BTreeMemoryStorage;

template < size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
class BTreeFileStorage;

} /* end of namespace milliways */

#endif /* MILLIWAYS_BTREECOMMON_H */
