#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

#include "developer/mock_serial_port_impl.h"

using namespace AqualinkAutomate::Developer;

//=============================================================================
// Replay-format parser tests for MockSerialPortImpl.
//
// The replayer must consume BOTH on-disk formats so that a capture produced by
// RecordingSerialPortImpl can be replayed as-is:
//
//   * Recorder format : "# comment"  and  "[<ts>] <DIR> 0x##|0x##|..."
//                       where <DIR> is R (read = from device) or W (to device).
//   * Legacy format   : bare "0x##|0x##|..."  (back-compat).
//
// Only R-direction bytes (and legacy bare lines) are the incoming stream; W
// lines, '#' comments and blank lines must be skipped.
//=============================================================================

namespace
{
	// Write text to a uniquely-named temp .cap file and return its path.  The
	// caller is responsible for nothing; a RemoveOnScope tidies it up.
	struct TempCaptureFile
	{
		std::filesystem::path path;

		explicit TempCaptureFile(const std::string& contents)
		{
			static int counter = 0;
			path = std::filesystem::temp_directory_path() /
				std::filesystem::path(std::string("aa_replay_fmt_") + std::to_string(++counter) + ".cap");

			std::ofstream out(path, std::ios::binary | std::ios::trunc);
			out << contents;
			out.close();
		}

		~TempCaptureFile()
		{
			std::error_code ec;
			std::filesystem::remove(path, ec);
		}

		TempCaptureFile(const TempCaptureFile&) = delete;
		TempCaptureFile& operator=(const TempCaptureFile&) = delete;
	};

	// Open a MockSerialPortImpl against the given capture file (file mode kicks
	// in because the path exists) and drain every replayable byte.
	std::vector<uint8_t> DrainReplay(const std::filesystem::path& cap_path)
	{
		MockSerialPortImpl port;
		boost::system::error_code ec;
		port.open(cap_path.string(), ec);
		BOOST_REQUIRE(!ec);
		BOOST_REQUIRE(port.is_open());

		std::vector<uint8_t> out;
		std::array<uint8_t, 8> chunk{}; // small chunk to exercise multi-read buffering

		for (;;)
		{
			boost::system::error_code read_ec;
			const auto n = port.read_some(boost::asio::buffer(chunk), read_ec);
			if (n > 0)
			{
				out.insert(out.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(n));
			}
			if (read_ec || (0 == n))
			{
				break;
			}
		}

		return out;
	}
}

BOOST_AUTO_TEST_SUITE(MockSerialReplayFormat_TestSuite)

// -----------------------------------------------------------------------------
// Legacy bare format still works (back-compat).
// -----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(LegacyBareFormat_IsParsed)
{
	TempCaptureFile cap("0x10|0x02|0x48|0x02|0x10|0x03\n");
	auto bytes = DrainReplay(cap.path);

	const std::vector<uint8_t> expected{ 0x10, 0x02, 0x48, 0x02, 0x10, 0x03 };
	BOOST_TEST(bytes == expected, boost::test_tools::per_element());
}

// -----------------------------------------------------------------------------
// Legacy format spanning multiple bare lines concatenates in order.
// -----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(LegacyBareFormat_MultipleLines_Concatenate)
{
	TempCaptureFile cap("0x10|0x02\n0x48|0x02\n");
	auto bytes = DrainReplay(cap.path);

	const std::vector<uint8_t> expected{ 0x10, 0x02, 0x48, 0x02 };
	BOOST_TEST(bytes == expected, boost::test_tools::per_element());
}

// -----------------------------------------------------------------------------
// Recorder format: R-direction lines are replayed.
// -----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(RecorderFormat_RLines_AreReplayed)
{
	TempCaptureFile cap(
		"[         0] R 0x10|0x02|0x48|0x02\n"
		"[       100] R 0x10|0x03\n");
	auto bytes = DrainReplay(cap.path);

	const std::vector<uint8_t> expected{ 0x10, 0x02, 0x48, 0x02, 0x10, 0x03 };
	BOOST_TEST(bytes == expected, boost::test_tools::per_element());
}

// -----------------------------------------------------------------------------
// Recorder format: W-direction lines (app output) are NOT replayed as input.
// -----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(RecorderFormat_WLines_AreSkipped)
{
	TempCaptureFile cap(
		"[         0] W 0xaa|0xbb|0xcc\n"     // must be skipped entirely
		"[       100] R 0x10|0x02\n"
		"[       200] W 0xde|0xad\n"          // must be skipped entirely
		"[       300] R 0x48|0x03\n");
	auto bytes = DrainReplay(cap.path);

	const std::vector<uint8_t> expected{ 0x10, 0x02, 0x48, 0x03 };
	BOOST_TEST(bytes == expected, boost::test_tools::per_element());
}

// -----------------------------------------------------------------------------
// Comment / header ('#') and blank lines are skipped.
// -----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(CommentAndBlankLines_AreSkipped)
{
	TempCaptureFile cap(
		"# Serial recording started at: whenever\n"
		"# Format: [timestamp_ms] direction byte|byte|...\n"
		"#\n"
		"\n"
		"[         0] R 0x10|0x02\n"
		"   \n"                                  // whitespace-only line
		"# a trailing comment\n"
		"[       100] R 0x48|0x03\n"
		"# Recording ended\n");
	auto bytes = DrainReplay(cap.path);

	const std::vector<uint8_t> expected{ 0x10, 0x02, 0x48, 0x03 };
	BOOST_TEST(bytes == expected, boost::test_tools::per_element());
}

// -----------------------------------------------------------------------------
// Full recorder header + mixed R/W traffic: only R bytes survive, in order.
// This is the canonical "record -> replay-as-fixture" shape.
// -----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(FullRecorderFile_OnlyRBytesInOrder)
{
	TempCaptureFile cap(
		"# Serial recording started at: Sun Jun 08 12:00:00 2026\n"
		"# Format: [timestamp_ms] direction byte|byte|byte|...\n"
		"# Direction: R=read (from device), W=write (to device)\n"
		"#\n"
		"[         0] # Opened device: /dev/ttyUSB0\n"
		"[       100] W 0x10|0x02|0x50|0x14|0x76|0x10|0x03\n"
		"[       142] R 0x10|0x02|0x50|0x14|0x76|0x10|0x03\n"
		"[       310] R 0x10|0x02|0x50|0x11|0x28|0x9b|0x10|0x03\n"
		"# Recording ended\n");
	auto bytes = DrainReplay(cap.path);

	const std::vector<uint8_t> expected{
		0x10, 0x02, 0x50, 0x14, 0x76, 0x10, 0x03,
		0x10, 0x02, 0x50, 0x11, 0x28, 0x9b, 0x10, 0x03,
	};
	BOOST_TEST(bytes == expected, boost::test_tools::per_element());
}

// -----------------------------------------------------------------------------
// Both formats interleaved in one file are accepted together.
// -----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(MixedLegacyAndRecorderFormat_BothAccepted)
{
	TempCaptureFile cap(
		"0x10|0x02\n"                       // legacy bare
		"[       100] R 0x48|0x03\n"        // recorder R
		"[       200] W 0xff|0xff\n"        // recorder W (skipped)
		"0x55|0xaa\n");                     // legacy bare
	auto bytes = DrainReplay(cap.path);

	const std::vector<uint8_t> expected{ 0x10, 0x02, 0x48, 0x03, 0x55, 0xaa };
	BOOST_TEST(bytes == expected, boost::test_tools::per_element());
}

// -----------------------------------------------------------------------------
// Malformed tokens within an R line are skipped without aborting the rest.
// -----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(MalformedTokens_AreSkipped_RemainderSurvives)
{
	TempCaptureFile cap(
		"[         0] R 0x10|ZZ|0x02|0xQQ|0x48\n");
	auto bytes = DrainReplay(cap.path);

	const std::vector<uint8_t> expected{ 0x10, 0x02, 0x48 };
	BOOST_TEST(bytes == expected, boost::test_tools::per_element());
}

// -----------------------------------------------------------------------------
// A short capture (fewer bytes than the read chunk) is not truncated.  Guards
// the read_some partial-buffer-vs-EOF fix: a small recording must replay in
// full instead of being discarded when EOF is hit mid-buffer.
// -----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(ShortCapture_FewerBytesThanChunk_NotTruncated)
{
	// 6 bytes; DrainReplay above uses an 8-byte chunk, so EOF lands mid-buffer.
	TempCaptureFile cap("[0] R 0x10|0x02|0x50|0x11|0x10|0x03\n");
	auto bytes = DrainReplay(cap.path);

	const std::vector<uint8_t> expected{ 0x10, 0x02, 0x50, 0x11, 0x10, 0x03 };
	BOOST_TEST(bytes == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END()
