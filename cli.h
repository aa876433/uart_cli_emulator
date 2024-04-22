//
// Created by JohnLin on 2024/4/20.
//

#ifndef CLI_H
#define CLI_H

#include <stdint.h>

typedef void (*command_func)(int argc, char *argv[]);

typedef struct command {
    const char *name;
    command_func func;
} command;

void cli_init(void *commands);
void cli_handle(uint8_t c);
void cli_prompt(void);

#endif //CLI_H
