#include "font.hpp"
#include <cmath>
#include <cstddef>
#include <fontconfig/fontconfig.h>
#include <freetype/ftglyph.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include <raylib.h>
#include <stdlib.h>
#include <vector>

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

FT_Library ftLibrary;								 // Freetype library object
std::unordered_map<unsigned int, Font> outlineFonts; // Fonts, but its an outline instead

void init(config::Config *conf)
{
	int error = FT_Init_FreeType(&ftLibrary);
	if (error)
		TraceLog(LOG_WARNING, TextFormat("FONT: Failed to initialize Freetype! (Error code %d)", error));

	dconf = conf;
	// We init Fontconfig here...
	fconf = FcInitLoadConfigAndFonts();
	// Also init bitmap deltarune tune fonts
	tuneFont = LoadFont(GetResourcePath("resources/fonts/MusicTitleFont.fnt").c_str());
	tuneFallbackFont = LoadFont(GetResourcePath("resources/fonts/ShinonomeGothic.fnt").c_str());
}

// This only assumes the bitmap pixel mode is gray
Image FT_BitmapToImage(const FT_Bitmap *bmp)
{
	int width = bmp->width;
	int height = bmp->rows;

	if (width == 0 || height == 0)
	{
		TraceLog(LOG_WARNING, TextFormat("FONT: Font bitmap width or height is 0!"));
		return GenImageColor(1, 1, BLANK);
	}

	unsigned char *pixels = (unsigned char *)malloc(width * height);

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int index = y * width + x;
			unsigned char value = bmp->buffer[y * bmp->pitch + x];
			// pixels[(y * width + x) * 2 + 0] = 255;
			pixels[index] = value;
		}
	}

	Image img = {
		.data = pixels,
		.width = width,
		.height = height,
		.mipmaps = 1,
		.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE};

	return img;
}

Font LoadFreetypeFont(std::string font, int fontSize, int *codepoints, int codepointCount, float outline)
{
	if (TextIsEqual(GetFileExtension(font.c_str()), ".fnt"))
		return LoadFont(font.c_str());

	if (ftLibrary == NULL)
		return LoadFontEx(font.c_str(), fontSize, codepoints, codepointCount);

	FT_Face face;
	FT_Stroker stroker;
	int error;

	// Init stroker if we're doing an outline
	if (outline > 0)
	{
		FT_Stroker_New(ftLibrary, &stroker);
		FT_Stroker_Set(
			stroker,
			(int)(outline * 64),
			FT_STROKER_LINECAP_ROUND,
			FT_STROKER_LINEJOIN_ROUND,
			0);
	}

	error = FT_New_Face(ftLibrary, font.c_str(), 0, &face);
	if (error)
	{
		TraceLog(LOG_WARNING, TextFormat("FONT: Failed to load freetype font! [%s] (Error code %d)", font.c_str(), error));
		return LoadFontEx(DEFAULT_FONT, fontSize, codepoints, codepointCount);
	}

	FT_Set_Pixel_Sizes(face, 0, fontSize);

	Font finalFont{};
	finalFont.baseSize = fontSize;
	finalFont.glyphCount = 0;
	finalFont.glyphPadding = 0;

	std::vector<GlyphInfo> glyphs;

	int *codepoint = codepoints;
	int ascender = face->size->metrics.ascender >> 6;

	for (int i = 0; i < codepointCount; i++)
	{
		int glyph_index = FT_Get_Char_Index(face, *codepoint);
		FT_Bitmap bitmap;
		GlyphInfo glyphInfo;
		FT_Glyph strokeGlyph;

		// TraceLog(LOG_INFO, TextFormat("FONT: Loading index %d from codepoint %d", glyph_index, *codepoint));

		if (glyph_index == 0)
			goto skip_loop;

		error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP);
		if (error)
		{
			TraceLog(LOG_WARNING, TextFormat("FONT: Failed to load glyph! [%s] (Error code %d)", font.c_str(), error));
			goto skip_loop;
		}

		if (outline <= 0)
		{
			error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
			if (error)
			{
				TraceLog(LOG_WARNING, TextFormat("FONT: Failed to render glyph! [%s] (Error code %d)", font.c_str(), error));
				goto skip_loop;
			}

			if (face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
			{
				TraceLog(LOG_WARNING, TextFormat("FONT: Font bitmap is not a grayscale bitmap! [%s]", font.c_str()));
				goto skip_loop;
			}

			bitmap = face->glyph->bitmap;
			glyphInfo.offsetX = face->glyph->bitmap_left;
			glyphInfo.offsetY = ascender - face->glyph->bitmap_top;
		}
		else
		{
			error = FT_Get_Glyph(face->glyph, &strokeGlyph);
			if (error)
			{
				TraceLog(LOG_WARNING, TextFormat("FONT: Failed to get glyph! [%s] (Error code %d)", font.c_str(), error));
				goto skip_loop;
			}

			error = FT_Glyph_StrokeBorder(&strokeGlyph, stroker, 0, 1);
			if (error)
			{
				TraceLog(LOG_WARNING, TextFormat("FONT: Failed to stroke glyph! [%s] (Error code %d)", font.c_str(), error));
				goto skip_loop;
			}

			error = FT_Glyph_To_Bitmap(&strokeGlyph, FT_RENDER_MODE_NORMAL, NULL, 1);
			if (error)
			{
				TraceLog(LOG_WARNING, TextFormat("FONT: Failed to render glyph! [%s] (Error code %d)", font.c_str(), error));
				goto skip_loop;
			}

			FT_BitmapGlyph bitmapGlyph = (FT_BitmapGlyph)strokeGlyph;
			bitmap = bitmapGlyph->bitmap;
			glyphInfo.offsetX = bitmapGlyph->left;
			glyphInfo.offsetY = ascender - bitmapGlyph->top;

			if (strokeGlyph)
				FT_Done_Glyph(strokeGlyph);
		}

		glyphInfo.advanceX = face->glyph->advance.x >> 6;
		glyphInfo.value = *codepoint;
		glyphInfo.image = FT_BitmapToImage(&bitmap);

		glyphs.push_back(glyphInfo);
		finalFont.glyphCount++;

	skip_loop:
		codepoint++;
	}

	finalFont.glyphs = (GlyphInfo *)malloc(sizeof(GlyphInfo) * finalFont.glyphCount);
	memcpy(finalFont.glyphs, glyphs.data(), sizeof(GlyphInfo) * finalFont.glyphCount);

	Image atlas = GenImageFontAtlas(finalFont.glyphs, &finalFont.recs, finalFont.glyphCount, finalFont.baseSize, finalFont.glyphPadding + (int)(floorf(outline)), 0);
	finalFont.texture = LoadTextureFromImage(atlas);
	SetTextureFilter(finalFont.texture, TEXTURE_FILTER_BILINEAR);

	for (int i = 0; i < finalFont.glyphCount; i++)
	{
		GlyphInfo *glyph = &finalFont.glyphs[i];

		if (IsImageValid(glyph->image))
		{
			UnloadImage(glyph->image);
			glyph->image = {0};
		}

		Rectangle *rec = &finalFont.recs[i];
		if (rec->width <= 0 || rec->height <= 0 || rec->x < 0 || rec->y < 0)
			continue;

		Image newImage = ImageFromImage(atlas, *rec);
		if (!IsImageValid(newImage))
			continue;

		glyph->image = newImage;
	}

	UnloadImage(atlas);
	FT_Done_Face(face);
	if (outline > 0)
		FT_Stroker_Done(stroker);

	TraceLog(LOG_INFO, "FONT: Data loaded successfully (%i pixel size | %i glyphs)", finalFont.baseSize, finalFont.glyphCount);

	return finalFont;
}

Font generateFontByFontName(std::string font, int fontSize, const int *codepoints, int codepointCount, float outlineSize)
{
	Font freetypeFont;
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
				TraceLog(LOG_INFO, TextFormat("FONT: Found path for matched font %s: %s", font.c_str(), path.c_str()));

				FcPatternDestroy(fon);
			}
			FcFontSetDestroy(fs);
		}

		if (os)
			FcObjectSetDestroy(os);
	}

	freetypeFont = LoadFreetypeFont(path, fontSize, (int *)codepoints, codepointCount, 0);
	if (IsFontValid(freetypeFont))
	{
		unsigned int id = freetypeFont.texture.id;
		outlineFonts[id] = LoadFreetypeFont(path, fontSize, (int *)codepoints, codepointCount, outlineSize);
	}
	return freetypeFont;

return_fallback_font:
	return LoadFontEx(DEFAULT_FONT, fontSize, codepoints, codepointCount);
}

void unloadFont(Font font)
{
	if (outlineFonts.count(font.texture.id))
	{
		UnloadFont(outlineFonts[font.texture.id]);
		outlineFonts.erase(font.texture.id);
	}
	UnloadFont(font);
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
		if (IsFontValid(font) && outlineFonts.count(font.texture.id))
			DrawTextEx(outlineFonts[font.texture.id], ctext, shadowPos, fontSize, 0, shadowTint);
		DrawTextEx(font, ctext, shadowPos, fontSize, 0, shadowTint);
	}
	if (IsFontValid(font) && outlineFonts.count(font.texture.id))
		DrawTextEx(outlineFonts[font.texture.id], ctext, position, fontSize, 0, outlineTint);

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
