
unsigned short opcode;      //OPCODE
unsigned char memory[4096]; //MEMORY

unsigned char V[16];        //REGISTERS
unsigned short I;
unsigned short pc=0;

unsigned char gfx[64*32];

unsigned char delay_timer;
unsigned char sound_timer;

unsigned short stack[16];
unsigned short sp;

unsigned char key[16];


int main(void){
    memory[pc] = 0xA2;
    memory[pc+1] = 0xF0;
    opcode = memory[pc] << 8 | memory[pc+1];
    I = opcode & 0xFFF;
    pc += 2;
}