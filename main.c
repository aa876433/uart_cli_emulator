#include <stdio.h>
#include <conio.h>
#include <string.h>

#include "cli.h"

void printCommand(int argc, char *argv[]) {
    printf("argc %d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
}

void simpleCommand(int argc, char *argv[]) {
    printf("This is a simple command.\n");
}

struct command commands[] = {
    {"prin", printCommand},
    {"print", printCommand},
    {"print123", printCommand},
    {"print123456", printCommand},
    {"print44123456", printCommand},
    {"simple", simpleCommand},
    {NULL, NULL}
};

int main(void) {
    cli_init(commands);
    unsigned char c;
    cli_prompt();
    while (1) {
        c = getch();
        cli_handle(c);
    }
    return 0;
}
