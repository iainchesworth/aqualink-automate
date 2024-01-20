#pragma once

#include <cstddef>
#include <format>
#include <string>
#include <vector>

#include <boost/url/decode_view.hpp>
#include <boost/url/segments_encoded_view.hpp>

#include "http/server/routing/child_idx_vector.h"
#include "http/server/routing/segment_template.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP::Routing
{
	
	template<typename HANDLER_TYPE>
	struct node
	{
		static constexpr std::size_t npos{ std::size_t(-1) };

		// literal segment or replacement field
		segment_template seg{};

		// A pointer to the resource
		HANDLER_TYPE* resource{ nullptr };

		// The complete match for the resource
		std::string path_template;

		// Index of the parent node in the
		// implementation pool of nodes
		std::size_t parent_idx{ npos };

		// Index of child nodes in the pool
		ChildIdxVector child_idx;
	};

	template<typename HANDLER_TYPE>
	class impl
	{
	public:
		impl()
		{
			// root node with no resource
			nodes_.push_back(node<HANDLER_TYPE>{});
		}

		~impl()
		{
			for (auto& r : nodes_)
			{
				r.resource = nullptr;
			}
		}

		void insert_impl(std::string_view path, HANDLER_TYPE* v)
		{
			LogTrace(Channel::Web, std::format("Inserting route {} into core routing impl", path));

			// Parse dynamic route segments
			if (path.starts_with("/"))
			{
				path.remove_prefix(1);
			}

			auto segsr = boost::urls::grammar::parse(path, path_template_rule);
			if (!segsr)
			{
				segsr.value();
			}

			auto segs = *segsr;
			auto it = segs.begin();
			auto end = segs.end();

			// Iterate existing nodes
			node<HANDLER_TYPE>* cur = &nodes_.front();
			int level = 0;
			while (it != end)
			{
				std::string_view seg = (*it).string();
				if (seg == ".")
				{
					++it;
					continue;
				}
				if (seg == "..")
				{
					// discount unmatched leaf or
					// keep track of levels behind root
					if (cur == &nodes_.front())
					{
						--level;
						++it;
						continue;
					}
					// move to parent deleting current
					// if it carries no resource
					std::size_t p_idx = cur->parent_idx;
					if (cur == &nodes_.back() && !cur->resource && cur->child_idx.empty())
					{
						node<HANDLER_TYPE>* p = &nodes_[p_idx];
						std::size_t cur_idx = cur - nodes_.data();
						p->child_idx.erase(
							std::remove(
								p->child_idx.begin(),
								p->child_idx.end(),
								cur_idx));
						nodes_.pop_back();
					}

					cur = &nodes_[p_idx];
					++it;
					continue;
				}

				// discount unmatched root parent
				if (level < 0)
				{
					++level;
					++it;
					continue;
				}

				// look for child
				auto cit = std::find_if(cur->child_idx.begin(), cur->child_idx.end(),
					[this, &it](std::size_t ci) -> bool
					{
						return nodes_[ci].seg == *it;
					}
				);

				if (cit != cur->child_idx.end())
				{
					// move to existing child
					cur = &nodes_[*cit];
				}
				else
				{
					// create child if it doesn't exist
					node<HANDLER_TYPE> child;
					child.seg = *it;
					std::size_t cur_id = cur - nodes_.data();
					child.parent_idx = cur_id;
					nodes_.push_back(std::move(child));
					nodes_[cur_id].child_idx.push_back(nodes_.size() - 1);
					if (nodes_[cur_id].child_idx.size() > 1)
					{
						// keep nodes sorted
						auto& cs = nodes_[cur_id].child_idx;
						std::size_t n = cs.size() - 1;
						while (n)
						{
							if (nodes_[cs.begin()[n]].seg < nodes_[cs.begin()[n - 1]].seg)
							{
								std::swap(cs.begin()[n], cs.begin()[n - 1]);
							}
							else
							{
								break;
							}

							--n;
						}
					}

					cur = &nodes_.back();
				}

				++it;
			}

			if (level != 0)
			{
				boost::urls::detail::throw_invalid_argument();
			}

			cur->resource = v;
			cur->path_template = path;
		}

		HANDLER_TYPE* find_impl(boost::urls::segments_encoded_view path, std::string_view*& matches, std::string_view*& ids) const
		{
			// parse_path is inconsistent for empty paths
			if (path.empty())
			{
				path = boost::urls::segments_encoded_view("./");
			}

			// Iterate nodes from the root
			if (auto p = try_match(path.begin(), path.end(), &nodes_.front(), 0, matches, ids); nullptr != p)
			{
				return p->resource;
			}

			return nullptr;
		}

	private:
		node<HANDLER_TYPE> const* try_match(boost::urls::segments_encoded_view::const_iterator it, boost::urls::segments_encoded_view::const_iterator end, node<HANDLER_TYPE> const* cur, int level, std::string_view*& matches, std::string_view*& ids) const
		{
			while (it != end)
			{
				boost::urls::pct_string_view s = *it;
				if (*s == ".")
				{
					// ignore segment
					++it;
					continue;
				}

				if (*s == "..")
				{

					// move back to the parent node
					++it;
					if (level <= 0 && cur != &nodes_.front())
					{
						if (!cur->seg.is_literal())
						{
							--matches;
							--ids;
						}

						cur = &nodes_[cur->parent_idx];
					}
					else
					{
						// there's no parent, so we
						// discount that from the implicit
						// tree beyond terminals
						--level;
					}
					continue;
				}

				// we are in the implicit tree above the
				// root, so discount that as a level
				if (level < 0)
				{
					++level;
					++it;
					continue;
				}

				// calculate the lower bound on the possible number of branches to determine if we need to branch.  We 
				// branch when we might have more than one child matching node at this level.  If so, we need to potentially 
				// branch to find which path leads to a valid resource. Otherwise, we can just consume the node and input 
				// without any recursive function calls.
				bool branch = false;
				if (cur->child_idx.size() > 1)
				{
					int branches_lb = 0;
					for (auto i : cur->child_idx)
					{
						auto& c = nodes_[i];
						if (c.seg.is_literal() || !c.seg.has_modifier())
						{
							// a literal path counts only
							// if it matches
							branches_lb += c.seg.match(s);
						}
						else
						{
							// everything not matching
							// a single path counts as
							// more than one path already
							branches_lb = 2;
						}
						if (branches_lb > 1)
						{
							// already know we need to
							// branch
							branch = true;
							break;
						}
					}
				}

				// attempt to match each child node
				node<HANDLER_TYPE> const* r = nullptr;
				bool match_any = false;
				for (auto i : cur->child_idx)
				{
					auto& c = nodes_[i];
					if (c.seg.match(s))
					{
						if (c.seg.is_literal())
						{
							// just continue from the
							// next segment
							if (branch)
							{
								r = try_match(std::next(it), end, &c, level, matches, ids);
								if (r)
								{
									break;
								}
							}
							else
							{
								cur = &c;
								match_any = true;
								break;
							}
						}
						else if (!c.seg.has_modifier())
						{
							// just continue from the
							// next segment
							if (branch)
							{
								auto matches0 = matches;
								auto ids0 = ids;
								*matches++ = *it;
								*ids++ = c.seg.id();
								r = try_match(std::next(it), end, &c, level, matches, ids);
								if (r)
								{
									break;
								}
								else
								{
									// rewind
									matches = matches0;
									ids = ids0;
								}
							}
							else
							{
								// only path possible
								*matches++ = *it;
								*ids++ = c.seg.id();
								cur = &c;
								match_any = true;
								break;
							}
						}
						else if (c.seg.is_optional())
						{
							// attempt to match by ignoring
							// and not ignoring the segment.
							// we first try the complete
							// continuation consuming the
							// input, which is the
							// longest and most likely
							// match
							auto matches0 = matches;
							auto ids0 = ids;
							*matches++ = *it;
							*ids++ = c.seg.id();
							r = try_match(std::next(it), end, &c, level, matches, ids);
							if (r)
							{
								break;
							}
							// rewind
							matches = matches0;
							ids = ids0;
							// try complete continuation
							// consuming no segment
							*matches++ = {};
							*ids++ = c.seg.id();
							r = try_match(it, end, &c, level, matches, ids);
							if (r)
								break;
							// rewind
							matches = matches0;
							ids = ids0;
						}
						else
						{
							// check if the next segments
							// won't send us to a parent
							// directory
							auto first = it;
							std::size_t ndotdot = 0;
							std::size_t nnondot = 0;
							auto it1 = it;
							while (it1 != end)
							{
								if (*it1 == "..")
								{
									++ndotdot;
									if (ndotdot >= (nnondot + c.seg.is_star()))
									{
										break;
									}
								}
								else if (*it1 != ".")
								{
									++nnondot;
								}
								++it1;
							}
							if (it1 != end)
							{
								break;
							}

							// attempt to match many
							// segments
							auto matches0 = matches;
							auto ids0 = ids;
							*matches++ = *it;
							*ids++ = c.seg.id();

							// if this is a plus seg, we
							// already consumed the first
							// segment
							if (c.seg.is_plus())
							{
								++first;
							}

							// {*} is usually the last
							// match in a path.
							// try complete continuation
							// match for every subrange
							// from {last, last} to
							// {first, last}.
							// We also try {last, last}
							// first because it is the
							// longest match.
							auto start = end;
							while (start != first)
							{
								r = try_match(start, end, &c, level, matches, ids);
								if (r)
								{
									std::string_view prev = *std::prev(start);
									*matches0 = { matches0->data(), prev.data() + prev.size() };
									break;
								}

								matches = matches0 + 1;
								ids = ids0 + 1;
								--start;
							}

							if (r)
							{
								break;
							}

							// start == first
							matches = matches0 + 1;
							ids = ids0 + 1;
							r = try_match(start, end, &c, level, matches, ids);
							if (r)
							{
								if (!c.seg.is_plus())
								{
									*matches0 = {};
								}
								break;
							}
						}
					}
				}
				// r represent we already found a terminal
				// node which is a match
				if (r)
				{
					return r;
				}
				// if we couldn't match anything, we go
				// one level up in the implicit tree
				// because the path might still have a
				// "..".
				if (!match_any)
				{
					++level;
				}

				++it;
			}
			if (level != 0)
			{
				// the path ended below or above an
				// existing node
				return nullptr;
			}
			if (!cur->resource)
			{
				// we consumed all the input and reached
				// a node with no resource, but it might
				// still have child optional segments
				// with resources we can reach without
				// consuming any input
				return find_optional_resource(cur, nodes_, matches, ids);
			}

			return cur;
		}
		
		static node<HANDLER_TYPE> const* find_optional_resource(const node<HANDLER_TYPE>* root, std::vector<node<HANDLER_TYPE>> const& ns, std::string_view*& matches, std::string_view*& ids)
		{
			BOOST_ASSERT(root);
			if (root->resource)
			{
				return root;
			}

			BOOST_ASSERT(!root->child_idx.empty());
			for (auto i : root->child_idx)
			{
				auto& c = ns[i];
				if (!c.seg.is_optional() && !c.seg.is_star())
				{
					continue;
				}

				// Child nodes are also potentially optional.
				auto matches0 = matches;
				auto ids0 = ids;
				*matches++ = {};
				*ids++ = c.seg.id();
				auto n = find_optional_resource(&c, ns, matches, ids);
				if (n)
				{
					return n;
				}

				matches = matches0;
				ids = ids0;
			}

			return nullptr;
		}

	private:
		std::vector<node<HANDLER_TYPE>> nodes_;

	};

}
// namespace AqualinkAutomate::HTTP::Routing
