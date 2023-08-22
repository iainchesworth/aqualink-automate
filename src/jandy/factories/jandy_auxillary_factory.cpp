#include <algorithm>

#include <boost/algorithm/string.hpp>

#include "jandy/auxillaries/jandy_auxillary_traits_types.h"
#include "jandy/errors/jandy_errors_auxillary_factory.h"
#include "jandy/factories/jandy_auxillary_factory.h"
#include "jandy/utility/string_conversion/auxillary_state.h"
#include "kernel/auxillary_devices/chlorinator_status.h"
#include "kernel/auxillary_devices/heater_status.h"
#include "kernel/auxillary_devices/pump_status.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "logging/logging.h"
#include "utility/overloaded_variant_visitor.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Factory
{

	JandyAuxillaryFactory::JandyAuxillaryFactory()
	{
	}

	JandyAuxillaryFactory& JandyAuxillaryFactory::Instance()
	{
		static JandyAuxillaryFactory instance;
		return instance;
	}

	tl::expected<std::shared_ptr<Kernel::AuxillaryDevice>, boost::system::error_code> JandyAuxillaryFactory::SerialAdapterDevice_CreateDevice(const Auxillaries::JandyAuxillaryIds aux_id)
	{
		DeviceData data{ AuxillaryDevice_Data{ std::nullopt, aux_id, std::nullopt } };
		return CreateDevice_Impl(data);
	}

	tl::expected<std::shared_ptr<Kernel::AuxillaryDevice>, boost::system::error_code> JandyAuxillaryFactory::SerialAdapterDevice_CreateDevice(const Auxillaries::JandyAuxillaryIds aux_id, const Auxillaries::JandyAuxillaryStatuses status)
	{
		Kernel::AuxillaryStatuses aux_status;

		switch (status)
		{
		case Auxillaries::JandyAuxillaryStatuses::Off:
			aux_status = Kernel::AuxillaryStatuses::Off;
			break;

		case Auxillaries::JandyAuxillaryStatuses::On:
			aux_status = Kernel::AuxillaryStatuses::On;
			break;

		case Auxillaries::JandyAuxillaryStatuses::Unknown:
			[[fallthrough]];
		default:
			aux_status = Kernel::AuxillaryStatuses::Unknown;
			break;
		}

		DeviceData data{ AuxillaryDevice_Data{ std::nullopt, aux_id, aux_status } };
		return CreateDevice_Impl(data);
	}

	tl::expected<std::shared_ptr<Kernel::AuxillaryDevice>, boost::system::error_code> JandyAuxillaryFactory::OneTouchDevice_CreateDevice(const Utility::AuxillaryState& aux_state)
	{
		auto ec = make_error_code(ErrorCodes::Factory_ErrorCodes::Error_UnknownFactoryError);

		if (!aux_state.Label().has_value())
		{
			LogDebug(Channel::Equipment, "Received an invalid auxillary status; factory cannot create a new device using auxillary status");
			ec = make_error_code(ErrorCodes::Factory_ErrorCodes::Error_ReceivedInvalidAuxillaryStatus);
		}
		else if (IsAuxillaryDevice(aux_state.Label().value()))
		{
			if (auto aux_id = magic_enum::enum_cast<Auxillaries::JandyAuxillaryIds>(aux_state.Label().value()); !aux_id.has_value())
			{
				ec = make_error_code(ErrorCodes::Factory_ErrorCodes::Error_CannotCastToJandyAuxillaryId);
			}
			else
			{
				DeviceData data{ AuxillaryDevice_Data{ std::nullopt, aux_id.value(), aux_state.State().value_or(Kernel::AuxillaryStatuses::Unknown) }};
				return CreateDevice_Impl(data);
			}
		}
		else if (IsChlorinatorDevice(aux_state.Label().value()))
		{
			DeviceData data{ ChlorinatorDevice_Data{ aux_state.Label().value(), aux_state.State().value_or(Kernel::AuxillaryStatuses::Unknown) }};
			return CreateDevice_Impl(data);
		}
		else if (IsCleanerDevice(aux_state.Label().value()))
		{
			DeviceData data{ CleanerDevice_Data{ aux_state.Label().value() } };
			return CreateDevice_Impl(data);
		}
		else if (IsHeaterDevice(aux_state.Label().value()))
		{
			// Pool Heat, Spa Heat, Heat Pump
			//
			// Note that ordering means that "Heat Pump" is caught here!

			DeviceData data{ HeaterDevice_Data{ aux_state.Label().value(), aux_state.State().value_or(Kernel::AuxillaryStatuses::Unknown) } };
			return CreateDevice_Impl(data);
		}
		else if (IsPumpDevice(aux_state.Label().value()))
		{
			// Filter Pump, Pool Pump, Spa Pump
			//
			// Note that ordering means that "Heat Pump" is caught above!

			DeviceData data{ PumpDevice_Data{ aux_state.Label().value(), aux_state.State().value_or(Kernel::AuxillaryStatuses::Unknown) } };
			return CreateDevice_Impl(data);
		}
		else if (IsSpilloverDevice(aux_state.Label().value()))
		{
			DeviceData data{ SpilloverDevice_Data{ aux_state.Label().value() } };
			return CreateDevice_Impl(data);
		}
		else if (IsSprinklerDevice(aux_state.Label().value()))
		{
			DeviceData data{ SprinklerDevice_Data{ aux_state.Label().value() } };
			return CreateDevice_Impl(data);
		}
		else
		{
			// Unknown device type...ignore it.
			ec = make_error_code(ErrorCodes::Factory_ErrorCodes::Error_UnknownDeviceLabel);
		}

		return tl::unexpected(ec);
	}

	bool JandyAuxillaryFactory::IsAuxillaryDevice(const std::string& label) const
	{
		if (EXTRA_AUX == label)
		{
			return true;
		}
		else if (label.starts_with(AUX_PREFIX))
		{
			// Generic auxillary labels are "Aux1" or "Aux B1" so 4 or 6 characters.
			static const uint8_t AUX_LABEL_LENGTH = 4;
			static const uint8_t AUX_LABEL_RPC_LENGTH = 6;

			switch (label.size())
			{
			case AUX_LABEL_LENGTH:
			case AUX_LABEL_RPC_LENGTH:
				return true;

			default:
				// Auxillary label length is not as expected.
				break;
			}
		}
		else
		{
			// Doesn't appear to be a valid auxillary label
		}

		return false;
	}

	bool JandyAuxillaryFactory::IsChlorinatorDevice(const std::string& label) const
	{
		return ((CHLORINATOR == label) || (AQUAPURE == label));
	}

	bool JandyAuxillaryFactory::IsCleanerDevice(const std::string& label) const
	{
		return (CLEANER == label);
	}

	bool JandyAuxillaryFactory::IsHeaterDevice(const std::string& label) const
	{
		return boost::algorithm::contains(label, HEAT);
	}

	bool JandyAuxillaryFactory::IsPumpDevice(const std::string& label) const
	{
		return boost::algorithm::contains(label, PUMP);
	}

	bool JandyAuxillaryFactory::IsSpilloverDevice(const std::string& label) const
	{
		return (SPILLOVER == label);
	}

	bool JandyAuxillaryFactory::IsSprinklerDevice(const std::string& label) const
	{
		return false;
	}

	tl::expected<std::shared_ptr<Kernel::AuxillaryDevice>, boost::system::error_code> JandyAuxillaryFactory::CreateDevice_Impl(DeviceData& device_data)
	{
		if (auto aux_ptr = std::make_shared<Kernel::AuxillaryDevice>(); nullptr == aux_ptr)
		{
			return tl::unexpected(make_error_code(ErrorCodes::Factory_ErrorCodes::Error_FailedToCreateAuxillaryPtr));
		}
		else
		{
			std::visit(
				Utility::OverloadedVisitor
				{
					[&aux_ptr](AuxillaryDevice_Data data) 
					{
						aux_ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
						aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, data.Label.value_or(std::string{ magic_enum::enum_name(data.Id) }));
						aux_ptr->AuxillaryTraits.Set(Auxillaries::JandyAuxillaryId{}, data.Id);
						aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryStatusTrait{}, data.Status.value_or(Kernel::AuxillaryStatuses::Unknown));
					},
					[&aux_ptr](ChlorinatorDevice_Data data)
					{
						aux_ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Chlorinator);
						aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, data.Label.value_or(CHLORINATOR));

						if (data.Status.has_value())
						{
							aux_ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::ChlorinatorStatusTrait{}, Kernel::ConvertToChlorinatorStatus(data.Status.value()));
						}
					},
					[&aux_ptr](CleanerDevice_Data data)
					{
						aux_ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Cleaner);
						aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, data.Label.value_or(CLEANER));
					},
					[&aux_ptr](HeaterDevice_Data data)
					{
						aux_ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Heater);
						aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, data.Label.value_or(HEATER));

						if (data.Status.has_value())
						{
							aux_ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::HeaterStatusTrait{}, Kernel::ConvertToHeaterStatus(data.Status.value()));
						}
					},
					[&aux_ptr](PumpDevice_Data data)
					{
						aux_ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Pump);
						aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, data.Label.value_or(PUMP));

						if (data.Status.has_value())
						{
							aux_ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::PumpStatusTrait{}, Kernel::ConvertToPumpStatus(data.Status.value()));
						}
					},
					[&aux_ptr](SpilloverDevice_Data data)
					{
						aux_ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Spillover);
						aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, data.Label.value_or(SPILLOVER));
					},
					[&aux_ptr](SprinklerDevice_Data data)
					{
						aux_ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Sprinkler);
						aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, data.Label.value_or(SPRINKLER));
					},
					[&aux_ptr](UnknownDevice_Data data)
					{
						aux_ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Unknown);
						aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, data.Label.value_or(UNKNOWN));
					}
				},
				device_data
			);

			return aux_ptr;
		}
	}

}
// namespace AqualinkAutomate::Factory
