#include "localisation/translations_and_units_formatter.h"

namespace AqualinkAutomate::Localisation
{

	TranslationsAndUnitsFormatter::TranslationsAndUnitsFormatter() = default;

	TranslationsAndUnitsFormatter& TranslationsAndUnitsFormatter::Instance()
	{
		static TranslationsAndUnitsFormatter instance;
		return instance;
	}

}
// namespace AqualinkAutomate::Localisation
