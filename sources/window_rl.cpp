// This file exists OUTSIDE of window.cpp due to the X11 Font conflict

#include "raylib.h"
#include "window.hpp"
#include <GLFW/glfw3.h>

/*
 * The next functions are shortcuts to raylib monitor management functions
 * While allowing for accounting for workareas n such
 */

Vector2 GetDeterministicMonitorPosition(int monitor, bool videomode)
{
	if (videomode)
		return GetMonitorPosition(monitor);

	int monitorCount = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

	if ((monitor >= 0) && (monitor < monitorCount))
	{
		int x = 0;
		int y = 0;
		glfwGetMonitorWorkarea(monitors[monitor], &x, &y, NULL, NULL);

		return (Vector2){(float)x, (float)y};
	}
	else
		TraceLog(LOG_WARNING, "GLFW: Failed to find selected monitor");
	return (Vector2){0, 0};
}

// Get selected monitor width (currently used by monitor)
int GetDeterministicMonitorWidth(int monitor, bool videomode)
{
	if (videomode)
		return GetMonitorWidth(monitor);

	int width = 0;
	int monitorCount = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

	if ((monitor >= 0) && (monitor < monitorCount))
	{
		glfwGetMonitorWorkarea(monitors[monitor], NULL, NULL, &width, NULL);
	}
	else
		TraceLog(LOG_WARNING, "GLFW: Failed to find selected monitor");

	return width;
}

// Get selected monitor height (currently used by monitor)
int GetDeterministicMonitorHeight(int monitor, bool videomode)
{
	if (videomode)
		return GetMonitorHeight(monitor);

	int height = 0;
	int monitorCount = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

	if ((monitor >= 0) && (monitor < monitorCount))
	{
		glfwGetMonitorWorkarea(monitors[monitor], NULL, NULL, NULL, &height);
	}
	else
		TraceLog(LOG_WARNING, "GLFW: Failed to find selected monitor");

	return height;
}
