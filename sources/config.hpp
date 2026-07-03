#pragma header

#include "raylib.h"
#include <string>

namespace config
{
enum AppearanceAlignment
{
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT,
};

enum AppearanceStyle
{
	STYLE_DELTARUNE,
	STYLE_TOUHOU,
};

struct ConfigAppearanceText
{
	float textScale;
	float padding[4];
	Vector2 offset;
	AppearanceAlignment align = ALIGN_RIGHT;
	float slideDistance = 24;
};

struct ConfigAppearanceBehavior
{
	bool changeAnimation = true;
	bool disappear;
};

struct ConfigAppearanceTiming
{
	float appearDelay = 0.5;
	float appearDuration = 0.75;
	float disppearDuration = 0.75;
	float stayTime = 2.5;
};

struct ConfigAppearance
{
	std::string style;
	ConfigAppearanceText text;
	ConfigAppearanceBehavior behavior;
	ConfigAppearanceTiming timing;
};

struct ConfigGeneral
{
	int monitor;
};

struct Config
{
	ConfigAppearance appearance;
	ConfigGeneral general;
};

Config LoadConfig(std::string path);
void SaveConfig(std::string path);

} // namespace config
