#include "raylib.h"
#include "raymath.h"
// #include "reasings.h"
#include "mpris.hpp"
#include "window.hpp"
#include <GLFW/glfw3.h>
#include <sstream>
#include <string>
#include <vector>

#define INI_IMPLEMENTATION
#include "ini.h"

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
};

Config config = {
    .textScale = 2.0f,
    .padding = 24.0f,
    .disappear = true,
    .changeAnimation = true,
};

TextState currentState = STATE_HIDDEN;
TextState lastState = STATE_HIDDEN;
double animationTimer = 0;
double opacity = 0;
Vector2 positionOffset = {0, 0};
Font tuneFont;

std::string currentText;
bool queuedText;

void DrawTuneText()
{
    if (currentState == STATE_HIDDEN)
        return;

    if (currentText.empty())
        return;

    // Get the size of the text to draw
    Vector2 textSize = MeasureTextEx(tuneFont, currentText.c_str(), tuneFont.baseSize * config.textScale, 0);

    // Calculate the top-left text position based on the rectangle and
    // alignment
    Rectangle rect = {config.padding, config.padding, (float)GetScreenWidth() - (config.padding * 2), (float)GetScreenHeight() - (config.padding * 2)};
    Vector2 textPos = (Vector2){rect.x + Lerp(0.0f, rect.width - textSize.x, 1.0f) + positionOffset.x, rect.y + Lerp(0.0f, rect.height - textSize.y, 0.5f) + positionOffset.y};

    DrawTextEx(tuneFont, currentText.c_str(), textPos, tuneFont.baseSize * config.textScale, 0, (Color){255, 255, 255, (unsigned char)(opacity * 255)});
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
                currentText = mpris::buildDisplayedText();
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
    char *file = LoadFileText(CONFIG_FILE);
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
    std::istringstream(get_ini_property(ini, "Disappear", general_section, "True")) >> b;
    config.disappear = b;

    TraceLog(LOG_INFO, TextFormat("CONFIG: TextScale: 	%.f", config.textScale));
    TraceLog(LOG_INFO, TextFormat("CONFIG: Padding: 	%.f", config.padding));
    TraceLog(LOG_INFO, TextFormat("CONFIG: Disappear: 	%s", config.disappear ? "TRUE" : "FALSE"));

free_all:
    ini_destroy(ini);
    UnloadFileText(file);
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
            currentText = mpris::buildDisplayedText();
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
        }
    };

    // Fonter
    tuneFont = LoadFont("resources/fonts/MusicTitleFont.fnt");

    // Move to the top of the current monitor
    int monitor = GetCurrentMonitor();
    SetWindowPosition(0, 0);
    SetWindowSize(GetMonitorWidth(monitor), (config.padding * 2) + (tuneFont.baseSize * config.textScale));

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

    CloseWindow();

    return 0;
}
