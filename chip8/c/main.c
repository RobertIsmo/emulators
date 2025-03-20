#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/_types/_timeval.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>

// Terminal Macros
#define INVISIBLE_CURSOR_SEQUENCE "\033[?25l"
#define SIZE_INVISIBLE_CURSOR_SEQUENCE sizeof(INVISIBLE_CURSOR_SEQUENCE)
#define VISIBLE_CURSOR_SEQUENCE "\033[?25h"
#define SIZE_VISIBLE_CURSOR_SEQUENCE sizeof(VISIBLE_CURSOR_SEQUENCE)
#define RESET_CURSOR_SEQUENCE "\033[H"
#define SIZE_RESET_CURSOR_SEQUENCE sizeof(RESET_CURSOR_SEQUENCE)
#define UP_CURSOR_SEQUENCE "\033[1A"
#define SIZE_UP_CURSOR_SEQUENCE sizeof(UP_CURSOR_SEQUENCE)
#define DOWN_CURSOR_SEQUENCE "\033[1B"
#define SIZE_DOWN_CURSOR_SEQUENCE sizeof(DOWN_CURSOR_SEQUENCE)
#define LEFT_CURSOR_SEQUENCE "\033[1D"
#define SIZE_LEFT_CURSOR_SEQUENCE sizeof(LEFT_CURSOR_SEQUENCE)
#define RIGHT_CURSOR_SEQUENCE "\033[1C"
#define SIZE_RIGHT_CURSOR_SEQUENCE sizeof(RIGHT_CURSOR_SEQUENCE)
#define NEXT_LINE_CURSOR_SEQUENCE "\033[1E"
#define SIZE_NEXT_LINE_CURSOR_SEQUENCE sizeof(NEXT_LINE_CURSOR_SEQUENCE)
#define PREV_LINE_CURSOR_SEQUENCE "\033[1F"
#define SIZE_PREV_LINE_CURSOR_SEQUENCE sizeof(PREV_LINE_CURSOR_SEQUENCE)
#define CLEAR_SEQUENCE "\033[2J\033[H\033[2J"
#define SIZE_CLEAR_SEQUENCE sizeof(CLEAR_SEQUENCE)

// Device Macros
#define FILL_CHARACTER " "
#define SIZE_FILL_CHARACTER sizeof(FILL_CHARACTER)
#define CLEAR_CHARACTER "\u2588"
#define SIZE_CLEAR_CHARACTER sizeof(CLEAR_CHARACTER)
#define SOUND_VOLUME_SEQUENCE "\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588"
#define DEFAULT_X_OFFSET 7
#define DEFAULT_Y_OFFSET 3
#define MAX_STAT_WIDTH 100
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCREEN_COUNT (SCREEN_WIDTH * SCREEN_HEIGHT)
#define SCREEN_BUFFER_SIZE ((SCREEN_WIDTH + MAX_STAT_WIDTH + 1) * (SCREEN_HEIGHT + 1))

// Machine Macros
#define SET_FRAMERATE 480.0
#define FRAME_TIME CLOCKS_PER_SEC / SET_FRAMERATE
#define STACK_LIMIT 512
#define REGISTER_COUNT 16
#define MEMORY_LIMIT 4096

// Memory Macros
#define NAME_MEMORY_SECTOR 0
#define FONT_MEMORY_SECTOR 80
#define PROGRAM_MEMORY_SECTOR 512

// Configuration Settings
#define PAUSE_ON_SAVE_MACHINE 1
#define UNPAUSE_ON_LOAD_MACHINE 1

#define SUPER_CHIP_SHIFT_CONFIGURATION 1
#define SUPER_CHIP_JUMP_CONFIGURATION 1
#define AMIGA_INDEX_OVERFLOW_CONFIGURATION 1
#define COSMAC_INDEX_INCREMENT_CONFIGURATION 0
#define COSMAC_VF_RESET_CONFIGURATION 0

/*
  SUPER-CHIP (MODERN / LEGACY)
#define SUPER_CHIP_SHIFT_CONFIGURATION 1
#define SUPER_CHIP_JUMP_CONFIGURATION 1
#define AMIGA_INDEX_OVERFLOW_CONFIGURATION 1
#define COSMAC_INDEX_INCREMENT_CONFIGURATION 0
#define COSMAC_VF_RESET_CONFIGURATION 0
*/
/*
  COSMAC CHIP-8
#define SUPER_CHIP_SHIFT_CONFIGURATION 0
#define SUPER_CHIP_JUMP_CONFIGURATION 0
#define AMIGA_INDEX_OVERFLOW_CONFIGURATION 1
#define COSMAC_INDEX_INCREMENT_CONFIGURATION 1
#define COSMAC_VF_RESET_CONFIGURATION 1
*/
struct termios term;

void disable_raw_mode() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

void enable_raw_mode() {
	tcgetattr(0, &term);
	atexit(disable_raw_mode);
	
	struct termios raw = term;
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
	raw.c_oflag &= ~(OPOST);
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	raw.c_cflag |= (CS8);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
}

void clear_terminal() {
	write(1, CLEAR_SEQUENCE, SIZE_CLEAR_SEQUENCE);
	write(2, CLEAR_SEQUENCE, SIZE_CLEAR_SEQUENCE);
	fsync(1);
	fsync(2);
	return;
}

void hide_cursor() {
	write(1, INVISIBLE_CURSOR_SEQUENCE, SIZE_INVISIBLE_CURSOR_SEQUENCE);
	return;
}

void reveal_cursor() {
	write(1, VISIBLE_CURSOR_SEQUENCE, SIZE_INVISIBLE_CURSOR_SEQUENCE);
	return;
}

void set_cursor_up() {
	write(1, UP_CURSOR_SEQUENCE, SIZE_UP_CURSOR_SEQUENCE);
	return;
}

void set_cursor_down() {
	write(1, DOWN_CURSOR_SEQUENCE, SIZE_DOWN_CURSOR_SEQUENCE);
	return;
}

void set_cursor_left() {
	write(1, LEFT_CURSOR_SEQUENCE, SIZE_LEFT_CURSOR_SEQUENCE);
	return;
}

void set_cursor_right() {
	write(1, RIGHT_CURSOR_SEQUENCE, SIZE_RIGHT_CURSOR_SEQUENCE);
	return;
}

void default_x_offset() {
	for (int i = 0; i < DEFAULT_X_OFFSET; i++) {
		set_cursor_right();
	}
}

void default_y_offset() {
	for (int i = 0; i < DEFAULT_Y_OFFSET; i++) {
		set_cursor_down();
	}
}

void set_cursor_next_line() {
	write(1, NEXT_LINE_CURSOR_SEQUENCE, SIZE_NEXT_LINE_CURSOR_SEQUENCE);
	default_x_offset();
	return;
}

void set_cursor_prev_line() {
	write(1, PREV_LINE_CURSOR_SEQUENCE, SIZE_PREV_LINE_CURSOR_SEQUENCE);
	default_x_offset();
	return;
}

void reset_cursor() {
	write(1, RESET_CURSOR_SEQUENCE, SIZE_RESET_CURSOR_SEQUENCE);
	default_x_offset();
	default_y_offset();
	return;
}

void write_blank_screen() {
	reset_cursor();
	char * clear_char = CLEAR_CHARACTER;
	for (int i = 0; i < SCREEN_HEIGHT; i++) {
		for (int j = 0; j < SCREEN_WIDTH; j++) {
			write(1, clear_char, SIZE_CLEAR_CHARACTER);
		}
	    set_cursor_next_line();
	}
	reset_cursor();
}

void write_to_stat_pane(const char * text, unsigned short int row) {
	reset_cursor();
	for (int i = 0; i < SCREEN_WIDTH + 1; i++) {
		set_cursor_right();
	}
	for (int i = 0; i < row; i++) {
		set_cursor_down();
	}

	for (int i = 0; i < MAX_STAT_WIDTH; i++) {
		if (text[i] == 0) {
			write(1, text, i);
			return;
		}
	}

}

void wait_for_input(int * c) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	struct timeval timeout = {0, 500};
	int ready = select(1, &fds, NULL, NULL, &timeout);
	if (ready) {
		ssize_t read_amount = read(0, c, 1);
		if (!read_amount) {
			*c = 0;
		}
	} else {
		*c = 0;
	}
}


const unsigned char inputMap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                         '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

int keymap[256] = {0};
void fill_keymap_from_input_map() {
	for (int i = 0; i < sizeof(keymap) / sizeof(*keymap); i++) {
		keymap[i] = -1;
	}
	for (int i = 0; i < sizeof(inputMap)  / sizeof(*inputMap); i++) {
		keymap[inputMap[i]] = i;
	}
}

char program_name[] = "Press Ctrl+m to exit";

const unsigned char font[] = {
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

typedef struct {
	unsigned short int sp;
	unsigned short int mem[STACK_LIMIT];
} StackMemory;

typedef struct {
	unsigned char reg[REGISTER_COUNT];
} RegisterMemory;

typedef struct {
	unsigned char mem[MEMORY_LIMIT];
} RandomAccessMemory;

typedef struct {
	unsigned char mem[SCREEN_COUNT];
} ScreenMemory;

typedef struct {
	char error[MAX_STAT_WIDTH];
	unsigned int cycles;
	clock_t cycleTime;
	double cycleTimeDelta;
	unsigned short int pc;
	unsigned short int regI;
	StackMemory stack;
	RegisterMemory registers;
	RandomAccessMemory ram;
	ScreenMemory screen;
	unsigned char delayTimer;
	unsigned char soundTimer;
	int keyBuffer;
} Machine;

int serialize_machine(Machine * machine) {
	int result = mkdir("spn", 0777);

	FILE * machineFile = fopen("spn/m.ch8.bin", "w");
	if (machineFile == 0) {
		return 2;
	}
	
	unsigned long int amount = fwrite(machine, 1, sizeof(*machine), machineFile);
	if (amount != sizeof(*machine)) {
		return 3;
	}

	fclose(machineFile);
	return 0;
}

int deserialize_machine(Machine * machine) {
	FILE * machineFile = fopen("spn/m.ch8.bin", "r");
	if (machineFile == 0) {
		return 1;
	}

	unsigned long int amount = fread(machine, 1, sizeof(*machine), machineFile);
	if (amount != sizeof(*machine)) {
		return 2;
	}

	fclose(machineFile);
	return 0;
}

int initialize_program_name(Machine * machine, const char * name, ssize_t size) {
	if (size > MAX_STAT_WIDTH) {
		perror("Size for program name cannot be larger than the maximum stat width.");
		return 1;
	}
	void * dest = &machine->ram.mem + NAME_MEMORY_SECTOR;
	memcpy(dest, name, size);
	return 0;
}

void initialize_font_memory(Machine * machine, const unsigned char * font_start, ssize_t size) {
	void * dest = machine->ram.mem + FONT_MEMORY_SECTOR;
	memcpy(dest, font_start, size);
}

void initialize_machine(Machine * machine) {
	*machine->error = 0;
	machine->cycles = 0;
	machine->cycleTime = clock();
	machine->pc = PROGRAM_MEMORY_SECTOR;
	machine->stack.sp = 0;
	machine->delayTimer = 0;
	machine->soundTimer = 0;
	machine->keyBuffer = -1;

	memset(machine->ram.mem, 0, MEMORY_LIMIT);
	memset(machine->screen.mem, 1, SCREEN_COUNT);

	initialize_program_name(machine, program_name, sizeof(program_name));
	initialize_font_memory(machine, font, sizeof(font) / sizeof(*font));
	fill_keymap_from_input_map();
}

void unsafe_write_machine_error(Machine * machine, const char * restrict format, ...) {
	clear_terminal();
	va_list args;
	va_start(args, format);
	vsprintf(machine->error, format, args);
}

void vunsafe_write_machine_error(Machine * machine, const char * restrict format, va_list args) {
	clear_terminal();
	vsprintf(machine->error, format, args);
}

int p = 0;
int b = 0;
void fatal_unsafe_write_machine_error(Machine * machine, const char * restrict format, ...) {
	if (b) return;
	va_list args;
	va_start(args, format);
	vunsafe_write_machine_error(machine, format, args);
	b = 1;
}

void update_machine_time(Machine * machine) {
	machine->cycleTimeDelta = 0;
    if (machine->delayTimer > 0) machine->delayTimer--;
    if (machine->soundTimer > 0) machine->soundTimer--;
}

void draw_screen(Machine * machine) {
	reset_cursor();
	char * fill_char = FILL_CHARACTER;
	char * clear_char = CLEAR_CHARACTER;
	for (int i = 0; i < SCREEN_HEIGHT; i++) {
		for (int j = 0; j < SCREEN_WIDTH; j++) {
			char val = machine->screen.mem[j + (i * SCREEN_WIDTH)];
			if (val) {
				write(1, fill_char, SIZE_FILL_CHARACTER);
			} else {
				write(1, clear_char, SIZE_CLEAR_CHARACTER);
			}
			
		}
	    set_cursor_next_line();
	}
	reset_cursor();
}

void draw_sprite(Machine * machine, unsigned short int size, unsigned short int sprite, unsigned char x, unsigned char y) {
	unsigned char px = x % SCREEN_WIDTH;
	unsigned char py = y % SCREEN_HEIGHT;

	machine->registers.reg[15] = 0;
	for (int i = 0; i < size; i++) {
		if (py + i > SCREEN_HEIGHT - 1) break;
		unsigned char spriteData = machine->ram.mem[sprite + i];
		for (int j = 0; j < 8; j++) {
			unsigned char draw = (spriteData << j) & 0x80;
			if (px + j > SCREEN_WIDTH - 1) goto outer;
			unsigned char current = machine->screen.mem[px + j + ((py + i) * SCREEN_WIDTH)];
			
			if (draw) {
				if (current) {
					machine->screen.mem[px + j + ((py + i) * SCREEN_WIDTH)] = 0;
					machine->registers.reg[15] = 1;
				} else {
					machine->screen.mem[px + j + ((py + i) * SCREEN_WIDTH)] = 1;
				}
			}
		inner: {};
		}
	outer: {};
	}
	
	machine->screen.mem[px + (py * SCREEN_WIDTH)] = 1;
}

char currentInstructionStat[MAX_STAT_WIDTH] = {0};
void execute_instruction(Machine * machine) {
	unsigned char p1 = machine->ram.mem[machine->pc];
	unsigned char p2 = machine->ram.mem[machine->pc + 1];

	unsigned char op   = (p1 & 240) >> 4;
	unsigned char x = p1 & 15;
	unsigned char y = (p2 & 240) >> 4;
	unsigned char reg1 = machine->registers.reg[x];
	unsigned char reg2 = machine->registers.reg[y];

	unsigned short int num  = p2 & 15;
	unsigned short int nnum  = p2;
	
	unsigned short int nnnum  = ((unsigned short int)(p1 & 15)  << 8) + p2;

	sprintf(currentInstructionStat, "(%.2x%.2x @ %.4x)  %.1x(%.1x,%.1x) 0=%.2x 1=%.2x | 0x%.4x 0x%.4x 0x%.4x", p1, p2, machine->pc, op, x, y, reg1, reg2, num, nnum, nnnum);
	
	switch(op) {
	case 0:
		if (p2 == 0xe0) {
			memset(machine->screen.mem, 0, SCREEN_COUNT);
			break;
		} else if (p2 == 0xee) {
			if (machine->stack.sp == 0) {
				fatal_unsafe_write_machine_error(machine, "Error: cannot return out of an empty stack");
				return;
			}
			machine->stack.sp--;
			machine->pc = machine->stack.mem[machine->stack.sp];
			break;
		}
		
		unsafe_write_machine_error(machine, "Warning: ignored instruction 0x%.2x%2.2x", p1, p2);
		break;
	case 1:
		machine->pc = nnnum;
		return;
	case 2:
		machine->stack.mem[machine->stack.sp] = machine->pc;
		machine->stack.sp++;
		machine->pc = nnnum;
		return;
	case 3:
		if (reg1 == nnum) machine->pc += 2;
		break;
	case 4:
		if (reg1 != nnum) machine->pc += 2;
		break;
	case 5:
		if (reg1 == reg2) machine->pc += 2;
		break;
	case 6:
			machine->registers.reg[x] = nnum;
			break;
	case 7:
		machine->registers.reg[x] += nnum;
		break;
	case 8:
		switch(num) {
		case 0:
			machine->registers.reg[x] = reg2;
			break;
		case 1:
			machine->registers.reg[x] |= reg2;
			if (COSMAC_VF_RESET_CONFIGURATION) machine->registers.reg[15] = 0;
			break;
		case 2:
			machine->registers.reg[x] &= reg2;
			if (COSMAC_VF_RESET_CONFIGURATION) machine->registers.reg[15] = 0;
			break;
		case 3:
			machine->registers.reg[x] ^= reg2;
			if (COSMAC_VF_RESET_CONFIGURATION) machine->registers.reg[15] = 0;
			break;
		case 4:
			machine->registers.reg[x] = reg1 + reg2;
			if (reg1 + reg2 > 255) machine->registers.reg[15] = 1;
			else machine->registers.reg[15] = 0;
			break;
		case 5:
			machine->registers.reg[x] = reg1 - reg2;
			if (reg1 >= reg2) machine->registers.reg[15] = 1;
			else machine->registers.reg[15] = 0;
			break;
		case 6:
			if (!SUPER_CHIP_SHIFT_CONFIGURATION) {
				machine->registers.reg[x] = reg2;
				machine->registers.reg[x] >>= 1;
				machine->registers.reg[15] = reg2 & 1;
			} else {
				machine->registers.reg[x] >>= 1;
				machine->registers.reg[15] = reg1 & 1;
			}
			break;
		case 7:
			machine->registers.reg[x] = reg2 - reg1;
			if (reg2 >= reg1) machine->registers.reg[15] = 1;
			else machine->registers.reg[15] = 0;
			break;
		case 14:
			if (!SUPER_CHIP_SHIFT_CONFIGURATION) {
				machine->registers.reg[x] = reg2;
				machine->registers.reg[x] <<= 1;
				machine->registers.reg[15] = reg2 & 8;
			} else {
				machine->registers.reg[x] <<= 1;
				machine->registers.reg[15] = (reg1 & 8) >> 3;
			}
			
			break;
		default:
			fatal_unsafe_write_machine_error(machine, "Error: unknown instruction 0x%.2x%2.2x at address 0x%.4x", p1, p2, machine->pc);
			return;
		}
		break;
	case 9:
		if (reg1 != reg2) machine->pc += 2;
		break;
	case 10:
		machine->regI = nnnum;
		break;
	case 11:
		if (SUPER_CHIP_JUMP_CONFIGURATION) {
			machine->pc = nnnum + reg1;
		} else {
			machine->pc = nnnum + machine->registers.reg[0];
		}
		return;
	case 12:
		machine->registers.reg[x] = (unsigned char)machine->cycleTime & nnum;
		break;
	case 13:
		draw_sprite(machine, num, machine->regI, reg1, reg2);
		break;
	case 14:
		switch (nnum) {
		case 0x9e:
			if (machine->keyBuffer == (reg1 & 15)) {
					machine->pc += 2;
					machine->keyBuffer = -1;
				}
			break;
		case 0xa1:
			if (machine->keyBuffer != (reg1 & 15)) {
				machine->pc += 2;
				machine->keyBuffer = -1;
			}
			machine->keyBuffer = -1;
			break;
		default:
			fatal_unsafe_write_machine_error(machine, "Error: unknown instruction 0x%.2x%2.2x at address 0x%.4x", p1, p2, machine->pc);
			return;
		}
		break;
	case 15:
		switch (nnum) {
		case 0x07:
			machine->registers.reg[x] = machine->delayTimer;
			break;
		case 0x15:
			machine->delayTimer = machine->registers.reg[x];
			break;
		case 0x18:
			machine->soundTimer = machine->registers.reg[x];
			break;
		case 0x1e:
			machine->regI += reg1;
			if (AMIGA_INDEX_OVERFLOW_CONFIGURATION) {
				if (machine->regI > 0x1000) {
					machine->registers.reg[15] = 1;
				} else machine->registers.reg[15] = 0;
			}
			break;
		case 0x0a:
			if (machine->keyBuffer > -1) {
				machine->registers.reg[x] = machine->keyBuffer;
				machine->keyBuffer = -1;
			} else machine->pc -= 2;
			break;
		case 0x29:
			machine->regI = FONT_MEMORY_SECTOR + (reg1 * 5);
			break;
		case 0x33:
			machine->ram.mem[machine->regI + 2] = reg1 % 10;
			machine->ram.mem[machine->regI + 1] = (reg1 / 10) % 10;
			machine->ram.mem[machine->regI] = (reg1 / 100);
			break;
		case 0x55:
			for (int i = 0; i < x + 1; i++) {
				machine->ram.mem[machine->regI + i] = machine->registers.reg[i];
			}
			if (COSMAC_INDEX_INCREMENT_CONFIGURATION) {
				machine->regI += x + 1;
			}
			break;
		case 0x65:
			for (int i = 0; i < x + 1; i++) {
				machine->registers.reg[i] = machine->ram.mem[machine->regI + i];	
			}
			if (COSMAC_INDEX_INCREMENT_CONFIGURATION) {
				machine->regI += x + 1;
			}
			break;
		default:
			fatal_unsafe_write_machine_error(machine, "Error: unknown instruction 0x%.2x%2.2x at address 0x%.4x", p1, p2, machine->pc);
			return;
		}
		break;
	default:
		fatal_unsafe_write_machine_error(machine, "Error: unknown instruction 0x%.2x%2.2x at address 0x%.4x", p1, p2, machine->pc);
		return;
	}

	machine->pc += 2;
}


void debug_first_4_program_instruction(Machine * machine) {
	unsafe_write_machine_error(machine, "%d %d %d %d %d %d %d %d",
							   machine->ram.mem[PROGRAM_MEMORY_SECTOR],
							   machine->ram.mem[PROGRAM_MEMORY_SECTOR + 1],
							   machine->ram.mem[PROGRAM_MEMORY_SECTOR + 2],
							   machine->ram.mem[PROGRAM_MEMORY_SECTOR + 3],
							   machine->ram.mem[PROGRAM_MEMORY_SECTOR + 4],
							   machine->ram.mem[PROGRAM_MEMORY_SECTOR + 5],
							   machine->ram.mem[PROGRAM_MEMORY_SECTOR + 6],
							   machine->ram.mem[PROGRAM_MEMORY_SECTOR + 7]);
}

int main(int argc, char * argv[]) {
	FILE * stdout = fdopen(1, "w");
	char * screenBuffer = malloc(SCREEN_BUFFER_SIZE);
	if (setvbuf(stdout, screenBuffer, _IOFBF, SCREEN_BUFFER_SIZE)) {
		perror("Cannot honor our buffering scheme");
		return 3;
	}
	
	Machine machine;
	

	// debug buffers
	char frameTimeStat[MAX_STAT_WIDTH];
	char framesPerSecondStat[MAX_STAT_WIDTH];
	char lastInputStat[MAX_STAT_WIDTH];
	char keyBufferStat[MAX_STAT_WIDTH];
	char programCounterStat[MAX_STAT_WIDTH];
	char soundTimerStat[MAX_STAT_WIDTH];
	char currentCycleStat[MAX_STAT_WIDTH];

	sprintf(lastInputStat, "Code of Last Input: %03d", 0);
	sprintf(keyBufferStat, "Key Buffer: %02d", -1);

	initialize_machine(&machine);

	if (argc > 1) {
		const char * filename = argv[1];
		FILE * programFile = fopen(filename, "rb");
		if (!programFile) {
			perror("Could not read program file");
			return 4;
		}

		fseek(programFile, 0, SEEK_END);
		long int programSize = ftell(programFile);
		rewind(programFile);

		if (programSize > MEMORY_LIMIT - PROGRAM_MEMORY_SECTOR) {
			perror("Program too large");
			return 5;
		}

		long amount = fread(&machine.ram.mem[PROGRAM_MEMORY_SECTOR], 1, programSize, programFile);
		fclose(programFile);
	}

	enable_raw_mode();

	
	hide_cursor();
	clear_terminal();
	write_blank_screen();

	while (1) {
		machine.cycleTimeDelta += clock() - machine.cycleTime;
		machine.cycleTime = clock();

		double secSinceLastEcycle = machine.cycleTimeDelta / CLOCKS_PER_SEC;
		double framesPerSecond = 1 / secSinceLastEcycle;
		sprintf(frameTimeStat, "Frame Time:  %10.6lf", secSinceLastEcycle);
		sprintf(framesPerSecondStat, "Frames per Second: %4.0lf", framesPerSecond);

		if (machine.cycleTimeDelta < FRAME_TIME) {
			continue;
		}

		draw_screen(&machine);


		int input = 0;
		int key = -1;
		wait_for_input(&input);
		key = keymap[input];
		if (input) {
			sprintf(lastInputStat, "Code of Last Input: %03d", input);
		}

		if (machine.keyBuffer == -1) {
			machine.keyBuffer = key;
		}

		sprintf(keyBufferStat, "Key Buffer: %02d", machine.keyBuffer);

		if (input == 13) break;
		if (input == 8) p = !p;
		if (input == 9) {
			int result = serialize_machine(&machine);
			if (result > 0) {
				perror("Serialization error");
				return result;
			}
			unsafe_write_machine_error(&machine, "Warning: wrote machine state to file");
			p = PAUSE_ON_SAVE_MACHINE | p;
		}
		if (input == 67) {
			int result = deserialize_machine(&machine);
			if (result > 0) {
				perror("Deserialization error");
				return result;
			}
			unsafe_write_machine_error(&machine, "Warning: loaded machine from file!");
			p = !UNPAUSE_ON_LOAD_MACHINE & p;
		}

		if (p) {
			write_to_stat_pane("PAUSED", 1);
		} else {
			if (b) continue;
			
			write_to_stat_pane("      ", 1);
			
			machine.cycles += 1;
			sprintf(currentCycleStat, "Current Cycle: %d", machine.cycles);
			update_machine_time(&machine);

			sprintf(programCounterStat, "Program Counter: %d 0x%.4x", machine.pc, machine.pc);
			sprintf(soundTimerStat, "%-*.*s", MAX_STAT_WIDTH - 1, machine.soundTimer, SOUND_VOLUME_SEQUENCE);
			execute_instruction(&machine);

			write_to_stat_pane(frameTimeStat, 3);
			write_to_stat_pane(framesPerSecondStat, 4);
		}

		write_to_stat_pane(machine.error, 0);
		write_to_stat_pane((char *)machine.ram.mem + NAME_MEMORY_SECTOR, 2);

		write_to_stat_pane(lastInputStat, 5);
		write_to_stat_pane(keyBufferStat, 6);
		write_to_stat_pane(programCounterStat, 7);
		write_to_stat_pane(currentInstructionStat, 8);
		write_to_stat_pane(currentCycleStat, 9);

		write_to_stat_pane(soundTimerStat, 12);
		
		fflush(0);
	}

	free(screenBuffer);
	reveal_cursor();
	disable_raw_mode();
	clear_terminal();
	return 0;
}
