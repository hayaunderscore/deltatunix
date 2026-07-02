#include "tray.hpp"
#include "raylib.h"

#include "tray_about_ui.hpp"

namespace tray
{

bool quitRequested;
static std::unique_ptr<tray_menu> trayMenu;
#ifdef BUILD_SYSTRAY
static std::unique_ptr<QSystemTrayIcon> trayIconPtr;
static std::unique_ptr<QDialog> aboutDialog;
#endif

void init(int argc, char **argv)
{
#ifdef BUILD_SYSTRAY
	if (!QSystemTrayIcon::isSystemTrayAvailable())
	{
		TraceLog(LOG_WARNING, "TRAY: Tray is not available in this environment!");
	}

	trayMenu = std::make_unique<tray_menu>();
	trayIconPtr = std::make_unique<QSystemTrayIcon>(QIcon(":/icon.svg"));

	aboutDialog = std::make_unique<QDialog>();
	Ui::AboutDeltatunix about;
	about.setupUi(aboutDialog.get());
	aboutDialog->connect(about.closeButton, &QPushButton::clicked, [&]() { aboutDialog->hide(); });

	addAction("About DeltaTunix", [&]() { aboutDialog->show(); });
	trayMenu->addSeparator();
	addAction("Quit", [&]() { quitRequested = true; });

	trayIconPtr->setContextMenu(trayMenu.get());
	trayIconPtr->setToolTip("DeltaTunix");
	trayIconPtr->show();

	TraceLog(LOG_INFO, "TRAY: Initialized system tray!");
#endif
}

void addAction(std::string name, std::function<void()> callback, bool first)
{
#ifdef BUILD_SYSTRAY
	QAction *action = new QAction(QString::fromStdString(name));
	if (first && !trayMenu->actions().isEmpty())
	{
		QAction *first = trayMenu->actions().at(0);
		trayMenu->insertAction(first, action);
	}
	else
	{
		trayMenu->addAction(action);
	}
	QObject::connect(action, &QAction::triggered, callback);
#endif
}

void processEvents()
{
#ifdef BUILD_SYSTRAY
	QApplication::processEvents();
#endif
}

void quit()
{
#ifdef BUILD_SYSTRAY
	aboutDialog.reset();
	trayMenu.reset();
	trayIconPtr.reset();
	QApplication::processEvents();
#endif
}

} // namespace tray
