#pragma once

#include "formatscript.hpp"
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

enum AppearanceVerticalAlignment
{
	ALIGN_TOP,
	ALIGN_VCENTER,
	ALIGN_BOTTOM,
};

enum AppearanceStyle
{
	STYLE_NONE = -1, // This doesn't actually get used, but its a placeholder value sooo
	STYLE_DELTARUNE,
	STYLE_TOUHOU,
};

struct AppearancePadding
{
	float top, right, bottom, left;
};

struct AppearanceTextEffects
{
	float shadow;
	Color shadowColor = BLACK;
	float outline;
	Color outlineColor = BLACK;
};

struct ConfigAppearanceText
{
	float textScale;
	float textSize = 16.0f;
	std::string font = "ipag.ttf";
	Color color = WHITE;
	AppearanceTextEffects effects;
	AppearancePadding padding;
	Vector2 offset;
	AppearanceAlignment align = ALIGN_RIGHT;
	AppearanceVerticalAlignment valign = ALIGN_TOP;
	fsc::Template format;
	std::unordered_map<std::string, std::string> formatArguments;
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
	AppearanceStyle style;
	ConfigAppearanceText text;
	ConfigAppearanceBehavior behavior;
	ConfigAppearanceTiming timing;
};

struct ConfigGeneral
{
	int monitor;
	bool useWorkArea = true;
	std::vector<std::string> blacklist;
	std::vector<std::string> whitelist;
};

struct Config
{
	ConfigAppearance appearance;
	ConfigGeneral general;
};

Config LoadConfig(std::string path);
void SaveConfig(std::string path);

} // namespace config
