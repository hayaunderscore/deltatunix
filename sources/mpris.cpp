#include "mpris.hpp"
#include "config.hpp"
#include "raylib.h"
#include <sdbus-c++/Message.h>
#include <vector>

extern config::Config g_config;

namespace mpris
{
std::unique_ptr<sdbus::IConnection> connection;

std::string title;
std::string artist;
std::string album;

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

							 updateMetadata(properties);
							 updatePlaybackState(properties);
						 });
}

std::string previousDisplayedText;
std::string buildDisplayedText()
{
	bool touhou = g_config.appearance.style == config::STYLE_TOUHOU;

	std::string text = touhou ? "" : "~   ";
	if (paused && !touhou)
		text = "⏸" + text;
	else
		text = "♪" + text;

	if (!artist.empty())
	{
		text.append(artist);
		if (!title.empty())
		{
			text.append(" - ");
		}
	}
	if (!title.empty())
	{
		text.append(title);
	}

	return text;
}

void updateMetadata(const variant_map &properties)
{
	if (properties.count("Metadata") == 0)
		return;

	title = album = artist = "";

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
	}

	std::string builtText = buildDisplayedText();
	if (previousDisplayedText != builtText)
	{
		metadataCallback();
		previousDisplayedText = builtText;
	}

	TraceLog(LOG_INFO, TextFormat("MPRIS: Updated metadata (%s | %s | %s)", title.c_str(), artist.c_str(), album.c_str()));
}

bool lastPaused = false;
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
	if (paused != lastPaused)
	{
		lastPaused = paused;
		pausedCallback(paused);
	}
}

void updateConnection() { connection->processPendingEvent(); }

void leave()
{
	// TODO
}
} // namespace mpris
