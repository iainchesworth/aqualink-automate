#include "platform/windows/uac_elevation.h"

namespace AqualinkAutomate::Developer
{

	tl::expected<bool, HRESULT> IsElevated()
	{
		TOKEN_ELEVATION te = { 0 };
		DWORD dwReturnLength = 0;

		if (HANDLE hToken = NULL; !::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken))
		{
			///FIXME LOGGING
			return tl::unexpected(E_FAIL);
		}
		else if (!::GetTokenInformation(hToken, TokenElevation, &te, sizeof(TOKEN_ELEVATION), &dwReturnLength))
		{
			///FIXME LOGGING
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
