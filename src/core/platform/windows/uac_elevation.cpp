#include "logging/logging.h"
#include "platform/windows/uac_elevation.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Developer
{

	tl::expected<bool, HRESULT> IsElevated()
	{
		TOKEN_ELEVATION te = { 0 };
		DWORD dwReturnLength = 0;

		if (HANDLE hToken = NULL; !::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken))
		{
			LogWarning(Channel::Platform, "Failed to get access token associated with the current process");
			return tl::unexpected(E_FAIL);
		}
		else if (!::GetTokenInformation(hToken, TokenElevation, &te, sizeof(TOKEN_ELEVATION), &dwReturnLength))
		{
			LogWarning(Channel::Platform, "Failed to query access token for elevation information");
			return tl::unexpected(E_FAIL);
		}
		else
		{
			::CloseHandle(hToken);
		}

		return (0 != te.TokenIsElevated);
	}

}
// namespace AqualinkAutomate::Developer
