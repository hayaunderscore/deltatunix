#include <functional>
#include <memory>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/sdbus-c++.h>

#pragma header

namespace mpris
{

typedef std::map<std::string, sdbus::Variant> variant_map;

/*
 * The current dbus connection.
 */
extern std::unique_ptr<sdbus::IConnection> connection;

/*
 * The title of the current song.
 */
extern std::string title;
/*
 * The artist of the current song.
 */
extern std::string artist;
/*
 * The album of the current song.
 * Songs with multiple artists are separated by ", ".
 */
extern std::string album;

extern bool playing;
extern bool paused;

extern std::function<void()> metadataCallback;
extern std::function<void(bool)> pausedCallback;
extern std::function<void()> stoppedCallback;

/*
 * Starts the connection.
 */
void start();
/*
 * Updates the current connection and listens for any changes.
 */
void updateConnection();
/*
 * Updates the metadata according to a sent properties list.
 */
void updateMetadata(const variant_map &properties);
/*
 * Updates playback state.
 */
void updatePlaybackState(const variant_map &properties);
/*
 * Closes the connection and frees up leftover resources.
 * Not used at the moment.
 */
void leave();

std::string buildDisplayedText();
extern std::string previousDisplayedText;
} // namespace mpris
