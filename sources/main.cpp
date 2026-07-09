#include "raylib.h"
#include "raymath.h"
// #include "reasings.h"
#include "config.hpp"
#include "font.hpp"
#include "mpris.hpp"
#include "tray.hpp"
#include "window.hpp"
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

inline constexpr const char *HOME_RESOURCE_PATH = "/.local/share/deltatunix/";
inline constexpr const char *GLOBAL_RESOURCE_PATH = "/usr/share/deltatunix/";

// Helper to get resources
std::string GetResourcePath(const std::string &path)
{
#ifdef __unix__
	// CHECK 1: $HOME/.local/share/deltatunix/
	const char *home = getenv("HOME");
	std::string expanded = (home + std::string(HOME_RESOURCE_PATH) + path);
	if (FileExists(expanded.c_str()))
		return expanded.c_str();
#endif
	// CHECK 2: EXECUTABLE DIRECTORY
	if (FileExists((GetApplicationDirectory() + path).c_str()))
		return GetApplicationDirectory() + path;
#ifdef __unix__
	// CHECK 3: /usr/share/deltatunix/
	return GLOBAL_RESOURCE_PATH + path;
#else
	return path;
#endif
}

enum TextState
{
	STATE_HIDDEN,
	STATE_APPEARING_DELAY,
	STATE_APPEARING,
	STATE_VISIBLE,
	STATE_DISAPPEARING,
};

inline constexpr const char *CONFIG_FILE = "config.kdl";

config::Config g_config;

TextState currentState = STATE_HIDDEN;
TextState lastState = STATE_HIDDEN;
double animationTimer = 0;
double opacity = 0;
Vector2 positionOffset = {0, 0};

Font kochiFont;

std::string currentText;
bool queuedText;

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

RenderTexture2D textTexture;
Rectangle textRect;
Vector2 textSize;

void GenerateTuneText()
{
	if (currentText.empty())
		return;

	Font *fon = &font::tuneFont;
	Font *backupFon = &font::tuneFallbackFont;
	int size = fon->baseSize;
	bool touhou = g_config.appearance.style == config::STYLE_TOUHOU;

	// Touhou fonts tend to use a Gothic font for this
	// Obviously bundling MS Gothic is not really allowed, so use IPA Gothic instead
	// (originally this said Kochi Gothic, but Kochi Gothic did not have the music symbol. Whoops.)
	if (touhou)
	{
		// Only load the glyphs necessary...
		if (IsFontValid(kochiFont))
		{
			font::unloadFont(kochiFont);
		}

		size = g_config.appearance.text.textSize;

		int codepointCount = 0;
		int *codepoints = LoadCodepoints(currentText.c_str(), &codepointCount);
		kochiFont = font::generateFontByFontName(
			g_config.appearance.text.font,
			size * g_config.appearance.text.textScale,
			codepoints,
			codepointCount,
			g_config.appearance.text.effects.outline);
		UnloadCodepoints(codepoints);

		fon = &kochiFont;
		backupFon = fon; // We don't really need a backup font...
	}

	textSize = MeasureTextExWithFallback(*fon, *backupFon, currentText.c_str(), size * g_config.appearance.text.textScale, 0);

	BeginTextureMode(textTexture);

	ClearBackground(BLANK);

	float align = 0.0f;
	switch (g_config.appearance.text.align)
	{
	case config::ALIGN_LEFT:
		break; // this is already the default (0.0)
	case config::ALIGN_CENTER:
		align = 0.5f;
		break;
	case config::ALIGN_RIGHT:
		align = 1.0f;
		break;
	}

	// Calculate the top-left text position based on the rectangle and
	// alignment
	const config::AppearancePadding &padding = g_config.appearance.text.padding;
	float paddingAdd = touhou ? 2 : 0; // Text rendered is emboldened in touhou style, also make room for the outline...
	Rectangle rect = {
		padding.left + paddingAdd,
		padding.top + paddingAdd,
		(float)GetScreenWidth() - ((padding.right + paddingAdd) * 2),
		(float)GetScreenHeight() - ((padding.bottom + paddingAdd) * 2)};
	Vector2 textPos = (Vector2){rect.x + Lerp(0.0f, rect.width - textSize.x, align), rect.y};

	textRect = (Rectangle){textPos.x, textPos.y, textSize.x, textSize.y};

	if (!touhou)
		font::drawDeltatuneFont(currentText, textPos, g_config.appearance.text.color);
	else
	{
		font::drawTruetypeFont(
			kochiFont,
			currentText,
			textPos,
			size,
			g_config.appearance.text.color,
			g_config.appearance.text.effects.shadow,
			g_config.appearance.text.effects.shadowColor,
			g_config.appearance.text.effects.outline,
			g_config.appearance.text.effects.outlineColor);
	}

	EndTextureMode();
}

double opacityMul = 1;

void DrawTuneText()
{
	if (currentState == STATE_HIDDEN)
		return;

	if (currentText.empty())
		return;

	Color alph = WHITE;
	alph.a = opacity * 255 * opacityMul;

	Rectangle srcrec = {0, 0, (float)textTexture.texture.width, (float)-textTexture.texture.height};
	Rectangle dstrec = {positionOffset.x + g_config.appearance.text.offset.x, positionOffset.y + g_config.appearance.text.offset.y, (float)textTexture.texture.width, (float)textTexture.texture.height};
	DrawTexturePro(textTexture.texture, srcrec, dstrec, {}, 0, alph);
}

inline static float InterpolateQuadratic(float a, float b, float t)
{
	float oneMinusT = 1 - t;
	float progress = 1 - oneMinusT * oneMinusT;

	return Lerp(a, b, progress);
}

void UpdateTuneState()
{
	float progress = 0;

	switch (currentState)
	{
	case STATE_APPEARING_DELAY:
		if (animationTimer == 0)
		{
			opacity = 0;
			positionOffset.x = 0;
			positionOffset.y = 0;
		}

		if (animationTimer >= g_config.appearance.timing.appearDelay)
			currentState = STATE_APPEARING;
		break;

	case STATE_APPEARING:
		if (animationTimer == 0)
		{
			opacity = 0;
			positionOffset.x = 0;
			positionOffset.y = 0;
			if (g_config.appearance.behavior.changeAnimation)
			{
				currentText = mpris::buildDisplayedText();
				GenerateTuneText();
			}
			queuedText = false;
		}

		progress = (animationTimer / g_config.appearance.timing.appearDuration);

		if (g_config.appearance.style == config::STYLE_TOUHOU)
		{
			opacity = 1.0;
			float dist = 0;
			switch (g_config.appearance.text.align)
			{
			case config::ALIGN_LEFT:
				dist = -textSize.x;
				break;
			case config::ALIGN_RIGHT:
				dist = textSize.x;
				break;
			default:
				opacity = Clamp(progress * 1.5f - 0.25f, 0, 1);
				break;
			}
			positionOffset.x = InterpolateQuadratic(dist * g_config.appearance.text.textScale, 0, progress);
		}
		else
		{
			opacity = Clamp(progress * 1.5f - 0.25f, 0, 1);
			positionOffset.x = InterpolateQuadratic(g_config.appearance.text.slideDistance * g_config.appearance.text.textScale, 0, progress);
		}

		if (animationTimer >= g_config.appearance.timing.appearDuration)
			currentState = STATE_VISIBLE;
		break;

	case STATE_VISIBLE:
		if (animationTimer == 0)
		{
			opacity = 1;
			positionOffset.x = 0;
			positionOffset.y = 0;
		}

		if (animationTimer >= g_config.appearance.timing.stayTime && g_config.appearance.behavior.disappear)
			currentState = STATE_DISAPPEARING;
		break;

	case STATE_DISAPPEARING:
		if (animationTimer == 0)
		{
			opacity = 0;
			positionOffset.x = 0;
			positionOffset.y = 0;
		}

		progress = (animationTimer / g_config.appearance.timing.disppearDuration);

		opacity = 1.0 - Clamp(progress * 1.5f - 0.25f, 0, 1);
		if (g_config.appearance.style == config::STYLE_TOUHOU)
		{
			float dist = 0;
			switch (g_config.appearance.text.valign)
			{
			case config::ALIGN_TOP:
				dist = -g_config.appearance.text.slideDistance;
				opacity = 1.0;
				break;
			case config::ALIGN_BOTTOM:
				dist = g_config.appearance.text.slideDistance;
				opacity = 1.0;
				break;
			default:
				break;
			}
			positionOffset.y = InterpolateQuadratic(dist * g_config.appearance.text.textScale, 0, 1 - progress);
		}
		else
			positionOffset.x = InterpolateQuadratic(-g_config.appearance.text.slideDistance * g_config.appearance.text.textScale, 0, 1 - progress);

		if (animationTimer >= g_config.appearance.timing.disppearDuration)
			currentState = queuedText ? STATE_APPEARING : STATE_HIDDEN;
		break;

	default:
		break;
	}

	animationTimer += GetFrameTime();

	if (lastState != currentState)
	{
		lastState = currentState;
		animationTimer = 0;
	}
}

void LoadConfig()
{
	// well then.
	g_config = config::LoadConfig(GetResourcePath(CONFIG_FILE));
}

void InitTextTexture()
{
	// Move to the top of the current monitor
	int monitor = g_config.general.monitor;
	int monitorCount = 0;
	glfwGetMonitors(&monitorCount);
	// Make sure its a valid monitor!
	monitor = (int)Clamp(monitor, 0, monitorCount);

	float size = g_config.appearance.style == config::STYLE_TOUHOU ? g_config.appearance.text.textSize : font::tuneFont.baseSize;
	float paddingAdd = g_config.appearance.style == config::STYLE_TOUHOU ? 2 : 0; // Text rendered is emboldened in touhou style, also make room for the outline...

	Vector2 monitorPos = GetDeterministicMonitorPosition(monitor, !g_config.general.useWorkArea);
	int monitorWidth = GetDeterministicMonitorWidth(monitor, !g_config.general.useWorkArea);
	int monitorHeight = GetDeterministicMonitorHeight(monitor, !g_config.general.useWorkArea);
	int textHeight = (g_config.appearance.text.padding.top + g_config.appearance.text.padding.bottom + (paddingAdd * 2)) + (size * g_config.appearance.text.textScale);

	float valign = 0.0f;
	switch (g_config.appearance.text.valign)
	{
	case config::ALIGN_TOP:
		break; // this is already the default (0.0)
	case config::ALIGN_VCENTER:
		valign = 0.5f;
		break;
	case config::ALIGN_BOTTOM:
		valign = 1.0f;
		break;
	}

	SetWindowPosition(monitorPos.x, Lerp(monitorPos.y, monitorHeight - textHeight, valign));
	SetWindowSize(monitorWidth, textHeight);

	// Create render texture
	if (IsRenderTextureValid(textTexture))
	{
		UnloadRenderTexture(textTexture);
	}

	textTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
	GenerateTuneText();
}

int main(int argc, char **argv)
{
#ifdef BUILD_SYSTRAY
	QApplication app(argc, argv); // This would go out of scope in tray::init, so just put it here...
#endif

	tray::init(argc, argv);
	tray::addAction(
		"Force Hide",
		[&]()
		{
			currentState = STATE_DISAPPEARING;
			animationTimer = 0;
		},
		true);
	tray::addAction(
		"Reload Config",
		[&]()
		{
			LoadConfig();
			InitTextTexture();
			currentText = mpris::buildDisplayedText();
			GenerateTuneText();
		},
		true);

	SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_MOUSE_PASSTHROUGH | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_TOPMOST);
	InitWindow(1, 1, "deltatunix");
	SetTargetFPS(60);

	LoadConfig();

	mpris::start();
	// Add a callback
	mpris::metadataCallback = [&]()
	{
		if (!g_config.appearance.behavior.changeAnimation)
		{
			currentText = mpris::buildDisplayedText();
			GenerateTuneText();
		}
		else
		{
			if (currentState == STATE_APPEARING || currentState == STATE_VISIBLE)
			{
				queuedText = true;
				currentState = STATE_DISAPPEARING;
				animationTimer = 0;
				return;
			}
		}

		if (currentState == STATE_HIDDEN || currentState == STATE_DISAPPEARING)
		{
			currentState = STATE_APPEARING_DELAY;
			animationTimer = 0;
		}
		else
		{
			animationTimer = 0;
		}
	};
	mpris::stoppedCallback = [&]()
	{
		if (currentState != STATE_HIDDEN && !g_config.appearance.behavior.disappear)
		{
			currentState = STATE_DISAPPEARING;
			animationTimer = 0;
		}
	};
	mpris::pausedCallback = [&](bool paused)
	{
		if (g_config.appearance.behavior.changeAnimation || currentState == STATE_VISIBLE || currentState == STATE_APPEARING)
		{
			currentText = mpris::buildDisplayedText();
			GenerateTuneText();
		}
	};

	font::init(&g_config);

	InitTextTexture();

	// Unfocus window, if possible
	WindowUnfocus();
	// Also hide from taskbar, if possible
	WindowHideFromTaskbar();

	while (!WindowShouldClose() && !tray::quitRequested)
	{
		mpris::updateConnection();
		tray::processEvents();

		// TODO: Hover opacity....
		// Rectangle collisionrec = {positionOffset.x + textRect.x, positionOffset.y + textRect.y, textRect.width, textRect.height};
		// TraceLog(LOG_INFO, TextFormat("MOUSE X: %.f | MOUSE Y: %.f", GetMouseX(), GetMouseY()));
		// TraceLog(LOG_INFO, TextFormat("RECT X: %.f | RECT Y: %.f | RECT S: (%.f, %.f)", collisionrec.x, collisionrec.y, collisionrec.width, collisionrec.height));
		// if (CheckCollisionPointRec(GetMousePosition(), collisionrec))
		// {
		//     opacityMul = Lerp(opacityMul, 0.5, GetFrameTime() * 4);
		// }
		// else
		//     opacityMul = Lerp(opacityMul, 1.0, GetFrameTime() * 4);

		BeginDrawing();

		ClearBackground(BLANK);

		// DrawRectangleRec({0, 0, (float)GetScreenWidth(),
		// (float)GetScreenHeight()}, {0, 0, 0, 128});
		UpdateTuneState();
		DrawTuneText();

		EndDrawing();
	}

	mpris::leave();

	font::deinit();

	if (IsFontValid(kochiFont))
		UnloadFont(kochiFont);

	CloseWindow();

#ifdef BUILD_SYSTRAY
	tray::quit();
	app.quit();
#endif

	return 0;
}
