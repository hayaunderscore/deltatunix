#include "raylib.h"
#include "raymath.h"
// #include "reasings.h"
#include "mpris.hpp"
#include "window.hpp"
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#define INI_IMPLEMENTATION
#include "ini.h"

inline constexpr const char *HOME_RESOURCE_PATH = "/.config/deltatunix/";
inline constexpr const char *GLOBAL_RESOURCE_PATH = "/usr/share/deltatunix/";

// Helper to get resources
std::string GetResourcePath(const std::string &path)
{
    // CHECK 1: EXECUTABLE DIRECTORY
    if (FileExists((GetApplicationDirectory() + path).c_str()))
        return GetApplicationDirectory() + path;
#ifdef __unix__
    // CHECK 2: $HOME/.config/deltatunix/
    const char *home = getenv("HOME");
    std::string expanded = (home + std::string(HOME_RESOURCE_PATH) + path);
    if (FileExists(expanded.c_str()))
        return expanded.c_str();
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

// TODO: Probably not make these constant??? Let the user change them I suppose
inline constexpr float APPEAR_DELAY_LENGTH = 0.5f;
inline constexpr float APPEAR_ANIMATION_LENGTH = 0.75f;
inline constexpr float DISAPPEAR_ANIMATION_LENGTH = 0.75f;
inline constexpr float STAY_TIME = 2.5f;
inline constexpr float SLIDE_IN_DISTANCE = 24;
inline constexpr float SLIDE_OUT_DISTANCE = 24;
inline constexpr const char *CONFIG_FILE = "config.ini";

struct Config
{
    float textScale;
    float padding;
    bool disappear;
    bool changeAnimation;
    int monitor;
};

Config config = {
    .textScale = 2.0f,
    .padding = 24.0f,
    .disappear = false,
    .changeAnimation = true,
    .monitor = 0,
};

TextState currentState = STATE_HIDDEN;
TextState lastState = STATE_HIDDEN;
double animationTimer = 0;
double opacity = 0;
Vector2 positionOffset = {0, 0};

Font tuneFont;
Font tuneFallbackFont; // For JP glyphs

std::string currentText;
bool queuedText;

// Edit of MeasureTextEx for fallback fonts
Vector2 MeasureTextExWithFallback(Font font, Font fallbackFont, const char *text, float fontSize, float spacing)
{
    Vector2 textSize = {0};

    if ((font.texture.id == 0) || (text == NULL) || (text[0] == '\0'))
        return textSize; // Security check

    int size = TextLength(text); // Get size in bytes of text
    int tempByteCounter = 0;     // Used to count longer text line num chars
    int byteCounter = 0;

    float textWidth = 0.0f;
    float tempTextWidth = 0.0f; // Used to count longer text line width

    float textHeight = fontSize;
    float scaleFactor = fontSize / (float)font.baseSize;

    int letter = 0; // Current character
    int index = 0;  // Index position in sprite font

    for (int i = 0; i < size;)
    {
        byteCounter++;

        int codepointByteCount = 0;
        letter = GetCodepointNext(&text[i], &codepointByteCount);
        index = GetGlyphIndex(font, letter);
        Font *chosenFont = &font;

        if (font.glyphs[index].value != letter)
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

    float textOffsetY = 0;    // Offset between lines (on linebreak '\n')
    float textOffsetX = 0.0f; // Offset X to next character to draw

    float scaleFactor = fontSize / font.baseSize; // Character quad scaling factor

    for (int i = 0; i < size;)
    {
        // Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);
        Font *chosenFont = &font;

        if (font.glyphs[index].value != codepoint)
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
                float size = (chosenFont == &font) ? fontSize : 12 * config.textScale;
                float addY = (chosenFont == &fallbackFont) ? (3 * config.textScale) : 0;
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

RenderTexture2D textTexture;

void GenerateTuneText()
{
    if (currentText.empty())
        return;

    // Get the size of the text to draw
    Vector2 textSize = MeasureTextExWithFallback(tuneFont, tuneFallbackFont, currentText.c_str(), tuneFont.baseSize * config.textScale, 0);

    BeginTextureMode(textTexture);

    ClearBackground(BLANK);

    // Calculate the top-left text position based on the rectangle and
    // alignment
    Rectangle rect = {config.padding, config.padding, (float)GetScreenWidth() - (config.padding * 2), (float)GetScreenHeight() - (config.padding * 2)};
    Vector2 textPos = (Vector2){rect.x + Lerp(0.0f, rect.width - textSize.x, 1.0f), rect.y + Lerp(0.0f, rect.height - textSize.y, 0.5f)};

    DrawTextExWithFallback(tuneFont, tuneFallbackFont, currentText.c_str(), textPos, tuneFont.baseSize * config.textScale, 0, WHITE);

    EndTextureMode();
}

void DrawTuneText()
{
    if (currentState == STATE_HIDDEN)
        return;

    if (currentText.empty())
        return;

    Color alph = WHITE;
    alph.a = opacity * 255;

    Rectangle srcrec = {0, 0, (float)textTexture.texture.width, (float)-textTexture.texture.height};
    Rectangle dstrec = {positionOffset.x, positionOffset.y, (float)textTexture.texture.width, (float)textTexture.texture.height};
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
        }

        if (animationTimer >= APPEAR_DELAY_LENGTH)
            currentState = STATE_APPEARING;
        break;

    case STATE_APPEARING:
        if (animationTimer == 0)
        {
            opacity = 0;
            positionOffset.x = 0;
            if (config.changeAnimation)
            {
                currentText = mpris::buildDisplayedText();
                GenerateTuneText();
            }
            queuedText = false;
        }

        progress = (animationTimer / APPEAR_ANIMATION_LENGTH);

        opacity = Clamp(progress * 1.5f - 0.25f, 0, 1);
        positionOffset.x = InterpolateQuadratic(SLIDE_IN_DISTANCE * config.textScale, 0, progress);

        if (animationTimer >= APPEAR_ANIMATION_LENGTH)
            currentState = STATE_VISIBLE;
        break;

    case STATE_VISIBLE:
        if (animationTimer == 0)
        {
            opacity = 1;
            positionOffset.x = 0;
        }

        if (animationTimer >= STAY_TIME && config.disappear)
            currentState = STATE_DISAPPEARING;
        break;

    case STATE_DISAPPEARING:
        if (animationTimer == 0)
        {
            opacity = 0;
            positionOffset.x = 0;
        }

        progress = (animationTimer / DISAPPEAR_ANIMATION_LENGTH);

        opacity = 1.0 - Clamp(progress * 1.5f - 0.25f, 0, 1);
        positionOffset.x = InterpolateQuadratic(SLIDE_OUT_DISTANCE * config.textScale, 0, 1 - progress);

        if (animationTimer >= DISAPPEAR_ANIMATION_LENGTH)
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

const char *get_ini_property(const ini_t *ini, const char *property, int section = INI_GLOBAL_SECTION, const char *defaultValue = "")
{
    int idx = ini_find_property(ini, section, property, strlen(property));
    if (idx == INI_NOT_FOUND)
        return defaultValue;
    return ini_property_value(ini, section, idx);
}

void LoadConfig()
{
    char *file = LoadFileText(GetResourcePath(CONFIG_FILE).c_str());
    if (file == NULL)
    {
        TraceLog(LOG_ERROR, "CONFIG: Could not parse config!");
        return;
    }

    ini_t *ini = ini_load(file, NULL);
    int general_section = ini_find_section(ini, "General", 0);
    if (general_section == INI_NOT_FOUND)
    {
        TraceLog(LOG_ERROR, "CONFIG: Could not find General section in config!");
        goto free_all;
    }

    // now set le config
    config.textScale = std::stod(get_ini_property(ini, "TextScale", general_section, "2.0"));
    config.padding = std::stod(get_ini_property(ini, "Padding", general_section, "24"));
    bool b;
    std::istringstream(get_ini_property(ini, "Disappear", general_section, "False")) >> b;
    config.disappear = b;
    std::istringstream(get_ini_property(ini, "ChangeAnimation", general_section, "True")) >> b;
    config.changeAnimation = b;
    config.monitor = std::stoi(get_ini_property(ini, "Monitor", general_section, "0"));

    TraceLog(LOG_INFO, TextFormat("CONFIG: Text Scale: 			%.f", config.textScale));
    TraceLog(LOG_INFO, TextFormat("CONFIG: Padding: 			%.f", config.padding));
    TraceLog(LOG_INFO, TextFormat("CONFIG: Disappear: 			%s", config.disappear ? "TRUE" : "FALSE"));
    TraceLog(LOG_INFO, TextFormat("CONFIG: Change Animation: 	%s", config.changeAnimation ? "TRUE" : "FALSE"));
    TraceLog(LOG_INFO, TextFormat("CONFIG: Monitor: 			%i", config.monitor));

free_all:
    ini_destroy(ini);
    UnloadFileText(file);
}

void InitTextTexture()
{
    // Move to the top of the current monitor
    int monitor = config.monitor;
    int monitorCount = 0;
    glfwGetMonitors(&monitorCount);
    // Make sure its a valid monitor!
    monitor = (int)Clamp(monitor, 0, monitorCount);

    Vector2 monitorPos = GetMonitorPosition(monitor);
    SetWindowPosition(monitorPos.x, monitorPos.y);
    SetWindowSize(GetMonitorWidth(monitor), (config.padding * 2) + (tuneFont.baseSize * config.textScale));

    // Create render texture
    if (IsRenderTextureValid(textTexture))
    {
        UnloadRenderTexture(textTexture);
    }

    textTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    GenerateTuneText();
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_MOUSE_PASSTHROUGH | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_TOPMOST);
    InitWindow(1, 1, "");
    SetTargetFPS(60);

    LoadConfig();

    mpris::start();
    // Add a callback
    mpris::metadataCallback = [&]()
    {
        if (!config.changeAnimation)
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
        if (currentState != STATE_HIDDEN && !config.disappear)
        {
            currentState = STATE_DISAPPEARING;
            animationTimer = 0;
        }
    };
    mpris::pausedCallback = [&](bool paused)
    {
        if (config.changeAnimation || currentState == STATE_VISIBLE || currentState == STATE_APPEARING)
        {
            currentText = mpris::buildDisplayedText();
            GenerateTuneText();
        }
    };

    // Fonter
    tuneFont = LoadFont(GetResourcePath("resources/fonts/MusicTitleFont.fnt").c_str());
    tuneFallbackFont = LoadFont(GetResourcePath("resources/fonts/ShinonomeGothic.fnt").c_str()); // TODO only load glyphs when needed????

    InitTextTexture();

    // Unfocus window, if possible
    WindowUnfocus();
    // Also hide from taskbar, if possible
    WindowHideFromTaskbar();

    while (!WindowShouldClose())
    {
        mpris::updateConnection();

        BeginDrawing();

        ClearBackground(BLANK);

        // DrawRectangleRec({0, 0, (float)GetScreenWidth(),
        // (float)GetScreenHeight()}, {0, 0, 0, 128});
        UpdateTuneState();
        DrawTuneText();

        EndDrawing();
    }

    mpris::leave();

    UnloadFont(tuneFont);
    UnloadFont(tuneFallbackFont);

    CloseWindow();

    return 0;
}
