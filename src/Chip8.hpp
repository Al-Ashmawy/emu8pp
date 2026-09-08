#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <random>
#include <string>
#include <vector>

constexpr int MEMORY_SIZE = 4 * 1024;
constexpr int FONT_START = 0x50;
constexpr int EXECUTION_START = 0x200;
constexpr int SCREEN_WIDTH = 64;
constexpr int SCREEN_HEIGHT = 32;

const std::vector<std::vector<uint8_t>> font_sprites
    = { { 0xF0, 0x90, 0x90, 0x90, 0xF0 },
          { 0x20, 0x60, 0x20, 0x20, 0x70 },
          { 0xF0, 0x10, 0xF0, 0x80, 0xF0 },
          { 0xF0, 0x10, 0xF0, 0x10, 0xF0 },
          { 0x90, 0x90, 0xF0, 0x10, 0x10 },
          { 0xF0, 0x80, 0xF0, 0x10, 0xF0 },
          { 0xF0, 0x80, 0xF0, 0x90, 0xF0 },
          { 0xF0, 0x10, 0x20, 0x40, 0x40 },
          { 0xF0, 0x90, 0xF0, 0x90, 0xF0 },
          { 0xF0, 0x90, 0xF0, 0x10, 0xF0 },
          { 0xF0, 0x90, 0xF0, 0x90, 0x90 },
          { 0xE0, 0x90, 0xE0, 0x90, 0xE0 },
          { 0xF0, 0x80, 0x80, 0x80, 0xF0 },
          { 0xE0, 0x90, 0x90, 0x90, 0xE0 },
          { 0xF0, 0x80, 0xF0, 0x80, 0xF0 },
          { 0xF0, 0x80, 0xF0, 0x80, 0x80 } };

class Chip8 {
public:
    Chip8(const std::string& rom);

    void cycle();

    const std::vector<std::vector<uint8_t>>& get_display() const;

    void set_key(int i);

    void unset_keys();

    void tick_timers();

    bool is_halted();

    bool is_rom_loaded();

    std::string error();

    bool is_playing_sound();

private:
    std::vector<uint8_t> memory { std::vector<uint8_t>(MEMORY_SIZE) };
    std::vector<uint8_t> registers { std::vector<uint8_t>(16) };
    std::vector<uint16_t> stack { std::vector<uint16_t>(16) };
    std::vector<bool> keyboard { std::vector<bool>(16) };
    std::vector<std::vector<uint8_t>> display { std::vector(
        SCREEN_HEIGHT, std::vector<uint8_t>(SCREEN_WIDTH)) };
    uint16_t opcode { };
    uint16_t index { 0 };
    uint16_t pc { EXECUTION_START };
    int8_t sp { -1 };
    uint8_t delay { 0 };
    uint8_t sound { 0 };
    std::mt19937 random_byte { std::mt19937(
        std::chrono::system_clock::now().time_since_epoch().count()) };
    std::map<uint16_t, std::function<void(void)>> table { };
    std::map<uint16_t, std::function<void(void)>> table_0 { };
    std::map<uint16_t, std::function<void(void)>> table_8 { };
    std::map<uint16_t, std::function<void(void)>> table_e { };
    std::map<uint16_t, std::function<void(void)>> table_f { };
    uint8_t x { }, y { }, n { }, kk { };
    uint16_t nnn { };
    bool halted { false };
    bool rom_loaded { true };
    std::string error_message { };

    void invalid_instruction();

    void init_table();

    void load_rom(const std::string& rom);

    void write_sprites();

    void fetch();

    void decode_execute();

    void op_00e0();
    void op_00ee();
    void op_1nnn();
    void op_2nnn();
    void op_3xkk();
    void op_4xkk();
    void op_5xy0();
    void op_6xkk();
    void op_7xkk();
    void op_8xy0();
    void op_8xy1();
    void op_8xy2();
    void op_8xy3();
    void op_8xy4();
    void op_8xy5();
    void op_8xy6();
    void op_8xy7();
    void op_8xye();
    void op_9xy0();
    void op_annn();
    void op_bnnn();
    void op_cxkk();
    void op_dxyn();
    void op_ex9e();
    void op_exa1();
    void op_fx07();
    void op_fx0a();
    void op_fx15();
    void op_fx18();
    void op_fx1e();
    void op_fx29();
    void op_fx33();
    void op_fx55();
    void op_fx65();
};
