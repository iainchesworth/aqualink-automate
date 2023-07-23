#include <memory>
#include <string>

#include "kernel/auxillary_devices/auxillary_device.h"
#include "jandy/utility/string_conversion/auxillary_state.h"

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
		std::shared_ptr<Kernel::AuxillaryDevice> CreateDevice(const Utility::AuxillaryState& aux_state);

	private:
		bool IsAuxillaryDevice(const std::string& label) const;
		bool IsChlorinatorDevice(const std::string& label) const;
		bool IsCleanerDevice(const std::string& label) const;
		bool IsHeaterDevice(const std::string& label) const;
		bool IsPumpDevice(const std::string& label) const;
		bool IsSpilloverDevice(const std::string& label) const;
		bool IsSprinklerDevice(const std::string& label) const;
	};

}
// namespace AqualinkAutomate::Factory
