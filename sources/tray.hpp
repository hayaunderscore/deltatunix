#pragma once

#include <functional>
#include <memory>
#include <string>

// We should only ever use this header if we're building with the system tray.
#ifdef BUILD_SYSTRAY
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>
#endif

namespace tray
{

#ifdef BUILD_SYSTRAY
typedef QMenu tray_menu;
#else
typedef void tray_menu;
#endif

extern bool quitRequested;

void init(int argc, char **argv);
void addAction(std::string name, std::function<void()> callback, bool first = false);
void processEvents();
void quit();

} // namespace tray
