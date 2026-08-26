#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
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

    chip8(const string& rom)
    {
        load_rom(rom);
        write_sprites();
        init_table();
    }

    void cycle()
    {
        println(log_file, "+=======================+");
        println(log_file, "pc: {:x}", pc);
        flush(log_file);

        fetch();

        println(log_file, "opcode {:04x}", opcode);
        flush(log_file);

        decode_execute();

        println(log_file, "registers:");
        flush(log_file);
        for (int i = 0; i < 16; i++) {
            println(log_file, "V{:x}: {:x}", i, registers[i]);
        }
        println(log_file, "I : {:x}", index);
        println(log_file);
        println(log_file, "display:");
        for (const auto& row : display) {
            for (const auto& col : row) {
                print(log_file, "{}", col);
            }
            println(log_file);
        }
        println(log_file);
        flush(log_file);
    }

private:
    ofstream log_file { "emu8.log" };
    vector<uint8_t> memory { vector<uint8_t>(MEMORY_SIZE) };
    vector<uint8_t> registers { vector<uint8_t>(16) };
    vector<uint16_t> stack { vector<uint16_t>(16) };
    uint16_t opcode { };
    uint16_t index { 0 };
    uint8_t delay { 0 };
    uint8_t sound { 0 };
    uint16_t pc { EXECUTION_START };
    int8_t sp { -1 };
    mt19937 random_byte { mt19937(
        chrono::system_clock::now().time_since_epoch().count()) };
    vector<function<void(void)>> table
        = vector<function<void(void)>>(OPCODE_INDEX::OP_FX65 + 1);

    struct OPCODE_INDEX {
        static constexpr int OP_EX9E = 0xe;
        static constexpr int OP_EXA1 = 0xf;

        static constexpr int OP_8XYN = 0x10;

        static constexpr int OP_00E0 = OP_8XYN + 0xe + 1;
        static constexpr int OP_00EE = OP_00E0 + 1;

        static constexpr int OP_FX07 = OP_00EE + 1;
        static constexpr int OP_FX0A = OP_FX07 + 1;
        static constexpr int OP_FX15 = OP_FX0A + 1;
        static constexpr int OP_FX18 = OP_FX15 + 1;
        static constexpr int OP_FX1E = OP_FX18 + 1;
        static constexpr int OP_FX29 = OP_FX1E + 1;
        static constexpr int OP_FX33 = OP_FX29 + 1;
        static constexpr int OP_FX65 = OP_FX33 + 1;
    };

    void init_table()
    {
        table[0x1] = [this]() { this->op_1nnn(); };
        table[0x2] = [this]() { this->op_2nnn(); };
        table[0x3] = [this]() { this->op_3xkk(); };
        table[0x4] = [this]() { this->op_4xkk(); };
        table[0x5] = [this]() { this->op_5xy0(); };
        table[0x6] = [this]() { this->op_6xkk(); };
        table[0x7] = [this]() { this->op_7xkk(); };
        table[0x9] = [this]() { this->op_9xy0(); };
        table[0xa] = [this]() { this->op_annn(); };
        table[0xb] = [this]() { this->op_bnnn(); };
        table[0xc] = [this]() { this->op_cxkk(); };
        table[0xd] = [this]() { this->op_dxyn(); };
        table[chip8::OPCODE_INDEX::OP_EX9E] = [this]() { this->op_ex9e(); };
        table[chip8::OPCODE_INDEX::OP_EXA1] = [this]() { this->op_exa1(); };
        table[chip8::OPCODE_INDEX::OP_8XYN + 0] = [this]() { this->op_8xy0(); };
        table[chip8::OPCODE_INDEX::OP_8XYN + 1] = [this]() { this->op_8xy1(); };
        table[chip8::OPCODE_INDEX::OP_8XYN + 2] = [this]() { this->op_8xy2(); };
        table[chip8::OPCODE_INDEX::OP_8XYN + 3] = [this]() { this->op_8xy3(); };
        table[chip8::OPCODE_INDEX::OP_8XYN + 4] = [this]() { this->op_8xy4(); };
        table[chip8::OPCODE_INDEX::OP_8XYN + 5] = [this]() { this->op_8xy5(); };
        table[chip8::OPCODE_INDEX::OP_8XYN + 6] = [this]() { this->op_8xy6(); };
        table[chip8::OPCODE_INDEX::OP_8XYN + 7] = [this]() { this->op_8xy7(); };
        table[chip8::OPCODE_INDEX::OP_8XYN + 0xe]
            = [this]() { this->op_8xye(); };
        table[chip8::OPCODE_INDEX::OP_00E0] = [this]() { this->op_00e0(); };
        table[chip8::OPCODE_INDEX::OP_00EE] = [this]() { this->op_00ee(); };
        table[chip8::OPCODE_INDEX::OP_FX07] = [this]() { this->op_fx07(); };
        table[chip8::OPCODE_INDEX::OP_FX0A] = [this]() { this->op_fx0a(); };
        table[chip8::OPCODE_INDEX::OP_FX15] = [this]() { this->op_fx15(); };
        table[chip8::OPCODE_INDEX::OP_FX18] = [this]() { this->op_fx18(); };
        table[chip8::OPCODE_INDEX::OP_FX1E] = [this]() { this->op_fx1e(); };
        table[chip8::OPCODE_INDEX::OP_FX29] = [this]() { this->op_fx29(); };
        table[chip8::OPCODE_INDEX::OP_FX33] = [this]() { this->op_fx33(); };
        table[chip8::OPCODE_INDEX::OP_FX65] = [this]() { this->op_fx65(); };
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
        uint8_t nibble2 = (opcode & 0x0f00) >> 8;
        uint8_t nibble3 = (opcode & 0x00f0) >> 4;
        uint8_t nibble4 = opcode & 0x000f;

        if (nibble1 >= 1 && nibble1 <= 0xd && nibble1 != 8) {
            table[nibble1]();
        } else if (nibble1 == 8) {
            table[OPCODE_INDEX::OP_8XYN + nibble4]();
        } else if (nibble1 == 0xe) {
            if (nibble4 == 0xe) {
                table[OPCODE_INDEX::OP_EX9E]();
            } else {
                table[OPCODE_INDEX::OP_EXA1]();
            }
        } else if (nibble1 == 0) {
            if (nibble4 == 0) {
                table[OPCODE_INDEX::OP_00E0]();
            } else {
                table[OPCODE_INDEX::OP_00EE]();
            }
        } else if (nibble1 == 0xf) {
            switch (nibble4) {
            case 7:
                table[OPCODE_INDEX::OP_FX07]();
                break;
            case 0xa:
                table[OPCODE_INDEX::OP_FX0A]();
                break;
            case 5:
                switch (nibble2) {
                case 1:
                    table[OPCODE_INDEX::OP_FX15]();
                    break;
                case 6:
                    table[OPCODE_INDEX::OP_FX65]();
                    break;
                }
                break;
            case 8:
                table[OPCODE_INDEX::OP_FX18]();
                break;
            case 0xe:
                table[OPCODE_INDEX::OP_FX1E]();
                break;
            case 9:
                table[OPCODE_INDEX::OP_FX29]();
                break;
            case 3:
                table[OPCODE_INDEX::OP_FX33]();
                break;
            }
        }
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
        for (int i = 0; i < 0xf; i++) {
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

    InitWindow(1280, 720, "chip-8 emulator");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        fill(chip.keyboard.begin(), chip.keyboard.end(), 0);
        if (IsKeyDown(KEY_ONE)) {
            chip.keyboard[0] = true;
        }
        if (IsKeyDown(KEY_TWO)) {
            chip.keyboard[1] = true;
        }
        if (IsKeyDown(KEY_THREE)) {
            chip.keyboard[2] = true;
        }
        if (IsKeyDown(KEY_FOUR)) {
            chip.keyboard[3] = true;
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
            chip.keyboard[7] = true;
        }
        if (IsKeyDown(KEY_A)) {
            chip.keyboard[8] = true;
        }
        if (IsKeyDown(KEY_S)) {
            chip.keyboard[9] = true;
        }
        if (IsKeyDown(KEY_D)) {
            chip.keyboard[10] = true;
        }
        if (IsKeyDown(KEY_F)) {
            chip.keyboard[11] = true;
        }
        if (IsKeyDown(KEY_Z)) {
            chip.keyboard[12] = true;
        }
        if (IsKeyDown(KEY_X)) {
            chip.keyboard[13] = true;
        }
        if (IsKeyDown(KEY_C)) {
            chip.keyboard[14] = true;
        }
        if (IsKeyDown(KEY_V)) {
            chip.keyboard[15] = true;
        }

        BeginDrawing();
        ClearBackground(WHITE);
        DrawVideo(chip.display);
        EndDrawing();

        chip.cycle();
    }

    CloseWindow();
    return 0;
}
