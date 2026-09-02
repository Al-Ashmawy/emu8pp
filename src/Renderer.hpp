#pragma once

#include <cstdint>
#include <raylib.h>
#include <string>
#include <vector>

using namespace std;

class Renderer {
public:
    Renderer(int width, int height, int scale, const string& title, int fps);

    ~Renderer();

    void load_sound(const string& file);

    void play_sound();

    bool elapsed_60hz();

    bool is_key_down(KeyboardKey key);

    bool should_close();

    void draw_display(const vector<vector<uint8_t>>& display);

private:
    Sound sound { };
    double tick { 0.0 };
    int scale { };
};
