#pragma once

#include <fontconfig/fontconfig.h>
#include <raylib.h>
#include <string>

#include "config.hpp"

namespace font
{

extern FcConfig *fconf;		  // Fontconfig config
extern config::Config *dconf; // Reference to a config struct
extern Font tuneFont;		  // The base font (MusicTitleFont.fnt)
extern Font tuneFallbackFont; // Fallback font, usually for missing JP characters (ShinonomeGothic.fnt)

void init(config::Config *conf);
Font generateFontByFontName(std::string font, int fontSize, const int *codepoints = NULL, int codepointCount = 0);
void drawDeltatuneFont(const std::string &text, Vector2 position, Color tint = WHITE);
void drawTruetypeFont(
	const Font &font,
	const std::string &text,
	Vector2 position,
	int fontSize,
	Color tint,
	float shadow = 0,
	Color shadowTint = BLACK,
	float outline = 0,
	Color outlineTint = WHITE);
void deinit();

} // namespace font
