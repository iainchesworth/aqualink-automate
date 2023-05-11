#include "jandy/generator/jandy_rawdata_generator.h"
#include "logging/logging.h"
#include "profiling/profiling.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Generators
{

	JandyRawDataGenerator::JandyRawDataGenerator() :
		Interfaces::IGenerator_MessageToRawData<Interfaces::ISerialPort::DataType, Types::JandyMessageTypePtr>()
	{
		static_cast<void>(Factory::ProfilerFactory::Instance().Get()->CreateDomain("JandyRawDataGenerator"));
	}

	boost::asio::awaitable<void> JandyRawDataGenerator::GenerateRawDataFromMessage(const Types::JandyMessageTypePtr& msg)
	{
		co_return;
	}

}
// namespace AqualinkAutomate::Generators
