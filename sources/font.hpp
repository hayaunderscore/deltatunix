#pragma once

#include <fontconfig/fontconfig.h>
#include <raylib.h>
#include <string>
#include <unordered_map>

#include "config.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace font
{

extern FcConfig *fconf;										// Fontconfig config
extern config::Config *dconf;								// Reference to a config struct
extern Font tuneFont;										// The base font (MusicTitleFont.fnt)
extern Font tuneFallbackFont;								// Fallback font, usually for missing JP characters (ShinonomeGothic.fnt)
extern std::unordered_map<unsigned int, Font> outlineFonts; // Fonts, but its an outline instead
extern FT_Library ftLibrary;								// Freetype library object

void init(config::Config *conf);
Image FT_BitmapToImage(const FT_Bitmap *bmp);
Font generateFontByFontName(std::string font, int fontSize, const int *codepoints = NULL, int codepointCount = 0, float outlineSize = 0);
void unloadFont(Font font);
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
