#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_NUM 10
#define MAX_SYMBOL 20
#define MAX_LINE 50

#define TEXT_BASE 0x00400000u
#define DATA_BASE 0x10000000u

typedef struct {
	const char* name;
	char type;
	int opcode;
	int funct;
} Operation;

typedef struct {
	const Operation *op;
	int rs, rt, rd, shamt;
	int immediate;
} Instruction;

typedef struct {
	char symbol_name[10];
	unsigned address;
}Symbol;

Symbol symbol_table[MAX_SYMBOL];
int symbol_count = 0;

unsigned data_address_ptr = DATA_BASE;
unsigned text_address_ptr = TEXT_BASE;
bool isData = false;


const Operation OP_TABLE[] = {
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
static unsigned string_to_Instruction(char* line, unsigned current_address);
void num_to_binary(unsigned num, char* buffer);
static char* trim(char* s);
bool end_token(char* s);
void add_symbol(char* label_token, bool data);
Symbol search_symbol(char* symbolname);
const Operation* find_op(char* mnemonic);

int main(int argc, char* argv[]) {

	if (argc != 2) {
		printf("Usage: ./runfile <assembly file>\n"); //Example) ./runfile /sample_input/example1.s
		printf("Example) ./runfile ./sample_input/example1.s\n");
		exit(0);
	}
	else {

		char* file = (char*)malloc(strlen(argv[1]) + 3);
		strncpy(file, argv[1], strlen(argv[1]));

		if (freopen(file, "r", stdin) == 0) {
			printf("File open Error!\n");
			exit(1);
		}

		file[strlen(file) - 1] = 'o';
		freopen(file, "w", stdout);

		unsigned text_section[MAX_LINE];
		unsigned data_section[MAX_LINE];
		int text_count = 0;
		int data_count = 0;
		char before_line[MAX_LINE];
		while (fgets(before_line, MAX_LINE, stdin)) {
			
			char* line = trim(before_line);
			if (!(*line)) continue;

			if (!strcmp(line, ".data")) {
				isData = true;
				continue;
			}
			else if (!strcmp(line, ".text")) {
				isData = false;
				continue;
			}
			char* token1 = strtok(line, " ,");
			char* token2 = strtok(line, " ,");
			char* main_token;

			if (end_token(token1)) {
				add_symbol(token1, isData);
				main_token = token2;
			}
			else {
				main_token = token2;
			}

			if (main_token == NULL) {
				continue;
			}

			if (!strcmp(main_token, ".word")) {
				data_address_ptr += 4;
			}
			else if (!strcmp(main_token, "la")) {
				text_address_ptr += 8;
			}
			else {
				text_address_ptr += 4;
			}
		}
		rewind(stdin);
		text_address_ptr = TEXT_BASE;

		while (fgets(before_line, MAX_LINE, stdin) != NULL) { 
			char* line = trim(before_line);
			if (!(*line)) continue;

			if (strcmp(line, ".data") == 0) {
				isData = true;
				continue;
			} else if (strcmp(line, ".text") == 0) {
				isData = false;
				continue;
			}

			char* colon = strchr(line, ':');
			if (colon != NULL) {
				line = trim(colon + 1); 
				if (!(*line)) continue; 
			}
			
			if (isData) {
				char* directive = strtok(line, " \t,");
				if (directive != NULL && (strcmp(directive, ".word") == 0)) {
					char* value_str = strtok(NULL, " \t,");
					data_section[data_count++] = strtol(value_str, NULL, 0);
				}
			}
			else {
				char line_copy[MAX_LINE];
				strcpy(line_copy, line);

				char* mnemonic = strtok(line_copy, " \t,");
				
				if (strcmp(mnemonic, "la") == 0) {
					char* rt_ptr = strtok(NULL, " \t,$");
					char* label_ptr = strtok(NULL, " \t,");
					
					Symbol sym = search_symbol(label_ptr);
					unsigned addr = sym.address;

					unsigned upper = addr >> 16;
					unsigned lower = addr & 0xFFFFu;

					Operation* lui_op = find_op("lui");
					text_section[text_count++] = encode_i(lui_op->opcode, 0, 1, upper);
					
					if (lower != 0) {
						Operation* ori_op = find_op("ori");
						text_section[text_count++] = encode_i(ori_op->opcode, 1, atoi(rt_ptr), lower);
					}
				}
				else {
					text_section[text_count++] = string_to_Instruction(line, text_address_ptr);
				}
			}
		}

		char final_output[4096] = "";
		char binary_buffer[33];

		unsigned text_size = text_count * 4;
		num_to_binary(text_size, binary_buffer);
		strcat(final_output, binary_buffer);

		unsigned data_size = data_count * 4;
		num_to_binary(data_size, binary_buffer);
		strcat(final_output, binary_buffer);

		for (int i = 0; i < text_count; i++) {
			num_to_binary(text_section[i], binary_buffer);
			strcat(final_output, binary_buffer);
		}


		for (int i = 0; i < data_count; i++) {
			num_to_binary(data_section[i], binary_buffer);
			strcat(final_output, binary_buffer);
		}


		printf("%s", final_output);
	}
	return 0;
}

static unsigned encode_r(unsigned rs, unsigned rt, unsigned rd, unsigned shamt, unsigned funct) {
	return (rs << 21) | (rt << 16) | (rd << 11) | (shamt << 6) | (funct);
}

static unsigned encode_j(unsigned op, unsigned target_addr) {
	return (op << 26) | ((target_addr << 2) & 0x03FFFFFFu);
}

static unsigned encode_i(unsigned op, unsigned rs, unsigned rt, unsigned imm) {
	return (op << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFFu);
}

void num_to_binary(unsigned num, char* buffer) {
	for (int i = 31; i >= 0; i--) {
		unsigned mask = 1u << i;
		if (num & mask) {
			*buffer = '1';
		}
		else {
			*buffer = '0';
		}
		buffer++;
	}
	*buffer = NULL;
}

bool end_token(char* s) {
	while (*s) {
		s++;
	}
	s--;
	if (*s == ':') {
		return true;
	}
	return false;
}

void add_symbol(char* label_token, bool data) {

    char* colon = strchr(label_token, ':');
    if (colon != NULL) {
        *colon = '\0'; 

        strcpy(symbol_table[symbol_count].symbol_name, label_token);

        *colon = ':'; 
    } else {
        strcpy(symbol_table[symbol_count].symbol_name, label_token);
    }
    
    if (data) {
        symbol_table[symbol_count].address = data_address_ptr;
    } else {
        symbol_table[symbol_count].address = text_address_ptr;
    }

    symbol_count++;
}

Symbol search_symbol(char* symbolname){
	for(int i = 0; i <= symbol_count; i++){
		if(!strcmp(symbolname, symbol_table[i].symbol_name)){
			return symbol_table[i];
		}
	}
}

static char* trim(char* s) {
	char* target = s;
	while (isspace(*target)) {
		target++;
	}
	char* rear = s + strlen(s) - 1;
	while (rear > target && isspace(*rear)) {
		rear--;
	}
	*(rear + 1) = '\0';
	return target;
}

const Operation* find_op(char* mnemonic) {
    for(int i = 0; i < sizeof(OP_TABLE) / sizeof(Operation); i++){
        if(strcmp(mnemonic, OP_TABLE[i].name) == 0){
            return &OP_TABLE[i];
        }
    }
    return NULL;
}

static unsigned string_to_Instruction(char* line, unsigned current_address) {
	Instruction I;
	char* delimiters = " ,\t\n";
    
	char* mnemonic = strtok(line, delimiters);
	const Operation *op_info = find_op(mnemonic);

	I.op = op_info;

	if (I.op->type == 'R') {

		if (strcmp(I.op->name, "sll") == 0 || strcmp(I.op->name, "srl") == 0) {
			char* rd_ptr = strtok(NULL, delimiters);
			char* rt_ptr = strtok(NULL, delimiters);
			char* shamt_ptr = strtok(NULL, delimiters);

			I.rd = atoi(rd_ptr + 1);
			I.rt = atoi(rt_ptr + 1);
			I.shamt = atoi(shamt_ptr);
			return encode_r(0, I.rt, I.rd, I.shamt, I.op->funct);
		} else if (strcmp(I.op->name, "jr") == 0) {
			char* rs_ptr = strtok(NULL, delimiters);
			I.rs = atoi(rs_ptr + 1);
			return encode_r(I.rs, 0, 0, 0, I.op->funct);
		} else {
			char* rd_ptr = strtok(NULL, delimiters);
			char* rs_ptr = strtok(NULL, delimiters);
			char* rt_ptr = strtok(NULL, delimiters);

			I.rd = atoi(rd_ptr + 1);
			I.rs = atoi(rs_ptr + 1);
			I.rt = atoi(rt_ptr + 1);
			return encode_r(I.rs, I.rt, I.rd, 0, I.op->funct);
		}
	}
	else if (I.op->type == 'I') {

		if (strcmp(I.op->name, "lw") == 0 || strcmp(I.op->name, "sw") == 0) {
			char* rt_ptr = strtok(NULL, delimiters);
			char* imm_rs_ptr = strtok(NULL, delimiters);


			char* imm_str = strtok(imm_rs_ptr, "()");
			char* rs_str = strtok(NULL, "()");

			I.rt = atoi(rt_ptr + 1);
			I.immediate = atoi(imm_str);
			I.rs = atoi(rs_str + 1);
			return encode_i(I.op->opcode, I.rs, I.rt, I.immediate);
		} 
		else if (strcmp(I.op->name, "beq") == 0 || strcmp(I.op->name, "bne") == 0) {
			char* rs_ptr = strtok(NULL, delimiters);
			char* rt_ptr = strtok(NULL, delimiters);
			char* label_ptr = strtok(NULL, delimiters);
			
			unsigned target_addr = find_symbol_address(label_ptr);
			int offset = (target_addr - (current_address + 4)) / 4;

			I.rs = atoi(rs_ptr + 1);
			I.rt = atoi(rt_ptr + 1);
			return encode_i(I.op->opcode, I.rs, I.rt, offset);
		}
		else {
			char* rt_ptr = strtok(NULL, delimiters);
            char* rs_ptr = strtok(NULL, delimiters);
			char* imm_ptr = strtok(NULL, delimiters);
			I.rt = atoi(rt_ptr + 1);
            I.rs = atoi(rs_ptr + 1);
			I.immediate = strtol(imm_ptr, NULL, 0);
			return encode_i(I.op->opcode, I.rs, I.rt, I.immediate);
		}
	} 
	else { 
		char* label_ptr = strtok(NULL, delimiters);
		unsigned target_addr = find_symbol_address(label_ptr);
		return encode_j(I.op->opcode, target_addr);
	}
}