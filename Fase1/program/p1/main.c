#include "../../libraries/io.h"
#include "../../libraries/time.h"

int main() {
    // Print 0-9 infinite times
    while (1) {
        for (int i = 0; i < 10; i++) {
            os_write_num(i);
            sleep(0.1); // Sleep for 0.1 seconds
        }
    }
    
    return 0;
}