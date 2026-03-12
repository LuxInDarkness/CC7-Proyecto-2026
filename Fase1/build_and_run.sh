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

echo "Compiling all relevant C files..."
declare -A FILES=(
    ["os/$FOLDER/os.c"]="os"
    ["os/$FOLDER/uart.c"]="uart"
    ["libraries/io.c"]="io"
    ["libraries/time.c"]="time"
    ["program/p1/main.c"]="p1"
    ["program/p2/main.c"]="p2"
)

for FILE in "${!FILES[@]}"; do
    OUT="${FILES[$FILE]}"
    echo "Compiling $FILE -> bin/$OUT.o..."
    arm-none-eabi-gcc -g -c \
        -ffreestanding -nostdlib -nostartfiles \
        -Wall -O1 \
        -I os \
        -I libraries \
        -o bin/$OUT.o $FILE
done

declare -A TO_LINK=(
    ["bin/p1.o"]="program/$FOLDER/linker_p1.ld"
    ["bin/p2.o"]="program/$FOLDER/linker_p2.ld"
    ["bin/os.o"]="os/$FOLDER/linker.ld"
)

echo "Linking all relevant object files..."
for OBJ in "${!TO_LINK[@]}"; do
    echo "Linking: $OBJ for ${TO_LINK[$OBJ]}"
    arm-none-eabi-gcc -nostartfiles -T "${TO_LINK[$OBJ]}" \
        -o "${OBJ%.o}.elf" \
        bin/root.o bin/io.o bin/time.o bin/uart.o $OBJ
done

echo "Converting ELFs to binary..."
for ELF in bin/*.elf; do
    arm-none-eabi-objcopy -O binary "$ELF" "${ELF%.elf}.bin"
done

echo "Running Program..."
qemu-system-arm $RUN_FLAGS -kernel bin/p2.elf