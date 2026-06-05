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
        PLATFORM_FLAG="-DPLATFORM_QEMU -mcpu=arm926ej-s"
    else
        echo "Using BeagleBone Black platform..."
        FOLDER="beagle"
        RUN_FLAGS="-M beagle -nographic"
        PLATFORM_FLAG="-DPLATFORM_BEAGLE -mcpu=cortex-a8"
    fi
else
    echo "No platform specified, defaulting to BeagleBone Black..."
    FOLDER="beagle"
    RUN_FLAGS="-M beagle -nographic"
    PLATFORM_FLAG="-DPLATFORM_BEAGLE -mcpu=cortex-a8"
fi

# Remove previous compiled objects and binaries
echo "Cleaning up previous build files..."
rm -f bin/*.o bin/*.elf bin/*.bin
mkdir -p bin

echo "Assembling os/root.s..."
arm-none-eabi-as -o bin/root.o root/$FOLDER/root.s
arm-none-eabi-as -o bin/start.o program/program_start.s

echo "Compiling all relevant C files..."
declare -A FILES=(
    ["os/os.c"]="os"
    ["os/$FOLDER/uart.c"]="uart"
    ["os/$FOLDER/timer.c"]="timer"
    ["os/$FOLDER/svc.c"]="svc"
    ["os/$FOLDER/watchdog.c"]="watchdog"
    ["libraries/io.c"]="io"
    ["libraries/io_common.c"]="io_common"
    ["libraries/time.c"]="time"
    ["program/p1/main.c"]="p1"
    ["program/p2/main.c"]="p2"
    ["program/p3/main.c"]="p3"
    ["os/pcb.c"]="pcb"
    ["os/scheduler.c"]="scheduler"
    ["os/interrupts.c"]="interrupts"
    ["os/os_io.c"]="os_io"
    ["os/fault.c"]="fault"
)

COMMON_CFLAGS="-g -c -ffreestanding -nostdlib -nostartfiles -Wall -O1 -I os -I libraries $PLATFORM_FLAG"

for FILE in "${!FILES[@]}"; do
    OUT="${FILES[$FILE]}"
    echo "Compiling $FILE -> bin/$OUT.o..."
    arm-none-eabi-gcc $COMMON_CFLAGS \
        -o bin/$OUT.o $FILE
done

declare -A P_TO_LINK=(
    ["bin/p1.o"]="program/$FOLDER/linker_p1.ld"
    ["bin/p2.o"]="program/$FOLDER/linker_p2.ld"
    ["bin/p3.o"]="program/$FOLDER/linker_p3.ld"
)

echo "Linking all relevant object files for P1, P2 and P3..."
for OBJ in "${!P_TO_LINK[@]}"; do
    echo "Linking: $OBJ for ${P_TO_LINK[$OBJ]}"
    arm-none-eabi-gcc -nostdlib -nostartfiles -T "${P_TO_LINK[$OBJ]}" \
        -o "${OBJ%.o}.elf" \
        bin/start.o bin/io.o bin/io_common.o bin/time.o $OBJ \
        -lgcc
done

declare -A OS_TO_LINK=(
    ["bin/os.o"]="os/$FOLDER/linker.ld"
)

echo "Linking all relevant object files for OS..."
for OBJ in "${!OS_TO_LINK[@]}"; do
    echo "Linking: $OBJ for ${OS_TO_LINK[$OBJ]}"
    arm-none-eabi-gcc -nostdlib -nostartfiles -T "${OS_TO_LINK[$OBJ]}" \
        -o "${OBJ%.o}.elf" \
        bin/root.o bin/os_io.o bin/io_common.o bin/time.o bin/uart.o bin/pcb.o bin/scheduler.o bin/timer.o bin/svc.o bin/watchdog.o bin/interrupts.o $OBJ \
        -lgcc
done

echo "Converting ELFs to binary..."
for ELF in bin/*.elf; do
    arm-none-eabi-objcopy -O binary "$ELF" "${ELF%.elf}.bin"
done

echo "Build finished"

if [ "$FOLDER" == "qemu" ]; then
    echo "Running OS image on QEMU."
    qemu-system-arm $RUN_FLAGS -kernel bin/os.elf -device driver=loader,file=bin/p1.elf -device driver=loader,file=bin/p2.elf -device driver=loader,file=bin/p3.elf
fi
