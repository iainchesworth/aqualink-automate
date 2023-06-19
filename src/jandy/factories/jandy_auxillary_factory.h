#include <memory>
#include <string>

#include "jandy/config/jandy_config_auxillary_base.h"

namespace AqualinkAutomate::Factory
{

	class JandyAuxillaryFactory
	{
	private:
		JandyAuxillaryFactory();
		~JandyAuxillaryFactory() = default;

	public:
		JandyAuxillaryFactory(const JandyAuxillaryFactory&) = delete;
		JandyAuxillaryFactory& operator=(const JandyAuxillaryFactory&) = delete;
		JandyAuxillaryFactory(JandyAuxillaryFactory&&) = delete;
		JandyAuxillaryFactory& operator=(JandyAuxillaryFactory&&) = delete;

	public:
		static JandyAuxillaryFactory& Instance();

	public:
		std::shared_ptr<Config::AuxillaryBase> CreateDevice(const Utility::AuxillaryState& aux_state);

	private:
		bool IsAuxillaryDevice(const std::string& label) const;
		bool IsCleanerDevice(const std::string& label) const;
		bool IsHeaterDevice(const std::string& label) const;
		bool IsPumpDevice(const std::string& label) const;
		bool IsSpilloverDevice(const std::string& label) const;
		bool IsSprinklerDevice(const std::string& label) const;
	};

}
// namespace AqualinkAutomate::Factory
