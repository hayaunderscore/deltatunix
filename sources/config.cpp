#include "config.hpp"
#include "external/glfw/include/GLFW/glfw3.h"
#include "external/utf8.h"
#include "raylib.h"
#include <GLFW/glfw3.h>
#include <kdlpp.h>
#include <string>

#define KDL_UTIL_CHANGEVAR(node, s, vname, kname, targettype) \
	if (node.name() == kname)                                 \
	{                                                         \
		s.vname = node.args()[0].as<targettype>();            \
		continue;                                             \
	}
#define KDL_UTIL_CHANGEVARSTRING(node, s, vname, kname)         \
	if (node.name() == kname)                                   \
	{                                                           \
		std::u8string val = node.args()[0].as<std::u8string>(); \
		s.vname = std::string(val.begin(), val.end());          \
	}

namespace config
{

inline constexpr const char *DEFAULT_FORMAT_DELTARUNE = "<BOL(paused,⏸,♪)~   <EXS(artist,{artist} - ,){title}";
inline constexpr const char *DEFAULT_FORMAT_TOUHOU = "♪<EXS(artist,{artist} - ,){title}";

Color ColorFromHexCode(std::u8string hex, Color defaultColor = BLACK)
{
	if (hex.empty())
		return defaultColor;
	if (hex[0] == u8'#')
		hex = hex.substr(1);

	unsigned int clr = (unsigned int)std::stoul(std::string(hex.begin(), hex.end()), nullptr, 16);
	if (hex.length() == 6)
		clr = (clr << 8) | 0xFF; // Shift left 8 bits and set it to FF (255)
	return GetColor(clr);
}

AppearanceStyle GetStyleFromString(std::u8string val, AppearanceStyle defaultStyle = STYLE_DELTARUNE)
{
	utf8lwr(val.data());
	if (val == u8"deltarune")
		return STYLE_DELTARUNE;
	if (val == u8"touhou")
		return STYLE_TOUHOU;
	return defaultStyle;
}

void IterateAppearanceNode(const kdl::Node &node, ConfigAppearance &appearance)
{
	for (const auto &child : node.children())
	{
		// style
		if (child.name() == u8"style")
		{
			std::u8string val = child.args()[0].as<std::u8string>();
			appearance.style = GetStyleFromString(val);
			if (appearance.text.format.empty())
				appearance.text.format = fsc::Parser(appearance.style == STYLE_DELTARUNE ? DEFAULT_FORMAT_DELTARUNE : DEFAULT_FORMAT_TOUHOU).parse();
			continue;
		}

		if (child.name() == u8"text")
		{
			AppearanceStyle style = appearance.style;
			if (child.args().size() > 0)
				style = GetStyleFromString(child.args()[0].as<std::u8string>(), appearance.style);

			if (appearance.style != style)
				continue;

			for (const auto &textChild : child.children())
			{
				KDL_UTIL_CHANGEVAR(textChild, appearance.text, textScale, u8"text-scale", float)
				KDL_UTIL_CHANGEVAR(textChild, appearance.text, textSize, u8"text-size", float)
				KDL_UTIL_CHANGEVARSTRING(textChild, appearance.text, font, u8"font")
				KDL_UTIL_CHANGEVAR(textChild, appearance.text.effects, shadow, u8"shadow", float)
				KDL_UTIL_CHANGEVAR(textChild, appearance.text.effects, outline, u8"outline", float)

				if (textChild.name() == u8"color")
				{
					appearance.text.color = ColorFromHexCode(textChild.args()[0].as<std::u8string>(), WHITE);
					continue;
				}
				if (textChild.name() == u8"shadow-color")
				{
					appearance.text.effects.shadowColor = ColorFromHexCode(textChild.args()[0].as<std::u8string>(), BLACK);
					continue;
				}
				if (textChild.name() == u8"outline-color")
				{
					appearance.text.effects.outlineColor = ColorFromHexCode(textChild.args()[0].as<std::u8string>(), BLACK);
					continue;
				}

				if (textChild.name() == u8"padding")
				{
					appearance.text.padding.top = textChild.args()[0].as<float>();
					appearance.text.padding.right = textChild.args()[1].as<float>();
					appearance.text.padding.bottom = textChild.args()[2].as<float>();
					appearance.text.padding.left = textChild.args()[3].as<float>();
					continue;
				}
				if (textChild.name() == u8"offset")
				{
					appearance.text.offset.x = textChild.args()[0].as<float>();
					appearance.text.offset.y = textChild.args()[1].as<float>();
					continue;
				}
				if (textChild.name() == u8"align")
				{
					// TODO lowercase
					std::u8string val = textChild.args()[0].as<std::u8string>();
					utf8lwr(val.data());

					if (val == u8"left")
						appearance.text.align = ALIGN_LEFT;
					if (val == u8"center")
						appearance.text.align = ALIGN_CENTER;
					if (val == u8"right")
						appearance.text.align = ALIGN_RIGHT;
					continue;
				}
				if (textChild.name() == u8"valign")
				{
					// TODO lowercase
					std::u8string val = textChild.args()[0].as<std::u8string>();
					utf8lwr(val.data());

					if (val == u8"top")
						appearance.text.valign = ALIGN_TOP;
					if (val == u8"center")
						appearance.text.valign = ALIGN_VCENTER;
					if (val == u8"bottom")
						appearance.text.valign = ALIGN_BOTTOM;
					continue;
				}
				KDL_UTIL_CHANGEVAR(textChild, appearance.text, slideDistance, u8"slide-distance", float)

				if (textChild.name() == u8"format")
				{
					std::u8string val = textChild.args()[0].as<std::u8string>();
					appearance.text.format = fsc::Parser(std::string(val.begin(), val.end())).parse();
				}
			}

			continue;
		}

		if (child.name() == u8"behavior")
		{
			for (const auto &behaviorChild : child.children())
			{
				KDL_UTIL_CHANGEVAR(behaviorChild, appearance.behavior, changeAnimation, u8"change-animation", bool)
				KDL_UTIL_CHANGEVAR(behaviorChild, appearance.behavior, disappear, u8"disappear", bool)
			}
			continue;
		}

		if (child.name() == u8"timing")
		{
			for (const auto &timingChild : child.children())
			{
				KDL_UTIL_CHANGEVAR(timingChild, appearance.timing, appearDelay, u8"appear-delay", float)
				KDL_UTIL_CHANGEVAR(timingChild, appearance.timing, appearDuration, u8"appear-duration", float)
				KDL_UTIL_CHANGEVAR(timingChild, appearance.timing, disppearDuration, u8"disappear-duration", float)
				KDL_UTIL_CHANGEVAR(timingChild, appearance.timing, stayTime, u8"stay-time", float)
			}
			continue;
		}
	}

	if (appearance.text.format.empty())
		appearance.text.format = fsc::Parser(appearance.style == STYLE_DELTARUNE ? DEFAULT_FORMAT_DELTARUNE : DEFAULT_FORMAT_TOUHOU).parse();
}

void IterateGeneralNode(const kdl::Node &node, ConfigGeneral &general)
{
	for (const auto &child : node.children())
	{
		if (child.name() == u8"monitor")
		{
			const auto &val = child.args()[0];
			int monitorIndex = 0;
			// If it's a string, use the monitor name
			if (val.type() == kdl::Type::String)
			{
				std::u8string monitorName = val.as<std::u8string>();
				int monitorCount = 0;
				GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);
				for (int i = 0; i < monitorCount; i++)
				{
					GLFWmonitor *monitor = monitors[i];
					if (TextIsEqual(glfwGetMonitorName(monitor), reinterpret_cast<const char *>(monitorName.c_str())))
					{
						monitorIndex = i;
						break;
					}
				}
			}
			// otherwise, just use the raw index
			else if (val.type() == kdl::Type::Number)
			{
				monitorIndex = val.as<int>();
			}

			general.monitor = monitorIndex;
			continue;
		}
		KDL_UTIL_CHANGEVAR(child, general, useWorkArea, u8"use-work-area", bool)
	}
}

Config LoadConfig(std::string path)
{
	Config conf{};
	char *confContent = LoadFileText(path.c_str());
	if (confContent == NULL)
	{
		TraceLog(LOG_ERROR, TextFormat("CONFIG: Failed to load config file (%s)! (Does the config file exist?)", path.c_str()));
		UnloadFileText(confContent);
		return conf;
	}

	kdl::Document doc = kdl::parse(std::u8string_view(reinterpret_cast<char8_t *>(confContent)));
	for (const auto &node : doc.nodes())
	{
		if (node.name() == u8"appearance")
			IterateAppearanceNode(node, conf.appearance);
		if (node.name() == u8"general")
			IterateGeneralNode(node, conf.general);
	}

	UnloadFileText(confContent);
	return conf;
}

} // namespace config
