
# Shell
SHELL = /bin/bash

# Directories
BSP_ROOT_DIR = bsp
TOOLS_PATH = /usr
EXTRA_TOOLS_PATH = tools

# Platform details
SRAM_BASE = 0x00400000
SERIAL_PORT = /dev/ttyUSB1
BAUDRATE = 115200

# Putty terminal
TERMINAL = putty -serial -sercfg $(BAUDRATE) $(SERIAL_PORT)

# Crosscompiler configuration
TOOLS_PREFIX = arm-none-eabi
CROSS_COMPILE = $(TOOLS_PATH)/bin/$(TOOLS_PREFIX)-
AS = $(CROSS_COMPILE)as
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OPENOCD = $(TOOLS_PATH)/bin/openocd
ARCHIVOSOPENOCD = /usr/share/openocd/scripts/

# Additional tools
MC1322X_LOAD = $(EXTRA_TOOLS_PATH)/bin/mc1322x-load
FLASHER = $(EXTRA_TOOLS_PATH)/flasher_redbee-econotag.bin
BBMC = $(EXTRA_TOOLS_PATH)/bin/bbmc

# Flags
ASFLAGS = -gstabs -mcpu=arm7tdmi -mfpu=softfpa
CFLAGS = -c -g -Wall -mcpu=arm7tdmi -std=gnu89
LDFLAGS = -nostartfiles


# Application files
PROGNAME = hello
OBJ = $(PROGNAME).o
ELF = $(PROGNAME).elf
BIN = $(PROGNAME).bin

# Add the BSP makefile and compilation flags
include $(BSP_ROOT_DIR)/bsp.mk
CFLAGS += $(BSP_CFLAGS)
LDFLAGS += $(BSP_LDFLAGS)
LIBS += $(BSP_LIBS)

# Application building rules
.PHONY: all
all: $(ELF) $(BIN)

$(ELF): $(OBJ) $(BSP_ROOT_DIR)/$(BSP_LIB) $(BSP_LINKER_SCRIPT)
	@echo "Linking $@."
	$(LD) $(LDFLAGS) $< -o $@ $(LIBS)

$(BIN): $(ELF)
	@echo "Generating $@."
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	@echo "Compiling $@."
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	@echo "Assembling $@."
	$(AS) $(ASFLAGS) $< -o $@

# BSP building rules
$(BSP_ROOT_DIR)/$(BSP_LIB):
	@echo "Building the BSP library"
	@make -C $(BSP_ROOT_DIR)

.PHONY: bsp
bsp: $(BSP_ROOT_DIR)/$(BSP_LIB)

# Clean the BSP
.PHONY: clean-bsp
clean-bsp:
	@make --no-print-directory -C $(BSP_ROOT_DIR) clean

# Stop the board
.PHONY: halt
halt: check-openocd
	@echo "Stopping the CPU."
	@echo -e "halt" | nc -i 1 localhost 4444 > /dev/null

# Execution with OpenOCD
.PHONY: run
run: $(BIN) check-openocd
	@echo "Executing the program."
	@echo -e "soft_reset_halt\n load_image $< $(SRAM_BASE)\n resume $(SRAM_BASE)" | nc -i 1 localhost 4444  > /dev/null

# Execution with mc1322x-load.pl
$(SERIAL_PORT):
	@echo "Please, connect the board."
	@false

$(MC1322X_LOAD): $(EXTRA_TOOLS_PATH)/mc1322x-load
	@echo "Building mc1322x_load."
	@make -C $< install

$(BBMC): $(EXTRA_TOOLS_PATH)/bbmc
	@echo "Building bbmc."
	@make -C $< install

.PHONY: run2
run2: $(BIN) $(MC1322X_LOAD) $(SERIAL_PORT)
	@echo "Executing the program."
	@$(MC1322X_LOAD) -f $(BIN) -t $(SERIAL_PORT)

# Record the image in the flash memory
.PHONY: flash
flash: $(BIN) $(MC1322X_LOAD) $(FLASHER) $(SERIAL_PORT)
	@echo "Recording the image in the board flash memory."
	@$(MC1322X_LOAD) -f $(FLASHER) -s $(BIN) -t $(SERIAL_PORT)

# Clean the image
.PHONY: erase
erase: $(BIN) $(BBMC) $(SERIAL_PORT)
	@echo "Cleaning the board image."
	@$(BBMC) -l redbee-econotag erase

# Serial terminal
.PHONY: term
term: $(SERIAL_PORT)
	@echo "Opening the serial terminal."
	@$(TERMINAL) &

# Debugging
.PHONY: openocd
openocd:
	@echo "Opening openocd."
	@xterm -e "sudo $(OPENOCD) -f $(ARCHIVOSOPENOCD)interface/ftdi/redbee-econotag.cfg -f $(ARCHIVOSOPENOCD)board/redbee.cfg" &
	@sleep 1

.PHONY: check-openocd
check-openocd:
	@if [ ! `pgrep openocd` ]; then make -s openocd; fi

.PHONY: openocd-term
openocd-term: check-openocd
	@echo "Opening openocd terminal."
	@xterm -e "telnet localhost 4444" &

# Cleaning
.PHONY: clean
clean:
	@echo "Clean the application."
	@rm -rf $(BIN) $(ELF) $(OBJ) *~
