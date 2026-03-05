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
        echo "Using VersatilePB platform..."
        LINKER="linker_vpb.ld"
        VPB_FLAG="-D PLATFORM_VERSATILEPB"
        RUN_FLAGS="-M versatilepb -nographic -gdb tcp::5000"
    else
        echo "Using default platform (BeagleBone Black)..."
        LINKER="linker_bbb.ld"
        VPB_FLAG=""
        RUN_FLAGS="-M beagle -nographic"
    fi
else
    echo "No platform specified, defaulting to BeagleBone Black..."
    LINKER="linker_bbb.ld"
    VPB_FLAG=""
    RUN_FLAGS="-M beagle -nographic"
fi

# Remove previous compiled objects and binaries
echo "Cleaning up previous build files..."
rm -f bin/*.o bin/program.elf bin/program.bin
mkdir -p bin

echo "Assembling os/root.s..."
arm-none-eabi-as -o bin/root.o os/root.s

echo "Compiling os/os.c..."
arm-none-eabi-gcc -g -c \
    $VPB_FLAG \
    -ffreestanding -nostdlib -nostartfiles \
    -Wall -O2 \
    -I os \
    -o bin/os.o os/os.c

echo "Linking object files..."
arm-none-eabi-gcc -nostartfiles -T $LINKER \
    -o bin/program.elf \
    bin/root.o bin/os.o

echo "Converting ELF to binary..."
arm-none-eabi-objcopy -O binary bin/program.elf bin/program.bin

echo "Running Program..."
qemu-system-arm $RUN_FLAGS -kernel bin/program.elf