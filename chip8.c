#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>      //for open
#include <sys/stat.h>       //for open
#include <fcntl.h>          //for open
#include <unistd.h>         //for read
#include "chip8.h"


void initialize(struct chip8* chip8){
    chip8->pc = 0x200;      //512
    chip8->I = 0;
    chip8->sp = 0;
    chip8->opcode =0;

    memset(chip8->gfx, 0, sizeof(uint16_t)*2048);
    memset(chip8->stack, 0, sizeof(uint16_t)*16);
    memset(chip8->key, 0, sizeof(uint16_t)*16);
    memset(chip8->V, 0, sizeof(uint16_t)*16);

    for (int i=0; i < 80; i++){
        chip8->memory[i] = chip8_fontset[i];
    }

    chip8->delay_timer = 0;
    chip8->sound_timer = 0;

    chip8->drawFlag = true;

    srand(time(NULL));
}


void loadApplication(struct chip8* chip8, const char* filename){
    uint32_t open_code, read_code;
    uint16_t buffer[4096];

    open_code = open(filename, O_RDONLY);
    if (open_code == -1){
        perror("Error loading application: ");
        exit(EXIT_FAILURE);
    }
    read_code = read(open_code, buffer, 4096);
    if (read_code == -1){
        perror("Error while reading file: ");
        exit(EXIT_FAILURE);
    }

    if (read_code - 512 > 4096)
        for (uint32_t i=0; i < 4096; i++)
            chip8->memory[i+512] = buffer[i];
    else{
        printf("Error, the file is too long");
        exit(EXIT_FAILURE);
    }

}

void emulateCycle(struct chip8* chip8){
    //Fetching opcode
    chip8->opcode = chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc+1];

    switch (chip8->opcode & 0xF000){
        case 0x0000:
            switch (chip8->opcode & 0x000F){

                case 0x0000:        //0x00E0: Clears the screen
                    for (uint32_t i=0; i < 64*32; ++i) chip8->gfx[i] = 0x0;
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