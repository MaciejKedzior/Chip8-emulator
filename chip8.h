#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#define MEM_SIZ 3096
#define PIXELS 64*32
#define HEX 16

struct chip8{
    bool drawFlag;

    uint8_t opcode;      //Current opcode
    uint16_t memory[MEM_SIZ]; //Memory (size= 4KB)

    uint16_t V[HEX];        //Registers (V0-VF)
    uint8_t I;           //Index register
    uint16_t pc;          //Program counter

    uint16_t gfx[PIXELS];   //Total amount of pixels: 2048

    uint16_t delay_timer;  //Delay timer
    uint16_t sound_timer;  //Sound timer

    uint8_t stack[HEX];   //Stack (16 levels)
    uint8_t sp;          //Stack counter

    uint16_t key[HEX];      //INPUT
};


void loadApplication(struct chip8* chip8, const char* filename);
void emulateCycle(struct chip8* chip8);
void initialize(struct chip8* chip8);