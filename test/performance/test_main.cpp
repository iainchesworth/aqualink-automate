#include <benchmark/benchmark.h>

#include "logging/logging.h"
#include "logging/logging_initialise.h"
#include "logging/logging_severity_filter.h"

using namespace AqualinkAutomate;

int main(int argc, char** argv) 
{
    // Turn off all logging barring FATAL messages to help not skew performance tests
    // by writting logging messages to the console.

    Logging::SeverityFiltering::SetGlobalFilterLevel(Logging::Severity::Fatal);
    Logging::Initialise();
    
    benchmark::Initialize(&argc, argv);
    
    if (benchmark::ReportUnrecognizedArguments(argc, argv))
    {
        return 1;
    }
 
    benchmark::RunSpecifiedBenchmarks();
}
