#pragma once

#include <cstdint>
#include <raylib.h>
#include <string>
#include <vector>

class Renderer {
public:
    Renderer(int width, int height, int scale, const std::string& title, int fps);

    ~Renderer();

    void load_sound(const std::string& file);

    void play_sound();

    bool elapsed_60hz();

    bool is_key_down(KeyboardKey key);

    bool should_close();

    void draw_display(const std::vector<std::vector<uint8_t>>& display);

private:
    Sound sound { };
    double tick { 0.0 };
    int scale { };
};
