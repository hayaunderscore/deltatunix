#pragma once

#include "raymath.h"

void WindowUnfocus();
void WindowHideFromTaskbar();
Vector2 GetDeterministicMonitorPosition(int monitor, bool videomode);
int GetDeterministicMonitorWidth(int monitor, bool videomode);
int GetDeterministicMonitorHeight(int monitor, bool videomode);
