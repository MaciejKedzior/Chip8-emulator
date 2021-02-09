#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>      //for open
#include <sys/stat.h>       //for open
#include <fcntl.h>          //for open
#include <unistd.h>         //for read
#include "chip8.h"

static uint16_t chip8_fontset[80] = { 
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


void initialize(struct chip8* chip8){
    chip8->pc = 0x200;      //512
    chip8->I = 0;
    chip8->sp = 0;
    chip8->opcode = 0;

    memset(chip8->gfx, 0, sizeof(uint16_t)*PIXELS);
    memset(chip8->stack, 0, sizeof(uint16_t)*HEX);
    memset(chip8->key, 0, sizeof(uint16_t)*HEX);
    memset(chip8->V, 0, sizeof(uint16_t)*HEX);

    for (int i=0; i < 80; i++){
        chip8->memory[i] = chip8_fontset[i];
    }

    chip8->delay_timer = 0;
    chip8->sound_timer = 0;

    chip8->drawFlag = true;

    srand(time(NULL));
    printf("Finished initialization!\n");
}


void loadApplication(struct chip8* chip8, const char* filename){
    uint32_t open_code, read_code;
    uint16_t buffer[MEM_SIZ];

    open_code = open(filename, O_RDONLY);
    if (open_code == -1){
        perror("Error loading application: ");
        exit(EXIT_FAILURE);
    }
    read_code = read(open_code, buffer, MEM_SIZ);
    if (read_code == -1){
        perror("Error while reading file: ");
        exit(EXIT_FAILURE);
    }

    if (read_code - 512 > MEM_SIZ)
        for (uint32_t i=0; i < MEM_SIZ; i++)
            chip8->memory[i+512] = buffer[i];
    else{
        printf("Error, the file is too long");
        exit(EXIT_FAILURE);
    }
    printf("Finished loading file!\n");
}

void emulateCycle(struct chip8* chip8){
    //Fetching opcode
    chip8->opcode = chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc+1];

    //Executing opcode
    switch (chip8->opcode & 0xF000){
        case 0x0000:
            switch (chip8->opcode & 0x000F){

                case 0x0000:        //0x00E0: Clears the screen
                    for (uint32_t i=0; i < PIXELS; ++i) 
                        chip8->gfx[i] = 0x0;
                    chip8->drawFlag = true;
                    chip8->pc += 2;
                break;

                case 0x000E:        //0x00EE: Returns from the subroutine
                    --chip8->sp;
                    chip8->pc = chip8->stack[chip8->sp];
                    chip8->pc += 2;
                break;

                default:
                    printf("Unknown opcode 0x%X\n", chip8->opcode);
                break;
            }
        
        case 0x1000:   // 0x1NNN - goto NNN - jump to address NNN
            chip8->pc = chip8->opcode & 0x0FFF;
            chip8->pc += 2;
        break;

        case 0x2000:    // 0x2NNN - *(0xNNN)() - calls subroutine at NNN
            chip8->stack[chip8->sp] = chip8->pc;
            ++chip8->sp;
            chip8->pc = chip8->opcode & 0x0FFF;
        break;

        case 0x3000:    // 0x3XNN - if (VX == NN) - skips the next instruction if VX equal to NN
            if (chip8->V[(chip8->opcode & 0x0F00) >> 8] == chip8->opcode & 0x00FF)
                chip8->pc += 4;     //because we skip next instruction
            else
                chip8->pc += 2;
        break;

        case 0x4000:     // 0x4XNN 0 if (VX != NN) - skips the next instruction if VX is not equal to NN
            if (chip8->V[(chip8->opcode & 0x0F00) >> 8] != chip8->opcode & 0x00FF)
                chip8->pc += 4;     //because we skip next instruction
            else
                chip8->pc += 2;
        break;

        case 0x5000:    // 0x5XY0 - Skips the next instruction if VX is equal to VY
            if (chip8->V[(chip8->opcode & 0x0F00) >> 8] == chip8->V[(chip8->opcode & 0x00F0) >> 4])
                chip8->pc += 4;     //because we skip next instruction
            else
                chip8->pc += 2;
        break;

        case 0x6000:    // 0x6XNN - Sets VX to NN
            chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->opcode & 0x00FF;
            chip8->pc += 2;
        break;

        case 0x7000:    // 0x7XNN - Adds NN to VX (Carry flag is not changed)
            chip8->V[(chip8->opcode & 0x0F00) >> 8] += chip8->opcode & 0x00FF;
            chip8->pc += 2;
        break;

        case 0x8000:    
            switch (chip8->opcode & 0x000F){
                case 0x0000:         // 0x8XY0 - Sets VX to the value of VY.
                    chip8->V[(chip8->opcode & 0x0F00) >> 8 ] = chip8->V[(chip8->opcode & 0x00F0) >> 4];
                break;
            }
            

        case 0xA000:
            chip8->I = chip8->opcode & 0x0FFF;
            chip8->pc += 2;
        break;

        case 0x0004:
            if (chip8->V[(chip8->opcode & 0x00F0) >> 4] > (0xFF - chip8->V[chip8->opcode & 0x0F00]))
                chip8->V[0xF] = 1;      //Set to carry
            else
                chip8->V[0xF] = 0;
            chip8->V[(chip8->opcode & 0x00F0) >> 8 ] += chip8->V[(chip8->opcode & 0x00F) >> 4];
            chip8->pc += 2;
        break;

        case 0x0033:
            chip8->memory[chip8->I] = chip8->V[(chip8->opcode & 0x0F00) >> 8] / 100;
            chip8->memory[chip8->I+1] = (chip8->V[(chip8->opcode & 0x0F00) >> 8] / 10) % 10;
            chip8->memory[chip8->I+2] = (chip8->V[(chip8->opcode & 0x0F00) >> 8] / 100) % 10;
            chip8->pc += 2;
        break;

        //more cases

        default:
            printf("Unknown opcode 0x%X\n", chip8->opcode);
    }

    if (chip8->delay_timer > 0)
        --chip8->delay_timer;

    if (chip8->sound_timer > 0){
        if (chip8->sound_timer == 0)
            printf("BEEP!\n");
        --chip8->sound_timer;
    }
}