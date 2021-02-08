#include <string.h>
#include <stdbool.h>
#include <inttypes.h>


struct chip8{
    bool drawFlag;

    uint8_t opcode;      //Current opcode
    uint16_t memory[4096]; //Memory (size= 4KB)

    uint16_t V[16];        //Registers (V0-VF)
    uint8_t I;           //Index register
    uint8_t pc;          //Program counter

    uint16_t gfx[64*32];   //Total amount of pixels: 2048

    uint16_t delay_timer;  //Delay timer
    uint16_t sound_timer;  //Sound timer

    uint8_t stack[16];   //Stack (16 levels)
    uint8_t sp;          //Stack counter

    uint16_t key[16];      //INPUT
};

uint16_t chip8_fontset[80] = { 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};


void loadApplication(struct chip8* chip8, const char* filename);
void emulateCycle(struct chip8* chip8);
void initialize(struct chip8* chip8);