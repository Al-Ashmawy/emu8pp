#include "Renderer.hpp"
#include <cstdint>
#include <raylib.h>
#include <string>
#include <vector>

using namespace std;

Renderer::Renderer(
    int width, int height, int scale, const string& title, int fps)
{
    this->scale = scale;
    SetTraceLogLevel(LOG_NONE);
    InitWindow(width, height, title.c_str());
    InitAudioDevice();
    SetTargetFPS(fps);
}

Renderer::~Renderer()
{
    if (IsSoundValid(sound)) {
        UnloadSound(sound);
    }
    CloseAudioDevice();
    CloseWindow();
}

void Renderer::load_sound(const string& file)
{
    sound = LoadSound(file.c_str());
}

void Renderer::play_sound()
{
    if (IsSoundValid(sound)) {
        PlaySound(sound);
    }
}

bool Renderer::elapsed_60hz()
{
    tick += GetFrameTime();
    if (tick >= 1.0 / 60) {
        tick = 0;
        return true;
    }
    return false;
}

bool Renderer::is_key_down(KeyboardKey key)
{
    return IsKeyDown(key);
}

bool Renderer::should_close()
{
    return WindowShouldClose();
}

void Renderer::draw_display(const vector<vector<uint8_t>>& display)
{
    BeginDrawing();
    ClearBackground(WHITE);
    for (size_t i = 0; i < display.size(); i++) {
        for (size_t j = 0; j < display[0].size(); j++) {
            Color color = display[i][j] ? BLACK : WHITE;
            DrawRectangle(scale * j, scale * i, scale, scale, color);
        }
    }
    EndDrawing();
}
