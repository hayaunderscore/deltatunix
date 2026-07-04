#include "font.hpp"
#include <cmath>
#include <fontconfig/fontconfig.h>
#include <raylib.h>

// Defined in main.cpp... should really put this somewhere else
std::string GetResourcePath(const std::string &path);

namespace font
{

// If we can't find a font with fontconfig, use this as a fallback.
inline constexpr const char *DEFAULT_FONT = "NotoSansJP-Regular.ttf";

FcConfig *fconf;	   // Fontconfig config
config::Config *dconf; // Reference to a config struct
Font tuneFont;		   // The base font (MusicTitleFont.fnt)
Font tuneFallbackFont; // Fallback font, usually for missing JP characters (ShinonomeGothic.fnt)

void init(config::Config *conf)
{
	dconf = conf;
	// We init Fontconfig here...
	fconf = FcInitLoadConfigAndFonts();
	// Also init bitmap deltarune tune fonts
	tuneFont = LoadFont(GetResourcePath("resources/fonts/MusicTitleFont.fnt").c_str());
	tuneFallbackFont = LoadFont(GetResourcePath("resources/fonts/ShinonomeGothic.fnt").c_str());
}

Font generateFontByFontName(std::string font, int fontSize, const int *codepoints, int codepointCount)
{
	std::string path = GetResourcePath("resources/fonts/" + font);
	if (!FileExists(path.c_str()))
	{
		FcPattern *pat = FcNameParse((FcChar8 *)font.c_str());
		if (!pat)
		{
			TraceLog(LOG_WARNING, TextFormat("FONT: Could not create pattern for font family %s!", font.c_str()));
			goto return_fallback_font;
		}
		FcConfigSubstitute(fconf, pat, FcMatchPattern);
		FcDefaultSubstitute(pat);

		FcFontSet *fs = FcFontSetCreate();
		FcObjectSet *os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, (char *)0);

		FcResult result;
		FcFontSet *fontPatterns = FcFontSort(fconf, pat, FcTrue, 0, &result);

		if (!fontPatterns || fontPatterns->nfont == 0)
		{
			TraceLog(LOG_WARNING, "FONT: Somehow, no fonts exist in your system.");
			if (fontPatterns)
				FcFontSetDestroy(fontPatterns);
			if (os)
				FcObjectSetDestroy(os);
			if (fs)
				FcFontSetDestroy(fs);
			if (pat)
				FcPatternDestroy(pat);
			goto return_fallback_font;
		}

		FcPattern *fontPattern = FcFontRenderPrepare(fconf, pat, fontPatterns->fonts[0]);
		if (!fontPattern)
		{
			TraceLog(LOG_WARNING, TextFormat("FONT: Could not prepare matched font for %s", font.c_str()));
			if (fontPattern)
				FcPatternDestroy(fontPattern);
			if (fontPatterns)
				FcFontSetDestroy(fontPatterns);
			if (os)
				FcObjectSetDestroy(os);
			if (fs)
				FcFontSetDestroy(fs);
			if (pat)
				FcPatternDestroy(pat);
			goto return_fallback_font;
		}
		FcFontSetAdd(fs, fontPattern);

		FcFontSetDestroy(fontPatterns);
		FcPatternDestroy(pat);

		if (fs)
		{
			if (fs->nfont > 0)
			{
				FcValue v;
				FcPattern *fon;

				fon = FcPatternFilter(fs->fonts[0], os);
				FcPatternGet(fon, FC_FILE, 0, &v);
				path = std::string((char *)v.u.f);
				TraceLog(LOG_WARNING, TextFormat("FONT: Found path for matched font %s: %s", font.c_str(), path.c_str()));

				FcPatternDestroy(fon);
			}
			FcFontSetDestroy(fs);
		}

		if (os)
			FcObjectSetDestroy(os);
	}

	return LoadFontEx(path.c_str(), fontSize, codepoints, codepointCount);

return_fallback_font:
	return LoadFontEx(DEFAULT_FONT, fontSize, codepoints, codepointCount);
}

// Edit of MeasureTextEx for fallback fonts
Vector2 MeasureTextExWithFallback(Font font, Font fallbackFont, const char *text, float fontSize, float spacing)
{
	Vector2 textSize = {0};

	if ((font.texture.id == 0) || (text == NULL) || (text[0] == '\0'))
		return textSize; // Security check

	int size = TextLength(text); // Get size in bytes of text
	int tempByteCounter = 0;	 // Used to count longer text line num chars
	int byteCounter = 0;

	float textWidth = 0.0f;
	float tempTextWidth = 0.0f; // Used to count longer text line width

	float textHeight = fontSize;
	float scaleFactor = fontSize / (float)font.baseSize;

	int letter = 0; // Current character
	int index = 0;	// Index position in sprite font

	for (int i = 0; i < size;)
	{
		byteCounter++;

		int codepointByteCount = 0;
		letter = GetCodepointNext(&text[i], &codepointByteCount);
		index = GetGlyphIndex(font, letter);
		Font *chosenFont = &font;

		if (font.glyphs[index].value != letter && IsFontValid(fallbackFont))
		{
			chosenFont = &fallbackFont;
			index = GetGlyphIndex(*chosenFont, letter);
		}

		i += codepointByteCount;

		if (letter != '\n')
		{
			if (chosenFont->glyphs[index].advanceX > 0)
				textWidth += chosenFont->glyphs[index].advanceX;
			else
				textWidth += (chosenFont->recs[index].width + chosenFont->glyphs[index].offsetX);
		}
		else
		{
			if (tempTextWidth < textWidth)
				tempTextWidth = textWidth;
			byteCounter = 0;
			textWidth = 0;

			// NOTE: Line spacing is a global variable, use SetTextLineSpacing() to setup
			textHeight += (fontSize);
		}

		if (tempByteCounter < byteCounter)
			tempByteCounter = byteCounter;
	}

	if (tempTextWidth < textWidth)
		tempTextWidth = textWidth;

	textSize.x = tempTextWidth * scaleFactor + (float)((tempByteCounter - 1) * spacing);
	textSize.y = textHeight;

	return textSize;
}

// Edit of DrawTextEx to add support for an additional fallback font
void DrawTextExWithFallback(Font font, Font fallbackFont, const char *text, Vector2 position, float fontSize, float spacing, Color tint)
{
	if (font.texture.id == 0)
		font = GetFontDefault(); // Security check in case of not valid font

	int size = TextLength(text); // Total size in bytes of the text, scanned by codepoints in loop

	float textOffsetY = 0;	  // Offset between lines (on linebreak '\n')
	float textOffsetX = 0.0f; // Offset X to next character to draw

	float scaleFactor = fontSize / font.baseSize; // Character quad scaling factor

	for (int i = 0; i < size;)
	{
		// Get next codepoint from byte string and glyph index in font
		int codepointByteCount = 0;
		int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
		int index = GetGlyphIndex(font, codepoint);
		Font *chosenFont = &font;

		if (font.glyphs[index].value != codepoint && IsFontValid(fallbackFont))
		{
			chosenFont = &fallbackFont;
			index = GetGlyphIndex(*chosenFont, codepoint);
		}

		if (codepoint == '\n')
		{
			// NOTE: Line spacing is a global variable, use SetTextLineSpacing() to setup
			textOffsetY += (fontSize);
			textOffsetX = 0.0f;
		}
		else
		{
			if ((codepoint != ' ') && (codepoint != '\t'))
			{
				// WHY ARE YOU BIG
				float size = (chosenFont == &font) ? fontSize : 12 * dconf->appearance.text.textScale;
				float addY = (chosenFont == &fallbackFont) ? (3 * dconf->appearance.text.textScale) : 0;
				DrawTextCodepoint(*chosenFont, codepoint, (Vector2){position.x + textOffsetX, position.y + textOffsetY + addY}, size, tint);
			}

			if (chosenFont->glyphs[index].advanceX == 0)
				textOffsetX += ((float)chosenFont->recs[index].width * scaleFactor + spacing);
			else
				textOffsetX += ((float)chosenFont->glyphs[index].advanceX * scaleFactor + spacing);
		}

		i += codepointByteCount; // Move text bytes counter to next codepoint
	}
}

void drawDeltatuneFont(const std::string &text, Vector2 position, Color tint)
{
	DrawTextExWithFallback(tuneFont, tuneFallbackFont, text.c_str(), position, tuneFont.baseSize * dconf->appearance.text.textScale, 0, tint);
}

inline constexpr float BOLD_ANGLE_DIVISOR = 8;

// Very creative function name
void DrawTextExOutlineKinda(Font font, Font fallbackFont, const char *text, Vector2 position, float fontSize, float spacing, float thickness, Color tint)
{
	if (thickness <= 0)
		return;

	float ang = 0;
	Vector2 originalPosition = position;
	for (ang = 0; ang < PI * 2; ang += (PI / BOLD_ANGLE_DIVISOR))
	{
		position.x = originalPosition.x + (cos(ang) * thickness);
		position.y = originalPosition.y + (sin(ang) * thickness);
		DrawTextExWithFallback(font, fallbackFont, text, position, fontSize, spacing, tint);
	}
}

void drawTruetypeFont(
	const Font &font,
	const std::string &text,
	Vector2 position,
	int fontSize,
	Color tint,
	float shadow,
	Color shadowTint,
	float outline,
	Color outlineTint)
{
	const char *ctext = text.c_str();
	if (shadow > 0)
	{
		Vector2 shadowPos = {
			position.x + shadow,
			position.y + shadow,
		};
		DrawTextExOutlineKinda(font, font, ctext, shadowPos, fontSize, 0, outline, shadowTint);
		DrawTextEx(font, ctext, position, fontSize, 0, shadowTint);
	}
	if (outline > 0)
		DrawTextExOutlineKinda(font, font, ctext, position, fontSize, 0, outline, outlineTint);

	DrawTextEx(font, ctext, position, fontSize, 0, tint);
}

void deinit()
{
	if (IsFontValid(tuneFont))
		UnloadFont(tuneFont);
	if (IsFontValid(tuneFallbackFont))
		UnloadFont(tuneFallbackFont);
}

} // namespace font
