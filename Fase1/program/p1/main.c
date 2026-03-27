#include "../../libraries/io.h"
#include "../../libraries/time.h"
#include "../../libraries/std_functions.h"

int main() {
    // Print 0-9 infinite times
    int max_iterations = 3;
    int counter = 0;

    while (1) {
        for (int i = 0; i < 10; i++) {
            print("----From P1: %d----\n", i);
            sleep(0.1); // Sleep for 0.1 seconds
        }

        if (++counter >= max_iterations) {
            counter = 0;
            yield();
        }
    }
    
    return 0;
}