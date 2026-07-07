#include "mpris.hpp"
#include "config.hpp"
#include "external/utf8.h"
#include "formatscript.hpp"
#include "raylib.h"
#include <optional>
#include <rapidfuzz/fuzz.hpp>
#include <sdbus-c++/Message.h>
#include <utility>
#include <vector>

extern config::Config g_config;

namespace mpris
{
std::unique_ptr<sdbus::IConnection> connection;

std::string title;
std::string artist;
std::string album;
std::string albumArtist;

bool playing;
bool paused;

std::function<void()> metadataCallback;
std::function<void(bool)> pausedCallback;
std::function<void()> stoppedCallback;

void start()
{
	connection = sdbus::createSessionBusConnection();

	// we listen to all paths that use mpris
	// but only listen for the PropertiesChanged signal
	std::string matchRule = "type='signal',"
							"path='/org/mpris/MediaPlayer2',"
							"member='PropertiesChanged'";

	connection->addMatch(matchRule,
						 [](sdbus::Message msg)
						 {
							 std::string interface;
							 std::map<std::string, sdbus::Variant> properties;

							 // Map these to the interface and properties
							 msg >> interface >> properties;

							 if (!proceedIdentity(getPlayerIdentity(msg.getSender())))
								 return;

							 updateMetadata(properties);
							 updatePlaybackState(properties);
						 });
}

std::string previousDisplayedText;
std::string buildDisplayedText()
{
	fsc::Context context = {
		{
			{"artist", artist},
			{"title", title},
			{"album", album},
			{"albumArtist", albumArtist},
			{"paused", paused ? "true" : "false"},
		}};

	return fsc::render(g_config.appearance.text.format, context);
}

void updateMetadata(const variant_map &properties)
{
	if (properties.count("Metadata") == 0)
		return;

	title = album = artist = albumArtist = "";

	auto metadata = properties.at("Metadata").get<variant_map>();
	for (const auto &[k, v] : metadata)
	{
		if (k == "xesam:title")
			title = v.get<std::string>();
		if (k == "xesam:album")
			album = v.get<std::string>();
		if (k == "xesam:artist")
		{
			// i THINK this is a string array
			// fuck it
			for (const auto &a : v.get<std::vector<std::string>>())
			{
				if (!artist.empty())
					artist += ", ";
				artist += a;
			}
		}
		if (k == "xesam:albumArtist")
		{
			// i THINK this is a string array
			// fuck it
			for (const auto &a : v.get<std::vector<std::string>>())
			{
				if (!albumArtist.empty())
					albumArtist += ", ";
				albumArtist += a;
			}
		}
	}

	std::string builtText = title + album + artist;
	if (previousDisplayedText != builtText)
	{
		metadataCallback();
		previousDisplayedText = builtText;
	}

	TraceLog(LOG_INFO, TextFormat("MPRIS: Updated metadata (%s | %s | %s)", title.c_str(), artist.c_str(), album.c_str()));
}

bool lastPaused = false;
std::string lastStatus = "";
void updatePlaybackState(const variant_map &properties)
{
	if (properties.count("PlaybackStatus") == 0)
		return;
	std::string status = properties.at("PlaybackStatus").get<std::string>();
	playing = status == "Playing";
	paused = status != "Playing";
	if (status == "Stopped")
	{
		paused = false;
		stoppedCallback();
		previousDisplayedText = "";
	}
	if (lastStatus == "Stopped" && status == "Playing") // Call metadata again, idiot.
		metadataCallback();
	if (paused != lastPaused)
	{
		lastPaused = paused;
		pausedCallback(paused);
	}
	lastStatus = status;
}

std::string getPlayerIdentity(const std::string &sender)
{
	try
	{
		auto proxy = sdbus::createProxy(*connection, sdbus::ServiceName{sender}, sdbus::ObjectPath{"/org/mpris/MediaPlayer2"});
		std::string val = proxy->getProperty("Identity").onInterface("org.mpris.MediaPlayer2").get<std::string>();
		utf8lwr((char8_t *)(val.data()));
		return val;
	}
	catch (const sdbus::Error &e)
	{
		// TraceLog(LOG_INFO, TextFormat("MPRIS: Failed to get player identity: %s", e.getMessage().c_str()));
		return {};
	}
}

std::optional<std::pair<std::string, double>> fuzzyFindIdentity(
	const std::string &query,
	const std::vector<std::string> &values,
	const double cutoff)
{
	bool matchFound = false;
	double best = cutoff;
	std::string bestMatch;

	rapidfuzz::fuzz::CachedTokenSortRatio<std::string::value_type> scorer(query);

	for (const auto &value : values)
	{
		double score = scorer.similarity(value, best);
		if (score >= best)
		{
			bestMatch = value;
			best = score;
			matchFound = true;
		}
	}

	if (!matchFound)
		return std::nullopt;

	return std::make_pair(bestMatch, best);
}

bool proceedIdentity(const std::string &identity)
{
	// No need to check if blacklist is empty anyways...
	if (g_config.general.blacklist.size() == 0)
		return true;

	std::optional<std::pair<std::string, double>> fuzzed = fuzzyFindIdentity(identity, g_config.general.blacklist, 60);
	if (not fuzzed.has_value())
		return true;

	// TraceLog(LOG_INFO, TextFormat("MPRIS: Fuzzer result: %s (%.f)", fuzzed.value().first.c_str(), fuzzed.value().second));

	if (fuzzed.value().second >= 60)
		return false;

	return true;
}

void updateConnection() { connection->processPendingEvent(); }

void leave()
{
	// TODO
}
} // namespace mpris
