# Multi-Process OS: Build and Load Guide

This README provides the exact instructions to build, load, and execute the Operating System along with Process 1 (P1), Process 2(P2) and Process 3 (P3) via U-Boot.

## 1. Memory Map & Base Addresses

Before building, ensure that your linker scripts are configured to use the following exact base addresses:

| Component | Binary Name | Base Address |
| :--- | :--- | :--- |
| **OS** | `os.bin` | `0x82000000` |
| **Process 1** | `p1.bin` | `0x82100000` |
| **Process 2** | `p2.bin` | `0x82200000` |
| **Process 3** | `p3.bin` | `0x82300000` |

---

## 2. Build Instructions

The system is composed of three separate binaries. You must compile and link them independently:

1. **OS Binary:** Compile and link the operating system. It must be linked exactly at the OS base address (`0x82000000`).
2. **Process Binaries:** Compile and link P1 and P2 into two separate binaries. Link P1 at `0x82100000`, P2 at `0x82200000` and P3 at `0x82300000`.

---

## 3. Loading via U-Boot

Use the `loady` command (YMODEM protocol) over your serial connection to transfer the binaries to the target board's memory. Wait for the U-Boot prompt to start the YMODEM transfer in your terminal emulator.

```bash
# Step 1: Load the OS image
=> loady 0x82000000

# Step 2: Load the Process 1 image
=> loady 0x82100000

# Step 3: Load the Process 2 image
=> loady 0x82200000

# Step 4: Load the Process 3 image
=> loady 0x82300000

# Step 5: Go OS
=> go 0x82000000
