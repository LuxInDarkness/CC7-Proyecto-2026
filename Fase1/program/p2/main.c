#include "../../libraries/io.h"
#include "../../libraries/time.h"

int main() {
    int char_value = 0;
    char buffer[2]; // Buffer to hold single character and null terminator
    // Print a-z infinite times
    while (1) {
        for (int i = 0; i < 26; i++) {
            char_value = alpha2int("a") + i; // Convert 'a' to its integer value and add i
            os_write(int2alpha(char_value, buffer)); // Convert back to char and print
            sleep(0.1); // Sleep for 0.1 seconds
        }
    }
    
    return 0;
}