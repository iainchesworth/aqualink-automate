#include "localisation/translations_and_units_formatter.h"

namespace AqualinkAutomate::Localisation
{

	TranslationsAndUnitsFormatter::TranslationsAndUnitsFormatter()
	{
	}

	TranslationsAndUnitsFormatter& TranslationsAndUnitsFormatter::Instance()
	{
		static TranslationsAndUnitsFormatter instance;
		return instance;
	}

}
// namespace AqualinkAutomate::Localisation
