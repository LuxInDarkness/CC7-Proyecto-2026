#include "../../libraries/io.h"
#include "../../libraries/time.h"

void timer_irq_handler() {
    // Stub implementation - not used in p2
}

int main() {
    int char_value = 0;
    char buffer[2]; // Buffer to hold single character and null terminator
    // Print a-z infinite times
    while (1) {
        for (int i = 0; i < 26; i++) {
            char_value = (int)'a' + i; // Convert 'a' to its integer value and add i
            buffer[0] = (char)char_value; // Convert back to character and store in buffer
            buffer[1] = '\0'; // Null-terminate the string
            print("----From P2: %s----\n", buffer);
            sleep(0.1); // Sleep for 0.1 seconds
        }
    }
    
    return 0;
}