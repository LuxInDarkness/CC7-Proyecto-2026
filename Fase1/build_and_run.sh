#!/bin/bash

# Build and Run Full Proyect on either QEMU or real hardware (BeagleBone Black)

# Exit immediately if a command exits with a non-zero status
set -e

# Run from script directory so paths work from anywhere
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Determine platform based on argument
if [ "$#" -ne "0" ]; then
    if [ "$1" == "1" ]; then
        echo "Using Qemu platform..."
        FOLDER="qemu"
        RUN_FLAGS="-M versatilepb -nographic -gdb tcp::5000"
    else
        echo "Using BeagleBone Black platform..."
        FOLDER="beagle"
        RUN_FLAGS="-M beagle -nographic"
    fi
else
    echo "No platform specified, defaulting to BeagleBone Black..."
    FOLDER="beagle"
    RUN_FLAGS="-M beagle -nographic"
fi

# Remove previous compiled objects and binaries
echo "Cleaning up previous build files..."
rm -f bin/*.o bin/program.elf bin/program.bin
mkdir -p bin

echo "Assembling os/root.s..."
arm-none-eabi-as -o bin/root.o root/$FOLDER/root.s

echo "Compiling uart library..."
arm-none-eabi-gcc -g -c \
    -ffreestanding -nostdlib -nostartfiles \
    -Wall -O1 \
    -I os \
    -o bin/uart.o os/$FOLDER/uart.c

echo "Compiling IO library..."
arm-none-eabi-gcc -g -c \
    -ffreestanding -nostdlib -nostartfiles \
    -Wall -O1 \
    -I libraries \
    -o bin/io.o libraries/io.c

echo "Compiling time library..."
arm-none-eabi-gcc -g -c \
    -ffreestanding -nostdlib -nostartfiles \
    -Wall -O1 \
    -I libraries \
    -o bin/time.o libraries/time.c

echo "Compiling program/p1/main.c..."
arm-none-eabi-gcc -g -c \
    -ffreestanding -nostdlib -nostartfiles \
    -Wall -O1 \
    -I program/p1 \
    -I libraries \
    -o bin/p1.o program/p1/main.c

echo "Compiling program/p2/main.c..."
arm-none-eabi-gcc -g -c \
    -ffreestanding -nostdlib -nostartfiles \
    -Wall -O1 \
    -I program/p2 \
    -I libraries \
    -o bin/p2.o program/p2/main.c

echo "Compiling os/$FOLDER/os.c..."
arm-none-eabi-gcc -g -c \
    -ffreestanding -nostdlib -nostartfiles \
    -Wall -O1 \
    -I os \
    -o bin/os.o os/$FOLDER/os.c

echo "Linking object files for p1 compilation..."
arm-none-eabi-gcc -nostartfiles -T "program/$FOLDER/linker_p1.ld" \
    -o bin/program_p1.elf \
    bin/root.o bin/p1.o bin/io.o bin/time.o bin/uart.o bin/os.o

echo "Linking object files for p2 compilation..."
arm-none-eabi-gcc -nostartfiles -T "program/$FOLDER/linker_p2.ld" \
    -o bin/program_p2.elf \
    bin/root.o bin/p2.o bin/io.o bin/time.o bin/uart.o bin/os.o

echo "Converting ELF to binary..."
arm-none-eabi-objcopy -O binary bin/program_p2.elf bin/program_p2.bin

echo "Running Program..."
qemu-system-arm $RUN_FLAGS -kernel bin/program_p2.elf