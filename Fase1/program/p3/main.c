#include "../../libraries/io.h"
#include "../../libraries/time.h"

int main() {
    int char_value = 0;
    char buffer[2]; // Buffer to hold single character and null terminator
    int max_iterations = 5;
    int iterations = 0;

    // Print z-a infinite times
    while (iterations < max_iterations) {
        for (int i = 25; i >= 0; i--) {
            char_value = (int)'a' + i; // Convert 'a' to its integer value and add i
            buffer[0] = (char)char_value; // Convert back to character and store in buffer
            buffer[1] = '\0'; // Null-terminate the string
            print("----From P3: %s----\n", buffer);
            sleep(0.1); // Sleep for 0.1 seconds
        }
        iterations++;
    }

    int r1 = write(9, "X\n", 2);
    int r2 = write(1, (void *)0xFFFFFFFF, 8);
    int r3 = write(1, "[B] ok\n", 7);

    if (r1 < 0 && r2 < 0 && r3 == 7)
        exit(0);
    else
        exit(1);
    return 0;
}