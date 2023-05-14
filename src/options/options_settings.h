#pragma once

#include "options/options_app_options.h"
#include "options/options_developer_options.h"
#include "options/options_emulated_device_options.h"
#include "options/options_serial_options.h"
#include "options/options_web_options.h"

namespace AqualinkAutomate::Options
{
	typedef struct
	{
		App::Settings app;
		Developer::Settings developer;
		Emulated::Settings emulated_device;
		Serial::Settings serial;
		Web::Settings web;
	}
	Settings;

}
// namespace AqualinkAutomate::Options
