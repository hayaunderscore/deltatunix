#include "config.hpp"
#include "external/glfw/include/GLFW/glfw3.h"
#include "external/utf8.h"
#include "raylib.h"
#include <GLFW/glfw3.h>
#include <kdlpp.h>

#define KDL_UTIL_CHANGEVAR(node, s, vname, kname, targettype) \
	if (node.name() == kname)                                 \
		s.vname = node.args()[0].as<targettype>();

namespace config
{

void IterateAppearanceNode(const kdl::Node &node, ConfigAppearance &appearance)
{
	for (const auto &child : node.children())
	{
		// style
		if (child.name() == u8"style")
		{
			std::u8string val = child.args()[0].as<std::u8string>();
			utf8lwr(val.data());

			if (val == u8"deltarune")
				appearance.style = STYLE_DELTARUNE;
			if (val == u8"touhou")
				appearance.style = STYLE_TOUHOU;
		}

		if (child.name() == u8"text")
		{
			for (const auto &textChild : child.children())
			{
				KDL_UTIL_CHANGEVAR(textChild, appearance.text, textScale, u8"text-scale", float)
				if (textChild.name() == u8"padding")
					for (int i = 0; i < 4; i++)
					{
						appearance.text.padding[i] = textChild.args()[i].as<float>();
					}
				if (textChild.name() == u8"offset")
				{
					appearance.text.offset.x = textChild.args()[0].as<float>();
					appearance.text.offset.y = textChild.args()[1].as<float>();
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
				}
				KDL_UTIL_CHANGEVAR(textChild, appearance.text, slideDistance, u8"slide-distance", float)
			}
		}

		if (child.name() == u8"behavior")
		{
			for (const auto &behaviorChild : child.children())
			{
				KDL_UTIL_CHANGEVAR(behaviorChild, appearance.behavior, changeAnimation, u8"change-animation", bool)
				KDL_UTIL_CHANGEVAR(behaviorChild, appearance.behavior, disappear, u8"disappear", bool)
			}
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
		}
	}
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
