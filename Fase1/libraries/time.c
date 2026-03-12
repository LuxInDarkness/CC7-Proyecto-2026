#include "time.h"

void sleep(float seconds) {
    // Simple busy-wait loop to create a delay
    // Note: This is not an accurate way to implement sleep, but it serves the purpose for demonstration
    for (volatile int i = 0; i < ((int)(seconds * 10000000)); i++);
}