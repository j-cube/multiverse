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

#ifndef MILLIWAYS_BTREENODE_H
#include "BTreeNode.h"
#endif

#ifndef MILLIWAYS_BTREENODE_IMPL_H
//#define MILLIWAYS_BTREENODE_IMPL_H

namespace milliways {

template < int B_, typename KeyTraits, typename TTraits, class Compare >
BTreeNode<B_, KeyTraits, TTraits, Compare>::BTreeNode(tree_type* tree, node_id_t node_id, node_id_t parent_id) :
	m_tree(tree), m_id(node_id), m_parent_id(parent_id), m_left_id(NODE_ID_INVALID), m_right_id(NODE_ID_INVALID),
	m_leaf(true), m_n(0), m_rank(0), m_dirty(false)
{
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
BTreeNode<B_, KeyTraits, TTraits, Compare>::BTreeNode(const BTreeNode& other) :
	m_tree(other.m_tree), m_id(other.m_id), m_parent_id(other.m_parent_id),
	m_left_id(other.m_left_id), m_right_id(other.m_right_id),
	m_leaf(other.m_leaf), m_n(other.m_n), m_rank(other.m_rank), m_dirty(other.m_dirty),
	m_keys(other.m_keys), m_values(other.m_values), m_children(other.m_children)
{
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
BTreeNode<B_, KeyTraits, TTraits, Compare>& BTreeNode<B_, KeyTraits, TTraits, Compare>::operator= (const BTreeNode& other)
{
	m_tree = other.m_tree;
	m_id = other.m_id;
	m_parent_id = other.m_parent_id;
	m_left_id = other.m_left_id;
	m_right_id = other.m_right_id;
	m_leaf = other.m_leaf;
	m_n = other.m_n;
	m_rank = other.m_rank;
	m_dirty = other.m_dirty;
	m_keys = other.m_keys;
	m_values = other.m_values;
	m_children = other.m_children;

	return *this;
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTreeNode<B_, KeyTraits, TTraits, Compare>::search(lookup_type& res, const key_type& key_)
{
	if (leaf())
	{
		shptr<node_type> self( this_node() );
		int i;
		for (i = 0; i < n(); i++)
		{
			if (key_ < key(i))
			{
				res.node(self).found(false).pos(i).key(key_);
				return false;
			} else if (key_ == key(i))
			{
				res.node(self).found(true).pos(i).key(key_);
				return true;
			}
		}
		res.node(self).found(false).pos(i).key(key_);
		return false;
	} else
	{
		assert(! leaf());

		int i;
		for (i = 0; i < n(); i++)
		{
			if (key_ < key(i))
			{
				shptr<node_type> child( child_node(i) );
				assert(child);
				return child->search(res, key_);
			}
		}
		shptr<node_type> child( child_node(n()) );
		assert(child);
		return child->search(res, key_);
	}
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
void BTreeNode<B_, KeyTraits, TTraits, Compare>::truncate(int num)
{
	assert((num >= 0) && (num <= (2 * B - 1)));
	assert(num <= n());
	if (num > n())
		return;
	for (int i = num + 1; i <= n(); i++)
	{
		child(i) = NODE_ID_INVALID;
	}
	n(num);
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTreeNode<B_, KeyTraits, TTraits, Compare>::bsearch(lookup_type& res, const key_type& key_)
{
	int lo = 0;
	int hi = n() - 1;
	shptr<node_type> self( this_node() );

	while (hi >= lo)
	{
		int m = (hi + lo) / 2;
		assert(m >= 0);
		assert(m < n());

		if (key_ < key(m))
			hi = m - 1;
		else if (key_ > key(m))
			lo = m + 1;
		else
		{
			// found!
			if (leaf())
				res.node(self).found(true).pos(m).key(key_);
			else
				res.node(self).found(true).pos(m + 1).key(key_);    // internal nodes handled differently
			return true;
		}
	}

	// not found
	res.node(self).found(false).pos(lo).key(key_);
	return false;
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
void BTreeNode<B_, KeyTraits, TTraits, Compare>::split_child(int i)
{
	assert(! leaf());
	assert(! full());
	assert((i >= 0) && (i <= n()));

	shptr<node_type> y( child_node(i) );
	assert(y && y->full());

	shptr<node_type> z( child_alloc() );
	assert(z);
	assert(z->parentId() == id());

	z->leaf(y->leaf());

	z->leftId(y->id());
	z->rightId(y->rightId());
	if (z->hasRight())
	{
		shptr<node_type> z_right( z->right() );
		z_right->leftId(z->id());
		node_put(z_right);
	}
	y->rightId(z->id());

	// copy second half of y into z
	//   z[0..B-1] := y[B..2B-1]
	for (int j = 0; j < (B - 1); j++)
	{
		z->key(j) = y->key(j + B);
		if (z->leaf())
			z->value(j) = y->value(j + B);
	}
	if (! y->leaf())
	{
		assert(! z->leaf());
		for (int j = 0; j < B; j++)
			z->child(j) = y->child(j + B);
	}
	z->n(B - 1);

	// add a spot here at pos i for key from child (y) median pos B
	// (not value, since values are only in leafs)
	//   x.child[i+2..n+1] := x.child[i+1..n]
	for (int j = n(); j >= i + 1; j--)
		child(j + 1) = child(j);

	//   x[i+1..n] := x[i..n-1]
	for (int j = n() - 1; j >= i; j--)
		key(j + 1) = key(j);    // only keys, not values (we are an internal node

	child(i + 1) = z->id();
	key(i) = z->key(0);
	// no value, we are an internal node
	n(n() + 1);

	// keep in y only its first half (B keys)
	// NOTE: since we keep values only in leafs, we keep B keys and not B-1
	y->truncate(B);

	node_put(y);
	node_put(z);
	shptr<node_type> self( this_node() );
	node_put(self);
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
shptr<typename BTreeNode<B_, KeyTraits, TTraits, Compare>::node_type> BTreeNode<B_, KeyTraits, TTraits, Compare>::insert_non_full(const key_type& key_, const mapped_type& value_)
{
	assert(! full());

	if (leaf())
	{
		lookup_type where;
		bsearch(where, key_);
		assert(! where.found());
		assert((where.pos() >= 0) && (where.pos() <= n()));

		// shift right
		//   x[pos+1..n] := x[pos..n-1]
		for (int j = n() - 1; j >= where.pos(); j--)
		{
			key(j + 1) = key(j);
			value(j + 1) = value(j);
		}
		key(where.pos()) = key_;
		value(where.pos()) = value_;
		n(n() + 1);
		shptr<node_type> self( this_node() );
		node_put(self);
		return self;
	} else
	{
		assert(! leaf());

		bool do_search = true;
		lookup_type where;
		shptr<node_type> child_pos;

		while (do_search)
		{
			do_search = false;

			bsearch(where, key_);
			assert((where.pos() >= 0) && (where.pos() <= n()));

			child_pos = child_node(where.pos());
			assert(child_pos);
			if (child_pos->full())
			{
				split_child(where.pos());
				do_search = true;
			}
		}

		return child_pos->insert_non_full(key_, value_);
	}
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTreeNode<B_, KeyTraits, TTraits, Compare>::remove(lookup_type& res, const key_type& key_)
{
	if (leaf())
	{
		if (! bsearch(res, key_))
			return false;

		assert(res.found());
		assert((res.pos() >= 0) && (res.pos() < n()));

		// overwrite pos - shift left
		//   x[pos..n-2] := x[pos+1..n-1]
		for (int j = res.pos(); j < (n() - 1); j++)
		{
			key(j) = key(j + 1);
			value(j) = value(j + 1);
		}
		n(n() - 1);
		shptr<node_type> self( this_node() );
		node_put(self);
		return true;
	} else
	{
		assert(! leaf());

		bsearch(res, key_);
		assert((res.pos() >= 0) && (res.pos() <= n()));

		shptr<node_type> child_pos( child_node(res.pos()) );
		assert(child_pos);
		return child_pos->remove(res, key_);
	}
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
inline std::ostream& BTreeNode<B_, KeyTraits, TTraits, Compare>::dotGraph(std::ostream& out)
{
	typedef std::map<node_id_t, bool> node_set_type;
	typedef std::map<int, node_set_type> rankmap_type;

	struct L {
		static void aggregate_by_rank(const shptr<node_type>& node_ptr, rankmap_type& rankmap, node_set_type& visitedmap)
		{
			assert(node_ptr);
			node_id_t node_id = node_ptr->id();
			int node_rank = node_ptr->rank();

			if (visitedmap.count(node_id) > 0)
				return;
			visitedmap[node_id] = true;
			if (rankmap.count(node_rank) == 0)                  // do we have a set for given rank?
				rankmap[node_rank] = node_set_type();           // no, create it
			node_set_type& nodes_with_same_rank = rankmap[node_rank];
			if (nodes_with_same_rank.count(node_id) == 0)       // is node id in set for rank?
				nodes_with_same_rank[node_id] = true;           //   no, add it
			for (int i = 0; i < node_ptr->n() + 1; i++)
			{
				if (node_ptr->hasChild(i))
				{
					shptr<node_type> child_i( node_ptr->child_node(i) );
					L::aggregate_by_rank(child_i, rankmap, visitedmap);
				}
			}
		}
	};

	rankmap_type rankmap;
	node_set_type visitedmap;
	L::aggregate_by_rank(this_node(), rankmap, visitedmap);

	out << "digraph btree {" << std::endl;
	out << "    node [shape=plaintext]" << std::endl;
	out << "    rankdir = TB;" << std::endl;

	std::map<node_id_t, bool> visitedMap;
	this->dotGraph(out, visitedMap, /* level */ 0, /* indent */ 1);

	rankmap_type::const_iterator it;
	for (it = rankmap.begin(); it != rankmap.end(); ++it)
	{
		int rank = it->first;
		std::string same_rank_s;

		node_set_type& nodes_with_rank = rankmap[rank];
		node_set_type::const_iterator n_it;

		for (n_it = nodes_with_rank.begin(); n_it != nodes_with_rank.end(); ++n_it)
		{
			node_id_t node_id = n_it->first;

			std::ostringstream ss;
			ss << "node_" << node_id;
			std::string nodename = ss.str();

			same_rank_s += nodename + "; ";
		}

		out << "{ rank = same; " << same_rank_s << " }" << std::endl;
	}

	out << "}" << std::endl;
	return out;
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
inline void BTreeNode<B_, KeyTraits, TTraits, Compare>::dotGraph(const std::string& basename, bool display)
{
	std::string pathname_dot = basename + ".dot";
	std::string pathname_pdf = basename + ".pdf";

	std::fstream st;

	st.open(pathname_dot.c_str(), std::fstream::out | std::fstream::trunc);
	dotGraph(st);
	st.close();

	std::ostringstream ss_cmd;
	ss_cmd << "dot -Tpdf " << pathname_dot << " -o" << pathname_pdf;
	system(ss_cmd.str().c_str());
	std::cout << "graph generated at " << pathname_pdf << std::endl;
	if (display)
	{
		std::ostringstream ss_cmd;
		ss_cmd << "open " << pathname_pdf;
		system(ss_cmd.str().c_str());
	}
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
inline std::ostream& BTreeNode<B_, KeyTraits, TTraits, Compare>::dotGraph(std::ostream& out, std::map<node_id_t, bool>& visitedMap, int level, int indent_)
{
	struct L {
		static inline std::string indent(int amount)
		{
			return std::string(amount * 4, ' ');
		}

		static inline std::string nodename( node_type& node )
		{
			std::ostringstream ss;
			ss << "node_" << node.id();
			return ss.str();
		}

		static inline std::string child_portname( node_type& node, int child )
		{
			std::ostringstream ss;
			ss << "c_" << node.id() << "_" << child;
			return ss.str();
		}

		static inline std::string child_idx_repr( node_type& node, int child )
		{
			std::ostringstream ss;
			if (node.hasChild(child))
				ss << node.child(child);
			else
				ss << ".";
			return ss.str();
		}

		static inline std::string key_portname( node_type& node, int key )
		{
			std::ostringstream ss;
			ss << "k_" << node.id() << "_" << key;
			return ss.str();
		}
	};

	if (visitedMap.count(id()) > 0)
		return out;
	visitedMap[id()] = true;

	/* node itself */

	out << L::indent(indent_) << "/* node " << id() << " */" << std::endl;
	out << L::indent(indent_) << L::nodename(*this) << " [style=filled, fillcolor=\"#EFEFDE\", label=<" << std::endl;
	out << L::indent(indent_) << "  " << "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\">" << std::endl;

	out << L::indent(indent_) << "  " << "<TR>" << std::endl;
	out << L::indent(indent_) << "    " << "<TD PORT=\"hl\" COLSPAN=\"1\">L</TD>" << std::endl;
	out << L::indent(indent_) << "    " << "<TD PORT=\"h\" COLSPAN=\"" << (n() * 2 + 1 - 2) << "\">#" << id() << "</TD>" << std::endl;
	out << L::indent(indent_) << "    " << "<TD PORT=\"hr\" COLSPAN=\"1\">R</TD>" << std::endl;
	out << L::indent(indent_) << "  " << "</TR>" << std::endl;

	out << L::indent(indent_) << "  " << "<TR>" << std::endl;
	out << L::indent(indent_) << "    " << "<TD PORT=\"" << L::child_portname(*this, 0) << "\"><FONT POINT-SIZE=\"8\">" << L::child_idx_repr(*this, 0) << "</FONT></TD>" << std::endl;
	for (int i = 0; i < n(); i++)
	{
		out << L::indent(indent_) << "    " << "<TD PORT=\"" << L::key_portname(*this, i) << "\" BGCOLOR=\"#FCD975\">" << key(i) << "</TD>" << std::endl;
		out << L::indent(indent_) << "    " << "<TD PORT=\"" << L::child_portname(*this, (i + 1)) << "\"><FONT POINT-SIZE=\"8\">" << L::child_idx_repr(*this, i+1) << "</FONT></TD>" << std::endl;
	}
	out << L::indent(indent_) << "  " << "</TR>" << std::endl;

	out << L::indent(indent_) << "  " << "</TABLE>" << std::endl;
	out << L::indent(indent_) << ">];" << std::endl;
	out << std::endl;

	if (! leaf())
	{
		/* children */
		out << L::indent(indent_) << "/* node " << id() << " children */" << std::endl;
		for (int i = 0; i <= n(); i++)
		{
			if (hasChild(i))
			{
				shptr<node_type> child_i( child_node(i) );
				child_i->dotGraph(out, visitedMap, level + 1, indent_ + 1);
			}
		}
		out << std::endl;

		/* children connections */
		out << L::indent(indent_) << "/* node " << id() << " connections */" << std::endl;
		for (int i = 0; i <= n(); i++)
		{
			if (hasChild(i))
			{
				shptr<node_type> child_i( child_node(i) );

				out << L::indent(indent_) << L::nodename(*this) << ":" << L::child_portname(*this, i) << " -> " << L::nodename(*child_i) << ":" << "h" << ";" << std::endl;
			}
		}
		out << std::endl;
	}

	/* left/right */
	out << L::indent(indent_) << "/* node " << id() << " left/right connections */" << std::endl;
	if (hasLeft())
		out << L::indent(indent_) << L::nodename(*this) << ":hl -> " << L::nodename(*left()) << ":hr  [color=\"#9ACEEB\", weight=0.3] ;" << std::endl;
	if (hasRight())
		out << L::indent(indent_) << L::nodename(*this) << ":hr -> " << L::nodename(*right()) << ":hl  [color=\"#CE9AEB\", weight=0.3] ;" << std::endl;
	out << std::endl;

	return out;
}

} /* end of namespace milliways */

#endif /* MILLIWAYS_BTREENODE_IMPL_H */
