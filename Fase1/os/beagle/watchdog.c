#include "watchdog.h"

// Watchdog (WDT1) addresses and registers on AM335x
#define WDT1_BASE       0x44E35000u
#define WDT_WSPR        (WDT1_BASE + 0x048u)  // Watchdog Start/Stop Reg
#define WDT_WWPS        (WDT1_BASE + 0x034u)  // Watchdog Write Posting Status

void watchdog_disable(void) {
    // Disable watchdog timer (WDT1) on AM335x
    // Sequence: Write 0xAAAA to WDT_WSPR, wait for WWPS, then write 0x5555
    
    // First disable sequence
    PUT32(WDT_WSPR, 0xAAAA);
    // Wait for write posting to complete
    while (GET32(WDT_WWPS) != 0);
    
    // Second disable sequence
    PUT32(WDT_WSPR, 0x5555);
    // Wait for write posting to complete
    while (GET32(WDT_WWPS) != 0);
}