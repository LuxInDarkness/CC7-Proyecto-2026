#include "../../libraries/io.h"
#include "../../libraries/time.h"

void timer_irq_handler() {
    // Stub implementation - not used in p1
}

int main() {
    // Print 0-9 infinite times
    while (1) {
        for (int i = 0; i < 10; i++) {
            print("----From P1: %d----\n", i);
            sleep(0.1); // Sleep for 0.1 seconds
        }
    }
    
    return 0;
}