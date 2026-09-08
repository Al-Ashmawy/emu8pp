#include "Chip8.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

using namespace std;

Chip8::Chip8(const string& rom)
{
    load_rom(rom);
    write_sprites();
    init_table();
}

void Chip8::cycle()
{
    fetch();

    decode_execute();
}

const vector<vector<uint8_t>>& Chip8::get_display() const
{
    return display;
}

void Chip8::set_key(int i)
{
    keyboard[i] = true;
}

void Chip8::unset_keys()
{
    fill(keyboard.begin(), keyboard.end(), false);
}

void Chip8::tick_timers()
{
    if (delay > 0) {
        delay--;
    }
    if (sound > 0) {
        sound--;
    }
}

bool Chip8::is_halted()
{
    return halted;
}

bool Chip8::is_rom_loaded()
{
    return rom_loaded;
}

string Chip8::error()
{
    return error_message;
}

bool Chip8::is_playing_sound()
{
    return sound != 0;
}

void Chip8::invalid_instruction()
{
    halted = true;
    error_message = format("invalid instruction {:4x}", opcode);
}

void Chip8::init_table()
{
    table_0[0x00e0] = [this]() { op_00e0(); };
    table_0[0x00ee] = [this]() { op_00ee(); };

    table[0x0] = [this]() {
        auto it = table_0.find(this->opcode);
        if (it == table_0.end()) {
            invalid_instruction();
            return;
        }
        it->second();
    };
    table[0x1] = [this]() { op_1nnn(); };
    table[0x2] = [this]() { op_2nnn(); };
    table[0x3] = [this]() { op_3xkk(); };
    table[0x4] = [this]() { op_4xkk(); };
    table[0x5] = [this]() {
        if ((opcode & 0x000f) != 0x0) {
            invalid_instruction();
            return;
        }
        op_5xy0();
    };
    table[0x6] = [this]() { op_6xkk(); };
    table[0x7] = [this]() { op_7xkk(); };
    table[0x8] = [this]() {
        auto it = table_8.find(this->opcode & 0xf);
        if (it == table_8.end()) {
            halted = true;
            error_message = format("invalid instruction {:4x}", opcode);
            return;
        }
        it->second();
    };
    table[0x9] = [this]() {
        if ((opcode & 0xf) != 0x0) {
            invalid_instruction();
            return;
        }
        op_9xy0();
    };
    table[0xa] = [this]() { op_annn(); };
    table[0xb] = [this]() { op_bnnn(); };
    table[0xc] = [this]() { op_cxkk(); };
    table[0xd] = [this]() { op_dxyn(); };
    table[0xe] = [this]() {
        auto it = table_e.find(this->opcode & 0xff);
        if (it == table_e.end()) {
            invalid_instruction();
            return;
        }
        it->second();
    };
    table[0xf] = [this]() {
        auto it = table_f.find(this->opcode & 0xff);
        if (it == table_f.end()) {
            invalid_instruction();
            return;
        }
        it->second();
    };

    table_8[0x0] = [this]() { op_8xy0(); };
    table_8[0x1] = [this]() { op_8xy1(); };
    table_8[0x2] = [this]() { op_8xy2(); };
    table_8[0x3] = [this]() { op_8xy3(); };
    table_8[0x4] = [this]() { op_8xy4(); };
    table_8[0x5] = [this]() { op_8xy5(); };
    table_8[0x6] = [this]() { op_8xy6(); };
    table_8[0x7] = [this]() { op_8xy7(); };
    table_8[0xe] = [this]() { op_8xye(); };

    table_e[0x9e] = [this]() { op_ex9e(); };
    table_e[0xa1] = [this]() { op_exa1(); };

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

void Chip8::load_rom(const string& rom)
{
    ifstream file(rom, ios::binary);
    int code = errno;
    if (!file.is_open()) {
        rom_loaded = false;
        error_message = format("can't open file {}: {}", rom, strerror(code));
        return;
    }

    size_t size = filesystem::file_size(rom);
    file.read(reinterpret_cast<char*>(&memory[EXECUTION_START]), size);
}

void Chip8::write_sprites()
{
    size_t i = FONT_START;
    for (const auto& vec : font_sprites) {
        for (const char byte : vec) {
            memory[i] = byte;
            i++;
        }
    }
}

void Chip8::fetch()
{
    opcode = (memory[pc] << 8) | memory[pc + 1];
    pc += 2;

    x = (opcode & 0x0f00) >> 8;
    y = (opcode & 0x00f0) >> 4;
    n = opcode & 0x000f;
    kk = opcode & 0x00ff;
    nnn = opcode & 0x0fff;
}

void Chip8::decode_execute()
{
    uint8_t nibble1 = opcode >> 12;

    table[nibble1]();
}

void Chip8::op_00e0()
{
    for (auto& vec : display) {
        for (auto& pixel : vec) {
            pixel = 0;
        }
    }
}

void Chip8::op_00ee()
{
    pc = stack[sp];
    sp--;
}

void Chip8::op_1nnn()
{
    pc = nnn;
}

void Chip8::op_2nnn()
{
    sp++;
    stack[sp] = pc;
    pc = nnn;
}

void Chip8::op_3xkk()
{
    if (registers[x] == kk) {
        pc += 2;
    }
}

void Chip8::op_4xkk()
{
    if (registers[x] != kk) {
        pc += 2;
    }
}

void Chip8::op_5xy0()
{
    if (registers[x] == registers[y]) {
        pc += 2;
    }
}

void Chip8::op_6xkk()
{
    registers[x] = kk;
}

void Chip8::op_7xkk()
{
    registers[x] = registers[x] + kk;
}

void Chip8::op_8xy0()
{
    registers[x] = registers[y];
}

void Chip8::op_8xy1()
{
    registers[x] = registers[x] | registers[y];
}

void Chip8::op_8xy2()
{
    registers[x] = registers[x] & registers[y];
}

void Chip8::op_8xy3()
{
    registers[x] = registers[x] ^ registers[y];
}

void Chip8::op_8xy4()
{
    uint16_t result = registers[x] + registers[y];
    if (result > 255) {
        registers[0xf] = 1;
    } else {
        registers[0xf] = 0;
    }
    registers[x] = result;
}

void Chip8::op_8xy5()
{
    if (registers[x] > registers[y]) {
        registers[0xf] = 1;
    } else {
        registers[0xf] = 0;
    }
    registers[x] = registers[x] - registers[y];
}

void Chip8::op_8xy6()
{
    registers[0xf] = registers[x] & 1;
    registers[x] >>= 1;
}

void Chip8::op_8xy7()
{
    if (registers[y] > registers[x]) {
        registers[0xf] = 1;
    } else {
        registers[0xf] = 0;
    }
    registers[x] = registers[y] - registers[x];
}

void Chip8::op_8xye()
{
    registers[0xf] = (registers[x] >> 7);
    registers[x] <<= 1;
}

void Chip8::op_9xy0()
{
    if (registers[x] != registers[y]) {
        pc += 2;
    }
}

void Chip8::op_annn()
{
    index = nnn;
}

void Chip8::op_bnnn()
{
    pc = nnn + registers[0];
}

void Chip8::op_cxkk()
{
    registers[x] = (random_byte() % 256) & kk;
}

void Chip8::op_dxyn()
{
    uint8_t vx = registers[x];
    uint8_t vy = registers[y];

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

void Chip8::op_ex9e()
{
    uint8_t vx = registers[x];
    if (keyboard[vx]) {
        pc += 2;
    }
}

void Chip8::op_exa1()
{
    uint8_t vx = registers[x];
    if (!keyboard[vx]) {
        pc += 2;
    }
}

void Chip8::op_fx07()
{
    registers[x] = delay;
}

void Chip8::op_fx0a()
{
    for (int i = 0; i <= 0xf; i++) {
        if (keyboard[i]) {
            registers[x] = i;
            return;
        }
    }

    pc -= 2;
}

void Chip8::op_fx15()
{
    delay = registers[x];
}

void Chip8::op_fx18()
{
    sound = registers[x];
}

void Chip8::op_fx1e()
{
    index = index + registers[x];
}

void Chip8::op_fx29()
{
    index = FONT_START + registers[x] * 5;
}

void Chip8::op_fx33()
{
    uint8_t num = registers[x];
    for (int i = 2; i >= 0; i--) {
        memory[index + i] = num % 10;
        num /= 10;
    }
}

void Chip8::op_fx55()
{
    for (size_t i = 0; i <= x; i++) {
        memory[index + i] = registers[i];
    }
}

void Chip8::op_fx65()
{
    for (size_t i = 0; i <= x; i++) {
        registers[i] = memory[index + i];
    }
}
