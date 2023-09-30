#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <boost/url/segments_encoded_view.hpp>

#include "http/server/router/child_idx_vector.h"
#include "http/server/router/router_base.h"
#include "http/server/router/segment_template.h"

namespace AqualinkAutomate::HTTP::Router
{

	struct node
	{
		static constexpr std::size_t npos{ std::size_t(-1) };

		// literal segment or replacement field
		segment_template seg{};

		// A pointer to the resource
		router_base::any_resource const* resource{ nullptr };

		// The complete match for the resource
		std::string path_template;

		// Index of the parent node in the
		// implementation pool of nodes
		std::size_t parent_idx{ npos };

		// Index of child nodes in the pool
		child_idx_vector child_idx;
	};

	class impl
	{
	public:
		impl()
		{
			// root node with no resource
			nodes_.push_back(node{});
		}

		~impl()
		{
			for (auto& r : nodes_)
			{
				delete r.resource;
			}
		}

		void insert_impl(std::string_view path, router_base::any_resource const* v);
		router_base::any_resource const* find_impl(boost::urls::segments_encoded_view path, std::string_view*& matches, std::string_view*& ids) const;

	private:
		node const* try_match(boost::urls::segments_encoded_view::const_iterator it, boost::urls::segments_encoded_view::const_iterator end, node const* root, int level, std::string_view*& matches, std::string_view*& ids) const;
		static node const* find_optional_resource(const node* root, std::vector<node> const& ns, std::string_view*& matches, std::string_view*& ids);

	private:
		std::vector<node> nodes_;

	};

}
// namespace AqualinkAutomate::HTTP::Router
