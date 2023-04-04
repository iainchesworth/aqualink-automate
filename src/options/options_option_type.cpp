#include "options/options_option_type.h"

namespace AqualinkAutomate::Options
{
	AppOption::AppOption(const std::string& long_name, const std::string& description) :
		AppOption(long_name, description, new boost::program_options::untyped_value(true))
	{
	}

	AppOption::AppOption(const std::string& long_name, const std::string& description, const boost::program_options::value_semantic* s) :
		boost::program_options::option_description(long_name.c_str(), s, description.c_str()),
		m_LongName(long_name),
		m_ShortName(""),
		m_Description(description)
	{
	}

	AppOption::AppOption(const std::string& long_name, const std::string& short_name, const std::string& description) :
		AppOption(long_name, short_name, description, new boost::program_options::untyped_value(true))
	{
	}

	AppOption::AppOption(const std::string& long_name, const std::string& short_name, const std::string& description, const boost::program_options::value_semantic* s) :
		boost::program_options::option_description((long_name + SEPERATOR + short_name).c_str(), s, description.c_str()),
		m_LongName(long_name),
		m_ShortName(short_name),
		m_Description(description)
	{
	}

	boost::shared_ptr<boost::program_options::option_description> AppOption::operator()()
	{
		return this->shared_from_this();
	}

	bool AppOption::IsPresent(boost::program_options::variables_map& vm) const
	{
		return (0 < vm.count(m_LongName.c_str()));
	}

}
// namespace AqualinkAutomate::Options
