#include <memory>

#include "logging/logging.h"
#include "platform/windows/uac_elevation.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Developer
{

	namespace
	{
		// RAII wrapper that closes the process token on every exit path. ::CloseHandle
		// returns BOOL; the deleter ignores it (nothing actionable on a close failure).
		struct HandleCloser
		{
			void operator()(HANDLE handle) const noexcept
			{
				::CloseHandle(handle);
			}
		};

		using UniqueHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleCloser>;
	}

	std::expected<bool, HRESULT> IsElevated()
	{
		TOKEN_ELEVATION te = { 0 };
		DWORD dwReturnLength = 0;

		HANDLE raw_token = nullptr;

		if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &raw_token))
		{
			LogWarning(Channel::Platform, "Failed to get access token associated with the current process");
			return std::unexpected(E_FAIL);
		}

		// Guarantees the token handle is closed on every subsequent return path.
		const UniqueHandle token{ raw_token };

		if (!::GetTokenInformation(token.get(), TokenElevation, &te, sizeof(TOKEN_ELEVATION), &dwReturnLength))
		{
			LogWarning(Channel::Platform, "Failed to query access token for elevation information");
			return std::unexpected(E_FAIL);
		}

		return (0 != te.TokenIsElevated);
	}

}
// namespace AqualinkAutomate::Developer
