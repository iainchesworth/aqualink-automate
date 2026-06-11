#include "startup/jandy_startup_planner.h"

namespace AqualinkAutomate::Jandy::Startup
{
	const std::vector<std::uint8_t> StartupPlanner::AQUALINKTOUCH_IDS{ 0x33, 0x32, 0x31, 0x30 };
	const std::vector<std::uint8_t> StartupPlanner::ONETOUCH_IDS{ 0x41, 0x40, 0x42, 0x43 };
	const std::vector<std::uint8_t> StartupPlanner::PDA_IDS{ 0x60, 0x61, 0x62, 0x63 };
	const std::vector<std::uint8_t> StartupPlanner::SERIALADAPTER_IDS{ 0x48, 0x49 };

	namespace
	{
		bool AnyInRange(const std::set<std::uint8_t>& s, std::uint8_t lo, std::uint8_t hi)
		{
			for (std::uint8_t a = lo; a <= hi; ++a)
			{
				if (s.contains(a))
				{
					return true;
				}
			}
			return false;
		}
	}
	// unnamed namespace

	PanelProfile StartupPlanner::DeriveProfile(const std::set<std::uint8_t>& probed, const std::string& model, const std::string& revision)
	{
		PanelProfile profile;
		profile.model = model;
		profile.revision = revision;
		profile.probes_aqualinktouch = AnyInRange(probed, 0x30, 0x33);
		profile.probes_onetouch = AnyInRange(probed, 0x40, 0x43);
		profile.probes_pda = AnyInRange(probed, 0x60, 0x63);
		profile.has_iaqualink2_slot = probed.contains(0xA3);
		return profile;
	}

	StartupPlan StartupPlanner::Plan(const PanelProfile& profile)
	{
		using DeviceType = Devices::JandyEmulatedDeviceTypes;

		StartupPlan plan;

		// The SerialAdapter is always useful and never collides with the controller emulation:
		// it sources the panel model and is the command channel. If a real adapter answers it
		// self-suppresses (Emulated::SuppressEmulation), so it is safe to stand up first.
		plan.devices.push_back(PlannedDevice{ DeviceType::SerialAdapter, SERIALADAPTER_IDS, 0x00, false, "panel model + command channel" });

		if (profile.probes_aqualinktouch)
		{
			plan.method = DataGatheringMethod::PagePush;
			plan.devices.push_back(PlannedDevice{ DeviceType::IAQ, AQUALINKTOUCH_IDS, 0x00, false, "live status via AqualinkTouch page-push (no menu crawl)" });
			plan.rationale = "master probes the AqualinkTouch range (0x30-0x33): source status from pushed pages, navigating to specific pages on demand";
		}
		else if (profile.probes_onetouch)
		{
			plan.method = DataGatheringMethod::MenuSpider;
			plan.devices.push_back(PlannedDevice{ DeviceType::OneTouch, ONETOUCH_IDS, 0x00, false, "live status via OneTouch menu scrape" });
			plan.rationale = "master probes the OneTouch range (0x40-0x43) but not AqualinkTouch: fall back to autonomous menu spidering";
		}
		else if (profile.probes_pda)
		{
			plan.method = DataGatheringMethod::PdaGraph;
			plan.devices.push_back(PlannedDevice{ DeviceType::PDA, PDA_IDS, 0x00, false, "live status via PDA page-graph scrape" });
			plan.rationale = "master probes the PDA range (0x60-0x63): use the PDA page-graph scraper";
		}
		else
		{
			plan.method = DataGatheringMethod::ObserveOnly;
			plan.rationale = "no emulatable controller slot probed: passive decode only";
		}

		return plan;
	}

	std::optional<std::uint8_t> StartupPlanner::SelectFreeAddress(const std::vector<std::uint8_t>& candidates, const std::set<std::uint8_t>& occupied)
	{
		for (auto id : candidates)
		{
			if (!occupied.contains(id))
			{
				return id;
			}
		}
		return std::nullopt;
	}

	void StartupPlanner::ResolveAddresses(StartupPlan& plan, const std::set<std::uint8_t>& occupied)
	{
		// Claim each resolved id as we go so two planned devices never land on the same slot.
		std::set<std::uint8_t> taken = occupied;

		for (auto& device : plan.devices)
		{
			if (auto id = SelectFreeAddress(device.candidate_ids, taken); id.has_value())
			{
				device.selected_id = id.value();
				device.resolved = true;
				taken.insert(id.value());
			}
			else
			{
				device.selected_id = 0x00;
				device.resolved = false;
			}
		}
	}

}
// namespace AqualinkAutomate::Jandy::Startup
