#include <string.h>
#include <stdbool.h>
#include <inttypes.h>


struct chip8{
    bool drawFlag;

    uint8_t opcode;      //Current opcode
    uint16_t memory[4096]; //Memory (size= 4KB)

    uint16_t V[16];        //Registers (V0-VF)
    uint8_t I;           //Index register
    uint16_t pc;          //Program counter

    uint16_t gfx[64*32];   //Total amount of pixels: 2048

    uint16_t delay_timer;  //Delay timer
    uint16_t sound_timer;  //Sound timer

    uint8_t stack[16];   //Stack (16 levels)
    uint8_t sp;          //Stack counter

    uint16_t key[16];      //INPUT
};


void loadApplication(struct chip8* chip8, const char* filename);
void emulateCycle(struct chip8* chip8);
void initialize(struct chip8* chip8);