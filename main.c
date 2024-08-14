#include <stdio.h>
#include <stdlib.h>

#include <xbox.h>

int main(void) {
    int used = 1;
    char memory[64];

    float a= 9.1f;
    float b = 3.2f;
    float c = a * b;

    snprintf(memory, sizeof(memory), "Hello, World!! %d %f\r\n", used, c);
    printf("%s", memory);
    printf("%s", memory);
    printf("%s", memory);
    printf("%s", memory);
    printf("%s", memory);
    printf("%s", memory);
    while(1);

    return 0;
}
