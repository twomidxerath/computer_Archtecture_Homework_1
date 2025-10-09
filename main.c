#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_SYMBOL 100
#define MAX_LINE 256
#define TEXT_BASE 0x00400000u
#define DATA_BASE 0x10000000u

typedef struct {
    const char* name;
    char type;
    int opcode;
    int funct;
} Operation;

typedef struct {
    char symbol_name[32];
    unsigned int address;
} Symbol;

const Operation OP_TABLE[] = {
    {"add", 'R', 0x00, 0x20}, {"sub", 'R', 0x00, 0x22},
    {"and", 'R', 0x00, 0x24}, {"or", 'R', 0x00, 0x25},
    {"nor", 'R', 0x00, 0x27}, {"slt", 'R', 0x00, 0x2a},
    {"sll", 'R', 0x00, 0x00}, {"srl", 'R', 0x00, 0x02},
    {"jr", 'R', 0x00, 0x08}, {"addi", 'I', 0x08, 0},
    {"andi", 'I', 0x0c, 0}, {"ori", 'I', 0x0d, 0},
    {"slti", 'I', 0x0a, 0}, {"beq", 'I', 0x04, 0},
    {"bne", 'I', 0x05, 0}, {"lw", 'I', 0x23, 0},
    {"sw", 'I', 0x2b, 0}, {"lui", 'I', 0x0f, 0},
    {"j", 'J', 0x02, 0}, {"jal", 'J', 0x03, 0}
};

Symbol symbol_table[MAX_SYMBOL];
int symbol_count = 0;

static unsigned int encode_r(unsigned int rs, unsigned int rt, unsigned int rd, unsigned int shamt, unsigned int funct);
static unsigned int encode_i(unsigned int op, unsigned int rs, unsigned int rt, int imm);
static unsigned int encode_j(unsigned int op, unsigned int target_addr);
static unsigned int string_to_Instruction(char* line, unsigned int current_address);
void num_to_binary(unsigned int num, char* buffer);
static char* trim(char* s);
bool end_token(const char* s);
void add_symbol(char* label_token, bool isData, unsigned int data_addr, unsigned int text_addr);
unsigned int search_symbol_address(const char* symbolname);
const Operation* find_op(const char* mnemonic);
char* remove_comment(char* line);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: ./runfile <assembly file>\n");
        printf("Example) ./runfile ./sample_input/example1.s\n");
        exit(0);
    }

    char* file = (char*)malloc(strlen(argv[1]) + 3);
    strncpy(file, argv[1], strlen(argv[1]));

    if (freopen(file, "r", stdin) == 0) {
        printf("File open Error!\n");
        exit(1);
    }

    unsigned int data_address_ptr = DATA_BASE;
    unsigned int text_address_ptr = TEXT_BASE;
    bool isData = false;
    char line_buffer[MAX_LINE];

    while (fgets(line_buffer, MAX_LINE, stdin) != NULL) {
        char* line = trim(remove_comment(line_buffer));
        if (!(*line)) continue;

        if (strcmp(line, ".data") == 0) {
            isData = true;
            continue;
        } else if (strcmp(line, ".text") == 0) {
            isData = false;
            continue;
        }

        char line_copy[MAX_LINE];
        strcpy(line_copy, line);

        char* token1 = strtok(line_copy, " \t,");
        if (token1 == NULL) continue;

        char* main_token = token1;
        if (end_token(token1)) {
            add_symbol(token1, isData, data_address_ptr, text_address_ptr);
            main_token = strtok(NULL, " \t,");
        }

        if (main_token == NULL) continue;
        
        if (strcmp(main_token, ".word") == 0) {
            data_address_ptr += 4;
        } else if (strcmp(main_token, "la") == 0) {
            text_address_ptr += 8;
        } else {
            text_address_ptr += 4;
        }
    }

    rewind(stdin);
    text_address_ptr = TEXT_BASE;
    isData = false;
    
    unsigned int text_section[MAX_LINE * 2] = {0};
    unsigned int data_section[MAX_LINE] = {0};
    int text_count = 0;
    int data_count = 0;

    while (fgets(line_buffer, MAX_LINE, stdin) != NULL) {
        char* line = trim(remove_comment(line_buffer));
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
            if (directive != NULL && strcmp(directive, ".word") == 0) {
                char* value_str = strtok(NULL, " \t,");
                if (value_str != NULL) {
                    data_section[data_count++] = strtol(value_str, NULL, 0);
                }
            }
        } else {
            char line_copy[MAX_LINE];
            strcpy(line_copy, line);
            char* mnemonic = strtok(line_copy, " \t,");

            if (strcmp(mnemonic, "la") == 0) {
                char* rt_ptr = strtok(NULL, " \t,");
                char* label_ptr = strtok(NULL, " \t,");
                
                unsigned int addr = search_symbol_address(label_ptr);
                unsigned int upper = addr >> 16;
                unsigned int lower = addr & 0xFFFF;
                
                const Operation* lui_op = find_op("lui");
                text_section[text_count++] = encode_i(lui_op->opcode, 0, 1, upper);
                text_address_ptr += 4;

                if (lower != 0) {
                    const Operation* ori_op = find_op("ori");
                    text_section[text_count++] = encode_i(ori_op->opcode, 1, atoi(rt_ptr + 1), lower);
                    text_address_ptr += 4;
                }
            } else {
                text_section[text_count++] = string_to_Instruction(line, text_address_ptr);
                text_address_ptr += 4;
            }
        }
    }

    file[strlen(file)] = '\0';
    freopen(file, "w", stdout);

    char binary_buffer[33];
    
    unsigned int text_size = text_count * 4;
    num_to_binary(text_size, binary_buffer);
    printf("%s", binary_buffer);

    unsigned int data_size = data_count * 4;
    num_to_binary(data_size, binary_buffer);
    printf("%s", binary_buffer);

    for (int i = 0; i < text_count; i++) {
        num_to_binary(text_section[i], binary_buffer);
        printf("%s", binary_buffer);
    }

    for (int i = 0; i < data_count; i++) {
        num_to_binary(data_section[i], binary_buffer);
        printf("%s", binary_buffer);
    }

    return 0;
}

static unsigned int encode_r(unsigned int rs, unsigned int rt, unsigned int rd, unsigned int shamt, unsigned int funct) {
    return (rs << 21) | (rt << 16) | (rd << 11) | (shamt << 6) | funct;
}

static unsigned int encode_j(unsigned int op, unsigned int target_addr) {
    return (op << 26) | ((target_addr >> 2) & 0x03FFFFFF);
}

static unsigned int encode_i(unsigned int op, unsigned int rs, unsigned int rt, int imm) {
    return (op << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFF);
}

void num_to_binary(unsigned int num, char* buffer) {
    buffer[32] = '\0';
    for (int i = 0; i < 32; i++) {
        unsigned int mask = 1u << (31 - i);
        buffer[i] = (num & mask) ? '1' : '0';
    }
}

bool end_token(const char* s) {
    int len = strlen(s);
    return (len > 0 && s[len - 1] == ':');
}

void add_symbol(char* label_token, bool isData, unsigned int data_addr, unsigned int text_addr) {
    if (symbol_count >= MAX_SYMBOL) return;
    
    char* colon = strchr(label_token, ':');
    if (colon != NULL) *colon = '\0';
    
    strncpy(symbol_table[symbol_count].symbol_name, label_token, 31);
    symbol_table[symbol_count].symbol_name[31] = '\0';

    if (colon != NULL) *colon = ':';
    
    symbol_table[symbol_count].address = isData ? data_addr : text_addr;
    symbol_count++;
}

unsigned int search_symbol_address(const char* symbolname) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbolname, symbol_table[i].symbol_name) == 0) {
            return symbol_table[i].address;
        }
    }
    return NULL;
}

static char* trim(char* s) {
    if (s == NULL) return NULL;
    char* start = s;
    while (isspace((unsigned char)*start)) start++;

    if (*start == 0) return start;

    char* end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    
    *(end + 1) = '\0';
    return start;
}

const Operation* find_op(const char* mnemonic) {
    for (int i = 0; i < sizeof(OP_TABLE) / sizeof(Operation); i++) {
        if (strcmp(mnemonic, OP_TABLE[i].name) == 0) {
            return &OP_TABLE[i];
        }
    }
    return NULL;
}

char* remove_comment(char* line) {
    char* comment_start = strchr(line, '#');
    if (comment_start != NULL) *comment_start = '\0';
    return line;
}

static unsigned int string_to_Instruction(char* line, unsigned int current_address) {
    char* delimiters = " ,\t\n";
    
    char* mnemonic = strtok(line, delimiters);
    const Operation* op_info = find_op(mnemonic);

    if (op_info == NULL) return 0;

    if (op_info->type == 'R') {
        if (strcmp(op_info->name, "sll") == 0 || strcmp(op_info->name, "srl") == 0) {
            char* rd_ptr = strtok(NULL, delimiters);
            char* rt_ptr = strtok(NULL, delimiters);
            char* shamt_ptr = strtok(NULL, delimiters);
            int rd = atoi(rd_ptr + 1);
            int rt = atoi(rt_ptr + 1);
            int shamt = strtol(shamt_ptr, NULL, 0);
            return encode_r(0, rt, rd, shamt, op_info->funct);
        } else if (strcmp(op_info->name, "jr") == 0) {
            char* rs_ptr = strtok(NULL, delimiters);
            int rs = atoi(rs_ptr + 1);
            return encode_r(rs, 0, 0, 0, op_info->funct);
        } else {
            char* rd_ptr = strtok(NULL, delimiters);
            char* rs_ptr = strtok(NULL, delimiters);
            char* rt_ptr = strtok(NULL, delimiters);
            int rd = atoi(rd_ptr + 1);
            int rs = atoi(rs_ptr + 1);
            int rt = atoi(rt_ptr + 1);
            return encode_r(rs, rt, rd, 0, op_info->funct);
        }
    } else if (op_info->type == 'I') {
        if (strcmp(op_info->name, "lw") == 0 || strcmp(op_info->name, "sw") == 0) {
            char* rt_ptr = strtok(NULL, delimiters);
            char* imm_rs_ptr = strtok(NULL, delimiters);
            char* imm_str = strtok(imm_rs_ptr, "()");
            char* rs_str = strtok(NULL, "()");
            int rt = atoi(rt_ptr + 1);
            int imm = strtol(imm_str, NULL, 0);
            int rs = atoi(rs_str + 1);
            return encode_i(op_info->opcode, rs, rt, imm);
        } else if (strcmp(op_info->name, "beq") == 0 || strcmp(op_info->name, "bne") == 0) {
            char* rs_ptr = strtok(NULL, delimiters);
            char* rt_ptr = strtok(NULL, delimiters);
            char* label_ptr = strtok(NULL, delimiters);
            unsigned int target_addr = search_symbol_address(label_ptr);
            int offset = (target_addr - (current_address + 4)) / 4;
            int rs = atoi(rs_ptr + 1);
            int rt = atoi(rt_ptr + 1);
            return encode_i(op_info->opcode, rs, rt, offset);
        } else if (strcmp(op_info->name, "lui") == 0) {
            char* rt_ptr = strtok(NULL, delimiters);
            char* imm_ptr = strtok(NULL, delimiters);
            int rt = atoi(rt_ptr + 1);
            int imm = strtol(imm_ptr, NULL, 0);

            return encode_i(op_info->opcode, 0, rt, imm);
        } else {
            char* rt_ptr = strtok(NULL, delimiters);
            char* rs_ptr = strtok(NULL, delimiters);
            char* imm_ptr = strtok(NULL, delimiters);
            int rt = atoi(rt_ptr + 1);
            int rs = atoi(rs_ptr + 1);
            int imm = strtol(imm_ptr, NULL, 0);
            
			return encode_i(op_info->opcode, rs, rt, imm);
        }
    } else {
        char* label_ptr = strtok(NULL, delimiters);
        unsigned int target_addr = search_symbol_address(label_ptr);
        
		return encode_j(op_info->opcode, target_addr);
    }
}