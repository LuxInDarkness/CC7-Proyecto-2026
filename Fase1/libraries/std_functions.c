#include "std_functions.h"

void yield() {
    print("Yielding.....................................\n");
    __asm__ volatile ("swi #0");   // trigger SWI exception
}
