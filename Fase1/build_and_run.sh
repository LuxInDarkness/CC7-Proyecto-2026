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
rm -f bin/*.o bin/*.elf bin/*.bin
mkdir -p bin

COMMON_CFLAGS="-g -c -ffreestanding -nostdlib -nostartfiles -Wall -O1 -I os -I libraries"

echo "Building OS image..."
arm-none-eabi-as -o bin/os_root.o root/$FOLDER/root.s
arm-none-eabi-gcc $COMMON_CFLAGS -o bin/os.o os/$FOLDER/os.c
arm-none-eabi-gcc $COMMON_CFLAGS -o bin/os_uart.o os/$FOLDER/uart.c
arm-none-eabi-gcc $COMMON_CFLAGS -o bin/os_io.o libraries/io.c
arm-none-eabi-gcc $COMMON_CFLAGS -o bin/os_time.o libraries/time.c
arm-none-eabi-gcc $COMMON_CFLAGS -o bin/os_pcb.o os/pcb.c

arm-none-eabi-gcc -nostartfiles -T "os/$FOLDER/linker.ld" \
    -o bin/os.elf \
    bin/os_root.o bin/os_io.o bin/os_time.o bin/os_uart.o bin/os_pcb.o bin/os.o
arm-none-eabi-objcopy -O binary bin/os.elf bin/os.bin

echo "Building P1 image..."
arm-none-eabi-as -o bin/p1_start.o program/program_start.s
arm-none-eabi-as -o bin/p1_mmio.o libraries/mmio.s
arm-none-eabi-gcc $COMMON_CFLAGS -o bin/p1_main.o program/p1/main.c
arm-none-eabi-gcc $COMMON_CFLAGS -o bin/p1_uart.o os/$FOLDER/uart.c
arm-none-eabi-gcc $COMMON_CFLAGS -o bin/p1_io.o libraries/io.c
arm-none-eabi-gcc $COMMON_CFLAGS -o bin/p1_time.o libraries/time.c

arm-none-eabi-gcc -nostartfiles -T "program/$FOLDER/linker_p1.ld" \
    -o bin/p1.elf \
    bin/p1_start.o bin/p1_mmio.o bin/p1_main.o bin/p1_uart.o bin/p1_io.o bin/p1_time.o
arm-none-eabi-objcopy -O binary bin/p1.elf bin/p1.bin

echo "Building P2 image..."
arm-none-eabi-as -o bin/p2_start.o program/program_start.s
arm-none-eabi-as -o bin/p2_mmio.o libraries/mmio.s
arm-none-eabi-gcc $COMMON_CFLAGS -o bin/p2_main.o program/p2/main.c
arm-none-eabi-gcc $COMMON_CFLAGS -o bin/p2_uart.o os/$FOLDER/uart.c
arm-none-eabi-gcc $COMMON_CFLAGS -o bin/p2_io.o libraries/io.c
arm-none-eabi-gcc $COMMON_CFLAGS -o bin/p2_time.o libraries/time.c

arm-none-eabi-gcc -nostartfiles -T "program/$FOLDER/linker_p2.ld" \
    -o bin/p2.elf \
    bin/p2_start.o bin/p2_mmio.o bin/p2_main.o bin/p2_uart.o bin/p2_io.o bin/p2_time.o
arm-none-eabi-objcopy -O binary bin/p2.elf bin/p2.bin

echo "Build finished"

if [ "$FOLDER" == "qemu" ]; then
    echo "Running OS image on QEMU."
    qemu-system-arm $RUN_FLAGS -kernel bin/os.elf
fi