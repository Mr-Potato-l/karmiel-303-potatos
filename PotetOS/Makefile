TARGET = myos.bin
ISO = myos.iso

CC = i686-elf-gcc
AS = i686-elf-as
NASM = nasm

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -T src/linker.ld -ffreestanding -O2 -nostdlib

SRC = src

# --- Automatically detect all source files ---
C_SOURCES   = $(wildcard $(SRC)/*.c)
S_SOURCES   = $(wildcard $(SRC)/*.s)
ASM_SOURCES = $(wildcard $(SRC)/*.asm)

# --- Convert them to .o names ---
OBJECTS = \
    $(C_SOURCES:.c=.o) \
    $(S_SOURCES:.s=.o) \
    $(ASM_SOURCES:.asm=.o)

all: run

run: iso
	qemu-system-i386 -cdrom $(ISO)

iso: $(SRC)/$(TARGET)
	mv $(SRC)/$(TARGET) isodir/boot/$(TARGET)
	grub-mkrescue -o $(ISO) isodir > grub_log.txt 2>&1
	@echo "ISO created!"

$(SRC)/$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(SRC)/$(TARGET) $(OBJECTS) -lgcc
	@echo "Kernel linked!"

# --- Assembly .s files ---
$(SRC)/%.o: $(SRC)/%.s
	$(AS) $< -o $@

# --- NASM .asm files ---
$(SRC)/%.o: $(SRC)/%.asm
	$(NASM) -f elf32 $< -o $@

# --- C files ---
$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRC)/*.o
	rm -f $(SRC)/$(TARGET)
	@echo "Cleaned!"

.PHONY: all clean run iso

