#pragma once

namespace AqualinkAutomate::Utility
{

	template<class... Ts>
	struct OverloadedVisitor : Ts... { using Ts::operator()...; };

	template<class... Ts>
	OverloadedVisitor(Ts...) -> OverloadedVisitor<Ts...>;

}
// namespace AqualinkAutomate::Utility
