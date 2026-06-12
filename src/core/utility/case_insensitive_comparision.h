#pragma once

namespace AqualinkAutomate::Utility
{

	// Correctly-spelled, PascalCase case-insensitive single-character comparison.
	// This is the canonical name; prefer it for all new call sites.
	bool CaseInsensitiveComparison(const unsigned char lhs, const unsigned char rhs);

	// Legacy mis-spelled alias retained so existing call sites keep compiling while
	// they migrate to CaseInsensitiveComparison(). Forwards to the canonical helper.
	// Do not use in new code. (No [[deprecated]] attribute: the build treats warnings
	// as errors, so flagging it would break the as-yet-unmigrated out-of-unit callers.)
	inline bool case_insensitive_comparision(const unsigned char lhs, const unsigned char rhs)
	{
		return CaseInsensitiveComparison(lhs, rhs);
	}

}
// namespace AqualinkAutomate::Utility
