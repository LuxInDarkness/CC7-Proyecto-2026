#include "../../libraries/io.h"
#include "../../libraries/time.h"

int main() {
    const char *msg = "[A] tick\n";
    for (int i = 0; i < 20; i++) {
        int n = write(1, msg, 9);
        if (n < 0) {
            print("Error writing to stdout\n");
            exit(1);
        }
    }
    yield();

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