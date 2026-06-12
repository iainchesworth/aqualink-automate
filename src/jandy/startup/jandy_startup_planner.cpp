#include "startup/jandy_startup_planner.h"

#include <format>
#include <string>
#include <vector>

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

		// Human-readable summary of the revision-gated peripherals, for the plan rationale.
		std::string RevisionFeatureSummary(const RevisionCapabilities& caps)
		{
			std::vector<std::string> features;
			if (caps.variable_speed_pumps)  { features.emplace_back("VS pumps"); }
			if (caps.chemlink_chlorinators) { features.emplace_back("ChemLink/AutoClear"); }
			if (caps.aqualink_touch)        { features.emplace_back("AqualinkTouch"); }
			if (caps.iaqualink_cloud)       { features.emplace_back("iAquaLink"); }
			if (caps.addressed_vs_pumps)    { features.emplace_back("16 addressed VS pumps"); }
			if (caps.infinite_watercolors)  { features.emplace_back("Infinite WaterColors"); }

			if (features.empty())
			{
				return "no CPU-era peripherals";
			}

			std::string out;
			for (std::size_t i = 0; i < features.size(); ++i)
			{
				if (i != 0) { out += ", "; }
				out += features[i];
			}
			return out;
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
		profile.revision_caps = DeriveRevisionCapabilities(revision);
		return profile;
	}

	StartupPlan StartupPlanner::Plan(const PanelProfile& profile)
	{
		using DeviceType = Devices::JandyEmulatedDeviceTypes;

		StartupPlan plan;
		plan.revision_caps = profile.revision_caps;

		// Genuine contradictions worth flagging: the master probes a controller range the
		// revision cannot support -- AqualinkTouch probed but pre-Rev-Q, or OneTouch probed but
		// pre-Rev-I. (The converse -- a capable revision with that UI simply not installed, so
		// not probed -- is normal and NOT an inconsistency.)
		if (profile.revision_caps.is_known)
		{
			const bool touch_contradiction = profile.probes_aqualinktouch && !profile.revision_caps.aqualink_touch;
			const bool onetouch_contradiction = profile.probes_onetouch && !profile.revision_caps.onetouch_support;
			plan.revision_consistent = !(touch_contradiction || onetouch_contradiction);
		}

		const bool no_controller_probed = !profile.probes_aqualinktouch && !profile.probes_onetouch && !profile.probes_pda;

		// The SerialAdapter is always useful and never collides with the controller emulation:
		// it sources the panel model and is the command channel. If a real adapter answers it
		// self-suppresses (Emulated::SuppressEmulation), so it is safe to stand up first.
		plan.devices.push_back(PlannedDevice{ DeviceType::SerialAdapter, SERIALADAPTER_IDS, 0x00, false, "panel model + command channel" });

		if (profile.probes_aqualinktouch || (no_controller_probed && profile.revision_caps.aqualink_touch))
		{
			plan.method = DataGatheringMethod::PagePush;
			plan.devices.push_back(PlannedDevice{ DeviceType::IAQ, AQUALINKTOUCH_IDS, 0x00, false, "live status via AqualinkTouch page-push (no menu crawl)" });
			plan.rationale = profile.probes_aqualinktouch
				? "master probes the AqualinkTouch range (0x30-0x33): source status from pushed pages, navigating to specific pages on demand"
				: "no controller probed yet, but revision is Touch-capable (Rev Q+): provisionally use AqualinkTouch page-push, to be confirmed once the touch range is probed";
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
			plan.rationale = "no emulatable controller slot probed and revision not Touch-capable: passive decode only";
		}

		// Append a revision summary so the choice (and the expected peripherals) is auditable.
		if (profile.revision_caps.is_known)
		{
			plan.rationale += std::format(" [Rev {} ({}): {}]",
				profile.revision_caps.revision_letter,
				profile.revision_caps.cpu_board ? "CPU" : "PPD",
				RevisionFeatureSummary(profile.revision_caps));

			if (!plan.revision_consistent)
			{
				plan.rationale += " -- WARNING: a probed controller range is unsupported by this revision (AqualinkTouch needs Rev Q+, OneTouch needs Rev I+)";
			}
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
