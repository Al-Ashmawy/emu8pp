#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <print>
#include <random>
#include <raylib.h>
#include <vector>

using namespace std;

constexpr int MEMORY_SIZE = 4 * 1024;
constexpr int FONT_START = 0x50;
constexpr int EXECUTION_START = 0x200;
constexpr int SCREEN_WIDTH = 64;
constexpr int SCREEN_HEIGHT = 32;

const vector<vector<uint8_t>> font_sprites = { { 0xF0, 0x90, 0x90, 0x90, 0xF0 },
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

class chip8 {
public:
    vector<vector<uint8_t>> display { vector(
        SCREEN_HEIGHT, vector<uint8_t>(SCREEN_WIDTH)) };
    vector<bool> keyboard { vector<bool>(16) };
    uint8_t delay { 0 };
    uint8_t sound { 0 };

    chip8(const string& rom)
    {
        load_rom(rom);
        write_sprites();
        init_table();
    }

    void cycle()
    {
        fetch();

        decode_execute();
    }

private:
    vector<uint8_t> memory { vector<uint8_t>(MEMORY_SIZE) };
    vector<uint8_t> registers { vector<uint8_t>(16) };
    vector<uint16_t> stack { vector<uint16_t>(16) };
    uint16_t opcode { };
    uint16_t index { 0 };
    uint16_t pc { EXECUTION_START };
    int8_t sp { -1 };
    mt19937 random_byte { mt19937(
        chrono::system_clock::now().time_since_epoch().count()) };
    map<uint8_t, function<void(void)>> table;
    map<uint8_t, function<void(void)>> table_0;
    map<uint8_t, function<void(void)>> table_8;
    map<uint8_t, function<void(void)>> table_e;
    map<uint8_t, function<void(void)>> table_f;

    void init_table()
    {
        table_0[0x0] = [this]() { op_00e0(); };
        table_0[0xe] = [this] { op_00ee(); };

        table[0x0] = [this]() { table_0[this->opcode & 0xf](); };
        table[0x1] = [this]() { op_1nnn(); };
        table[0x2] = [this]() { op_2nnn(); };
        table[0x3] = [this]() { op_3xkk(); };
        table[0x4] = [this]() { op_4xkk(); };
        table[0x5] = [this]() { op_5xy0(); };
        table[0x6] = [this]() { op_6xkk(); };
        table[0x7] = [this]() { op_7xkk(); };
        table[0x8] = [this]() { table_8[this->opcode & 0xf](); };
        table[0x9] = [this]() { op_9xy0(); };
        table[0xa] = [this]() { op_annn(); };
        table[0xb] = [this]() { op_bnnn(); };
        table[0xc] = [this]() { op_cxkk(); };
        table[0xd] = [this]() { op_dxyn(); };
        table[0xe] = [this]() { table_e[this->opcode & 0xf](); };
        table[0xf] = [this]() { table_f[this->opcode & 0xff](); };

        table_8[0x0] = [this]() { op_8xy0(); };
        table_8[0x1] = [this]() { op_8xy1(); };
        table_8[0x2] = [this]() { op_8xy2(); };
        table_8[0x3] = [this]() { op_8xy3(); };
        table_8[0x4] = [this]() { op_8xy4(); };
        table_8[0x5] = [this]() { op_8xy5(); };
        table_8[0x6] = [this]() { op_8xy6(); };
        table_8[0x7] = [this]() { op_8xy7(); };
        table_8[0xe] = [this]() { op_8xye(); };

        table_e[0xe] = [this]() { op_ex9e(); };
        table_e[0x1] = [this]() { op_exa1(); };

        table_f[0x07] = [this]() { op_fx07(); };
        table_f[0x0a] = [this]() { op_fx0a(); };
        table_f[0x15] = [this]() { op_fx15(); };
        table_f[0x18] = [this]() { op_fx18(); };
        table_f[0x1e] = [this]() { op_fx1e(); };
        table_f[0x29] = [this]() { op_fx29(); };
        table_f[0x33] = [this]() { op_fx33(); };
        table_f[0x55] = [this]() { op_fx55(); };
        table_f[0x65] = [this]() { op_fx65(); };
    }

    void load_rom(const string& rom)
    {
        ifstream file(rom, ios::binary);
        int code = errno;
        if (!file.is_open()) {
            println(cerr, "can't open file {}: {}", rom, strerror(code));
            return;
        }

        size_t size = filesystem::file_size(rom);
        file.read(reinterpret_cast<char*>(&memory[EXECUTION_START]), size);
    }

    void write_sprites()
    {
        size_t i = FONT_START;
        for (const auto& vec : font_sprites) {
            for (const char byte : vec) {
                memory[i] = byte;
                i++;
            }
        }
    }

    void fetch()
    {
        opcode = (memory[pc] << 8) | memory[pc + 1];
        pc += 2;
    }

    void decode_execute()
    {
        uint8_t nibble1 = opcode >> 12;

        table[nibble1]();
    }

    void op_00e0()
    {
        for (auto& vec : display) {
            for (auto& pixel : vec) {
                pixel = 0;
            }
        }
    }

    void op_00ee()
    {
        pc = stack[sp];
        sp--;
    }

    void op_1nnn()
    {
        uint16_t address = opcode & 0x0fff;
        pc = address;
    }

    void op_2nnn()
    {
        uint16_t address = opcode & 0x0fff;
        sp++;
        stack[sp] = pc;
        pc = address;
    }

    void op_3xkk()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint16_t k = opcode & 0x00ff;
        if (registers[x] == k) {
            pc += 2;
        }
    }

    void op_4xkk()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint16_t k = opcode & 0x00ff;
        if (registers[x] != k) {
            pc += 2;
        }
    }

    void op_5xy0()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint8_t y = (opcode & 0x00f0) >> 4;
        if (registers[x] == registers[y]) {
            pc += 2;
        }
    }

    void op_6xkk()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint16_t k = (opcode & 0x00ff);
        registers[x] = k;
    }

    void op_7xkk()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint16_t k = (opcode & 0x00ff);
        registers[x] = registers[x] + k;
    }

    void op_8xy0()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint8_t y = (opcode & 0x00f0) >> 4;
        registers[x] = registers[y];
    }

    void op_8xy1()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint8_t y = (opcode & 0x00f0) >> 4;
        registers[x] = registers[x] | registers[y];
    }

    void op_8xy2()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint8_t y = (opcode & 0x00f0) >> 4;
        registers[x] = registers[x] & registers[y];
    }

    void op_8xy3()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint8_t y = (opcode & 0x00f0) >> 4;
        registers[x] = registers[x] ^ registers[y];
    }

    void op_8xy4()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint8_t y = (opcode & 0x00f0) >> 4;
        uint16_t result = registers[x] + registers[y];
        if (result > 255) {
            registers[0xf] = 1;
        } else {
            registers[0xf] = 0;
        }
        registers[x] = result;
    }

    void op_8xy5()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint8_t y = (opcode & 0x00f0) >> 4;
        if (registers[x] > registers[y]) {
            registers[0xf] = 1;
        } else {
            registers[0xf] = 0;
        }
        registers[x] = registers[x] - registers[y];
    }

    void op_8xy6()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        registers[0xf] = registers[x] & 1;
        registers[x] >>= 1;
    }

    void op_8xy7()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint8_t y = (opcode & 0x00f0) >> 4;
        if (registers[y] > registers[x]) {
            registers[0xf] = 1;
        } else {
            registers[0xf] = 0;
        }
        registers[x] = registers[y] - registers[x];
    }

    void op_8xye()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        registers[0xf] = (registers[x] >> 7);
        registers[x] <<= 1;
    }

    void op_9xy0()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint8_t y = (opcode & 0x00f0) >> 4;
        if (registers[x] != registers[y]) {
            pc += 2;
        }
    }

    void op_annn()
    {
        uint16_t address = opcode & 0x0fff;
        index = address;
    }

    void op_bnnn()
    {
        uint16_t address = opcode & 0x0fff;
        pc = address + registers[0];
    }

    void op_cxkk()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint16_t k = (opcode & 0x00ff);

        registers[x] = (random_byte() % 256) & k;
    }

    void op_dxyn()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint8_t y = (opcode & 0x00f0) >> 4;
        uint8_t vx = registers[x];
        uint8_t vy = registers[y];
        uint8_t n = opcode & 0x000f;

        bool collision = false;
        for (int i = 0; i < n; i++) {
            uint8_t sprite_byte = memory[index + i];

            for (int j = 0; j < 8; j++) {
                uint8_t& display_bit = display[(vy + i) % display.size()]
                                              [(vx + j) % display[0].size()];
                uint8_t sprite_bit = (sprite_byte >> (7 - j)) & 1;
                if (sprite_bit && display_bit) {
                    collision = true;
                }
                display_bit = display_bit ^ sprite_bit;
            }
        }
        registers[0xf] = collision;
    }

    void op_ex9e()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint8_t vx = registers[x];
        if (keyboard[vx]) {
            pc += 2;
        }
    }

    void op_exa1()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint8_t vx = registers[x];
        if (!keyboard[vx]) {
            pc += 2;
        }
    }

    void op_fx07()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        registers[x] = delay;
    }

    void op_fx0a()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        for (int i = 0; i <= 0xf; i++) {
            if (keyboard[i]) {
                registers[x] = i;
                return;
            }
        }

        pc -= 2;
    }

    void op_fx15()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        delay = registers[x];
    }

    void op_fx18()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        sound = registers[x];
    }

    void op_fx1e()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        index = index + registers[x];
    }

    void op_fx29()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        index = FONT_START + registers[x] * 5;
    }

    void op_fx33()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        uint8_t num = registers[x];
        for (int i = 2; i >= 0; i--) {
            memory[index + i] = num % 10;
            num /= 10;
        }
    }

    void op_fx55()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        for (size_t i = 0; i <= x; i++) {
            memory[index + i] = registers[i];
        }
    }

    void op_fx65()
    {
        uint8_t x = (opcode & 0x0f00) >> 8;
        for (size_t i = 0; i <= x; i++) {
            registers[i] = memory[index + i];
        }
    }
};

constexpr int SCALE = 20;
void DrawVideo(const vector<vector<uint8_t>>& display)
{
    for (size_t i = 0; i < display.size(); i++) {
        for (size_t j = 0; j < display[0].size(); j++) {
            Color color = display[i][j] ? BLACK : WHITE;
            DrawRectangle(SCALE * j, SCALE * i, SCALE, SCALE, color);
        }
    }
}

int main(int argc, char** argv)
{

    if (argc < 2) {
        println(cerr, "{} FLIE", argv[0]);
        return 1;
    }

    chip8 chip(argv[1]);

    InitWindow(1280, 640, "chip-8 emulator");
    SetTargetFPS(800);
    double tick = 0;

    while (!WindowShouldClose()) {
        tick += GetFrameTime();
        fill(chip.keyboard.begin(), chip.keyboard.end(), 0);
        if (IsKeyDown(KEY_ONE)) {
            chip.keyboard[1] = true;
        }
        if (IsKeyDown(KEY_TWO)) {
            chip.keyboard[2] = true;
        }
        if (IsKeyDown(KEY_THREE)) {
            chip.keyboard[3] = true;
        }
        if (IsKeyDown(KEY_FOUR)) {
            chip.keyboard[0xc] = true;
        }
        if (IsKeyDown(KEY_Q)) {
            chip.keyboard[4] = true;
        }
        if (IsKeyDown(KEY_W)) {
            chip.keyboard[5] = true;
        }
        if (IsKeyDown(KEY_E)) {
            chip.keyboard[6] = true;
        }
        if (IsKeyDown(KEY_R)) {
            chip.keyboard[0xd] = true;
        }
        if (IsKeyDown(KEY_A)) {
            chip.keyboard[7] = true;
        }
        if (IsKeyDown(KEY_S)) {
            chip.keyboard[8] = true;
        }
        if (IsKeyDown(KEY_D)) {
            chip.keyboard[9] = true;
        }
        if (IsKeyDown(KEY_F)) {
            chip.keyboard[0xe] = true;
        }
        if (IsKeyDown(KEY_Z)) {
            chip.keyboard[0xa] = true;
        }
        if (IsKeyDown(KEY_X)) {
            chip.keyboard[0] = true;
        }
        if (IsKeyDown(KEY_C)) {
            chip.keyboard[0xb] = true;
        }
        if (IsKeyDown(KEY_V)) {
            chip.keyboard[0xf] = true;
        }

        BeginDrawing();
        ClearBackground(WHITE);
        DrawVideo(chip.display);
        EndDrawing();

        if (tick >= 1.0 / 60) {
            tick = 0;
            if (chip.delay > 0) {
                chip.delay--;
            }
            if (chip.sound > 0) {
                chip.sound--;
            }
        }
        chip.cycle();
    }

    CloseWindow();
    return 0;
}
