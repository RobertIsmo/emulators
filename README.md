# Emulators

THis repo collects multiple different emulators I have implemented across many different languages.

## Usage

### CHIP-8

To build the C sources CHIP-8 emulator use zig.

```zig build -Dcbuild -Dchip8```

To run simply 

```./zig-out/bin/CHIP-8_c```

which will halt due having no program. Use

```./zig-out/bin/CHIP-8_c chip8/programs/<program file name>.ch8```

to run a program.

#### Controls

Inputs `0-F` are their keyboard match. `Ctrl+m` or `Enter` to exit. `Ctrl+p` to pause/unpause. `Tab` to save the current emulation state in the `spn` directory. `Ctrl+l` to load the saved state.

#### Configuration

`chip8/src/main.c` has a variety of macros to control quirks and other configurations.

*Main Configurations*:


**PAUSE_ON_SAVE_MACHINE** *default 1*

**UNPAUSE_ON_LOAD_MACHINE** *default 1*

**SUPER_CHIP_SHIFT_CONFIGURATION** *default 1*

**SUPER_CHIP_JUMP_CONFIGURATION** *default 1*

**AMIGA_INDEX_OVERFLOW_CONFIGURATION** *default 1*

**COSMAC_INDEX_INCREMENT_CONFIGURATION** *default 0*

**COSMAC_VF_RESET_CONFIGURATION** *default 0*
