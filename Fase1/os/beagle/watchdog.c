#include "watchdog.h"

// Watchdog (WDT1) addresses and registers on AM335x
#define WDT1_BASE       0x44E35000u
#define WDT_WSPR        (WDT1_BASE + 0x048u)  // Watchdog Start/Stop Reg
#define WDT_WWPS        (WDT1_BASE + 0x034u)  // Watchdog Write Posting Status

void watchdog_disable(void) {
    PUT32(WDT_WSPR, 0xAAAA);
    while (GET32(WDT_WWPS) & (1 << 4));  // wait for W_PEND_WSPR only

    PUT32(WDT_WSPR, 0x5555);
    while (GET32(WDT_WWPS) & (1 << 4));  // wait again
}