#pragma once

#include <cstdint>
#include <string>

#include <boost/enable_shared_from_this.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>

namespace AqualinkAutomate::Options
{
	class AppOption : public boost::program_options::option_description, public boost::enable_shared_from_this<AppOption>
	{
		inline static const std::string SEPERATOR{ "," };

	private:
		explicit AppOption(const std::string& long_name, const std::string& description);
		explicit AppOption(const std::string& long_name, const std::string& description, const boost::program_options::value_semantic* s);
		explicit AppOption(const std::string& long_name, const std::string& short_name, const std::string& description);
		explicit AppOption(const std::string& long_name, const std::string& short_name, const std::string& description, const boost::program_options::value_semantic* s);

	private:
		AppOption(const AppOption&) = delete;
		AppOption(AppOption&&) = delete;

	public:
		boost::shared_ptr<boost::program_options::option_description> operator()();

	public:
		bool IsPresent(boost::program_options::variables_map& vm) const;
		bool IsPresentAndNotJustDefaulted(boost::program_options::variables_map& vm) const;

	public:
		template<typename T>
		const T& As(boost::program_options::variables_map& vm) const
		{
			return vm[m_LongName.c_str()].as<T>();
		}

	private:
		const std::string m_LongName;
		const std::string m_ShortName;
		const std::string m_Description;

	private:
		template<typename...Args>
		friend auto make_appoption(Args&&... args);
	};

	typedef boost::shared_ptr<AppOption> AppOptionPtr;

	template<typename...Args>
	auto make_appoption(Args&&... args)
	{
		auto ao = new AppOption(args...);
		return AppOptionPtr(ao);
	};
 }
// namespace AqualinkAutomate::Options
