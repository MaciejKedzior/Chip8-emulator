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
                    chip8->pc += 2;
                break;

                case 0x0001:        // 0x8XY1 - Set VX to VX | VY
                    chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->V[(chip8->opcode & 0x0F00) >> 8] | chip8->V[(chip8->opcode & 0x00F0) >> 4];
                    chip8->pc += 2;
                break;
                
                case 0x0002:        // 0x8XY2 - Set VX to VX & VY
                    chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->V[(chip8->opcode & 0x0F00) >> 8] & chip8->V[(chip8->opcode & 0x00F0) >> 4];
                    chip8->pc += 2;
                break;

                case 0x0003:        // 0x8XY3 - Set VX to VX ^ VY (XOR)
                    chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->V[(chip8->opcode & 0x0F00) >> 8] ^ chip8->V[(chip8->opcode & 0x00F0) >> 4];
                    chip8->pc += 2;
                break;

                case 0x0004:        // 0x8XY4 - Adds VY to VX. VF is set to 1 when there's carry and 0 when there isn't
                    if (chip8->V[(chip8->opcode & 0x00F0) >> 4] > (0xFF - chip8->V[chip8->opcode & 0x0F00]))
                        chip8->V[0xF] = 1;      //Set to carry
                    else
                        chip8->V[0xF] = 0;
                    chip8->V[(chip8->opcode & 0x00F0) >> 8 ] += chip8->V[(chip8->opcode & 0x00F) >> 4];
                    chip8->pc += 2;
                break;

                case 0x0005:        // 0x8XY5 - VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't. 
                    if (chip8->V[(chip8->opcode & 0x00F0) >> 4] > (0xFF - chip8->V[chip8->opcode & 0x0F00]))
                        chip8->V[0xF] = 1;      //Set to borrow
                    else
                        chip8->V[0xF] = 0;
                    chip8->V[(chip8->opcode & 0x00F0) >> 8 ] += chip8->V[(chip8->opcode & 0x00F) >> 4];
                    chip8->pc += 2;
                break;

                case 0x0006:        // 0x8XY6 - Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
                    chip8->V[0xF] = chip8->V[(chip8->opcode & 0x0F00) >> 8] & 0x1;
                    chip8->V[(chip8->opcode & 0x0F00) >> 8] >>= 1;
                    chip8->pc += 2;
                break;

                case 0x0007:        // 0x8XY7 - Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't. 
                    if (chip8->V[(chip8->opcode & 0x00F0) >> 4] > (0xFF - chip8->V[chip8->opcode & 0x0F00]))
                        chip8->V[0xF] = 1;      //Set to borrow
                    else
                        chip8->V[0xF] = 0;
                    chip8->V[(chip8->opcode & 0x00F0) >> 8 ] = chip8->V[(chip8->opcode & 0x00F) >> 4] - chip8->V[(chip8->opcode & 0x00F0) >> 8 ];
                    chip8->pc += 2;
                break;

                case 0x000E:        // 0x8XYE - Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
                    chip8->V[0xF] = chip8->V[(chip8->opcode & 0x0F00) >> 8] >> 7;
                    chip8->V[(chip8->opcode & 0x0F00) >> 8] <<=  1;
                    chip8->pc += 2;
                break;
            }
            
        case 0x9000:    // 0x9XY0 - Skips the next instruction if VX doesn't equal VY. (Usually the next instruction is a jump to skip a code block) 
            if (chip8->V[(chip8->opcode & 0x0F00) >> 8] != chip8->V[(chip8->opcode & 0x00F0) >> 4])
                chip8->pc += 4;
            else
                chip8->pc += 2;
        break;

        case 0xA000:    // 0xANNN - Sets I to the address NNN. 
            chip8->I = chip8->opcode & 0x0FFF;
            chip8->pc += 2;
        break;

        case 0xB000:    // 0xBNNN - Jumps to the address NNN plus V0. 
            chip8->pc = chip8->opcode & 0x0FFF + chip8->V[0x0];
        break;


        case 0xC000:   // 0xCXNN - Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN. 
            chip8->V[(chip8->opcode & 0x0F00) >> 8] = (chip8->opcode & 0x00FF) & rand();
            chip8->pc += 2;
        break;

        //  NOW THERE WOULD BE FEW OPCODES CONNECTED WITH DISPLAY AND KEYS:
        //  DXYN, EX9E, EXA1, FX0A 

        case 0xF007:    // 0xFX07 - Sets VX to the value of the delay timer. 
            chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->delay_timer;
            chip8->pc += 2;
        break;

        case 0xF015:    // 0xFX15 - Sets the delay timer to VX. 
            chip8->delay_timer = chip8->V[(chip8->opcode & 0x0F00) >> 8];
            chip8->pc += 2;
        break;

        case 0xF018:    // 0xFX18 - Sets the sound timer to VX. 
            chip8->sound_timer = chip8->V[(chip8->opcode & 0x0F00) >> 8];
            chip8->pc += 2;
        break;

        case 0xF01E:    // 0xFX1E - Adds VX to I. VF is not affected.
            chip8->I += chip8->V[(chip8->opcode & 0x0F00) >> 8];
            chip8->pc += 2;
        break;

        case 0xF033:    // 0xFX33 - Stores the binary-coded decimal representation of VX, with the most significant of three digits at the address in I, the middle digit at I plus 1, and the least significant digit at I plus 2. (In other words, take the decimal representation of VX, place the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.) 
            chip8->memory[chip8->I] = chip8->V[(chip8->opcode & 0x0F00) >> 8] / 100;
            chip8->memory[chip8->I+1] = (chip8->V[(chip8->opcode & 0x0F00) >> 8] / 10) % 10;
            chip8->memory[chip8->I+2] = (chip8->V[(chip8->opcode & 0x0F00) >> 8] / 100) % 10;
            chip8->pc += 2;
        break;

        case 0xF029:    // 0xFX29 - Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font. 
            //to be added
        break;

        case 0xF055:    // 0xFX55 - Stores V0 to VX (including VX) in memory starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified
            for (int i =0; i <= ((chip8->opcode & 0x0F00) >> 8); i++){
                chip8->memory[chip8->I + i] = chip8->V[i];
            }

            chip8->I += ((chip8->opcode & 0x0F00) >> 8) + 1;
					chip8->pc += 2;
        break;

        case 0xF065:    // 0xFX65 - Fills V0 to VX (including VX) with values from memory starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified.
            for (int i =0; i <= ((chip8->opcode & 0x0F00) >> 8); i++){
                chip8->V[i] = chip8->memory[chip8->I + i];
            }

            chip8->I += ((chip8->opcode & 0x0F00) >> 8) + 1;
					chip8->pc += 2;
        break;

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