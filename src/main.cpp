#include "Chip8.hpp"
#include "Renderer.hpp"
#include <iostream>
#include <print>
#include <raylib.h>
#include <vector>

using namespace std;

int main(int argc, char** argv)
{
    const map<KeyboardKey, int> keymap = { { KEY_ONE, 1 },
        { KEY_TWO, 2 },
        { KEY_THREE, 3 },
        { KEY_FOUR, 0xc },
        { KEY_Q, 4 },
        { KEY_W, 5 },
        { KEY_E, 6 },
        { KEY_R, 0xd },
        { KEY_A, 7 },
        { KEY_S, 8 },
        { KEY_D, 9 },
        { KEY_F, 0xe },
        { KEY_Z, 0xa },
        { KEY_X, 0 },
        { KEY_C, 0xb },
        { KEY_V, 0xf } };

    if (argc < 2) {
        println(cerr, "{} FILE", argv[0]);
        return 1;
    }

    Chip8 chip(argv[1]);
    if (!chip.is_rom_loaded()) {
        println(cerr, "{}", chip.error());
        return -1;
    }

    Renderer renderer(1280, 640, 1280 / SCREEN_WIDTH, "chip-8 emulator", 60);
    renderer.load_sound("8-bit-sound.mp3");

    bool halt = false;
    while (!renderer.should_close()) {
        chip.unset_keys();
        for (auto [key, hex] : keymap) {
            if (renderer.is_key_down(key)) {
                chip.set_key(hex);
            }
        }
        renderer.draw_display(chip.get_display());

        if (renderer.elapsed_60hz()) {
            chip.tick_timers();
        }

        for (int i = 0; i < 13; i++) {
            if (chip.is_playing_sound()) {
                renderer.play_sound();
            }

            chip.cycle();
            if ((halt = chip.is_halted())) {
                println(cerr, "{}", chip.error());
                break;
            }
        }

        if (halt) {
            break;
        }
    }

    return 0;
}
