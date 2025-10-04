#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define TEXT_BASE 0x00400000
#define DATA_BASE 0x10000000

typedef struct {
	const char* name;
	char type;
	int opcode;
	int funct;
} MipsOp;

const MipsOp OP_TABLE[] = {
	{"add",    'R',  0x00,   0x20},
	{"sub",    'R',  0x00,   0x22},
	{"and",    'R',  0x00,   0x24},
	{"or",     'R',  0x00,   0x25},
	{"nor",    'R',  0x00,   0x27},
	{"slt",    'R',  0x00,   0x2a},
	{"sll",    'R',  0x00,   0x00},
	{"srl",    'R',  0x00,   0x02},
	{"jr",     'R',  0x00,   0x08},
	{"addi",   'I',  0x08,   0},
	{"andi",   'I',  0x0c,   0},
	{"ori",    'I',  0x0d,   0},
	{"slti",   'I',  0x0a,   0},
	{"beq",    'I',  0x04,   0},
	{"bne",    'I',  0x05,   0},
	{"lw",     'I',  0x23,   0},
	{"sw",     'I',  0x2b,   0},
	{"lui",    'I',  0x0f,   0},
	{"j",      'J',  0x02,   0},
	{"jal",    'J',  0x03,   0}
};

static unsigned encode_r(unsigned rs, unsigned rt, unsigned rd, unsigned shamt, unsigned funct);
static unsigned encode_i(unsigned op, unsigned rs, unsigned rt, unsigned imm);
static unsigned encode_j(unsigned op, unsigned target_addr);
static char* trim(char* s);



int main(int argc, char* argv[]) {

	if (argc != 2) {
		printf("Usage: ./runfile <assembly file>\n"); //Example) ./runfile /sample_input/example1.s
		printf("Example) ./runfile ./sample_input/example1.s\n");
		exit(0);
	}
	else {

		// To help you handle the file IO, the deafult code is provided.
		// If we use freopen, we don't need to use fscanf, fprint,..etc. 
		// You can just use scanf or printf function 
		// ** You don't need to modify this part **
		// If you are not famailiar with freopen,  you can see the following reference
		// http://www.cplusplus.com/reference/cstdio/freopen/

		//For input file read (sample_input/example*.s)

		char* file = (char*)malloc(strlen(argv[1]) + 3);
		strncpy(file, argv[1], strlen(argv[1]));

		if (freopen(file, "r", stdin) == 0) {
			printf("File open Error!\n");
			exit(1);
		}

		//From now on, if you want to read string from input file, you can just use scanf function.


		// For output file write 
		// You can see your code's output in the sample_input/example#.o 
		// So you can check what is the difference between your output and the answer directly if you see that file
		// make test command will compare your output with the answer
		file[strlen(file) - 1] = 'o';
		freopen(file, "w", stdout);

		//If you use printf from now on, the result will be written to the output file.

		while (true) {

		}

		rewind(stdin);

		while (true) {

		}

	}

	return 0;
}

static unsigned encode_r(unsigned rs, unsigned rt, unsigned rd, unsigned shamt, unsigned funct) {
	return (rs << 21) | (rt << 16) | (rd << 11) | (shamt << 6) | (funct);
}

static unsigned encode_j(unsigned op, unsigned target_addr) {
	return (op << 26) | ((target_addr << 2) & 0x03FFFFFF);
}

static unsigned encode_i(unsigned op, unsigned rs, unsigned rt, unsigned imm) {
	return (op << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFFu);
}

static char* trim(char* s) {

}

static char*
