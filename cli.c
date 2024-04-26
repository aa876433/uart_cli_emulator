//
// Created by JohnLin on 2024/4/20.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include "gapbuf.h"
#include "cli.h"

#define MAX_HISTORY 8
#define MAX_COMPLETE 128
#define MAX_ARGS 8
#define MAX_COMMAND_LENGTH 256
#define DELIMS " \t\n"

#define BACKSPACE 8
#define TAB       9
#define ENTER     13
#define ESC       27
#define SPACE     32
#define UP        72
#define LEFT      75
#define RIGHT     77
#define DOWN      80
#define DELETE    83
#define ARROW     224

struct cli {
    void *commands;
    void *gap;
    char *argv[MAX_COMPLETE];
    uint16_t *command_length;
    char history[MAX_HISTORY][MAX_COMMAND_LENGTH];
    char backup[MAX_COMMAND_LENGTH];
    uint32_t command_count;
    uint32_t same_prefix_count;
    uint32_t command_index[MAX_COMPLETE];
    uint8_t argc;
    uint8_t max_command_len;
    uint8_t tab;
    uint8_t arrow;
    uint8_t history_ptr;
    uint8_t history_size;
    uint8_t history_move;
    uint8_t history_update;
    uint8_t history_backup;
};

struct cli g_cli;

void sort_commands(command commands[], int n) {
    for (int i = 1; i < n; i++) {
        command cmd = commands[i];
        int j = i - 1;

        while (j >= 0 && commands[j].name != NULL && strcmp(commands[j].name, cmd.name) > 0) {
            commands[j + 1] = commands[j];
            j = j - 1;
        }

        commands[j + 1] = cmd;
    }
}

void cli_init(void *commands) {
    g_cli.commands = commands;
    void *gap = gap_buf_create(MAX_COMMAND_LENGTH);
    g_cli.gap = gap;
    g_cli.tab = 0;
    g_cli.arrow = 0;
    g_cli.history_ptr = 0;
    g_cli.history_size = 0;
    g_cli.history_move = 0;
    g_cli.history_update = 0;
    g_cli.history_backup = 0;
    g_cli.same_prefix_count = 0;

    uint32_t command_count = 0;

    command *cmd = commands;
    for (int i = 0; cmd[i].name; i++) {
        command_count++;
    }
    g_cli.command_count = command_count;

    sort_commands(commands, command_count);
    uint16_t *command_length = malloc(command_count * sizeof(uint16_t));
    uint16_t max_command_len = 0;
    for (int i = 0; i < command_count; i++) {
        uint16_t len = strlen(cmd[i].name);
        command_length[i] = len;
        if (len > max_command_len) {
            max_command_len = len;
        }
    }

    g_cli.command_length = command_length;
    g_cli.max_command_len = max_command_len;
}

void cli_prompt(void) {
    printf("$> ");
}

void cli_parse(char *input) {
    int arg_count = 0;
    char *token = strtok(input, DELIMS);

    while (token != NULL && arg_count < MAX_ARGS - 1) {
        g_cli.argv[arg_count++] = token;
        token = strtok(NULL, DELIMS);
    }

    g_cli.argv[arg_count] = NULL;
    g_cli.argc = arg_count;
}

void cli_move_cursor(int num, uint8_t left) {
    if (left) {
        printf("\x1b[%dD", num);
    } else {
        printf("\x1b[%dC", num);
    }
}

void cli_save_history(const char *str) {
    strncpy(g_cli.history[g_cli.history_ptr], str, MAX_COMMAND_LENGTH - 1);
    g_cli.history_ptr = (g_cli.history_ptr + 1) % MAX_HISTORY;
    g_cli.history_size = g_cli.history_size >= MAX_HISTORY ? MAX_HISTORY : g_cli.history_size + 1;
    g_cli.history_move = 0;
    g_cli.history_backup = 0;
}

void cli_restore_history(uint8_t only_show) {
    int pos;
    const char *history;
    if (g_cli.history_update) {
        if (g_cli.history_move == 0) {
            assert(g_cli.history_backup);
            history = g_cli.backup;
        } else {
            pos = (g_cli.history_ptr + MAX_HISTORY - g_cli.history_move) % MAX_HISTORY;
            history = g_cli.history[pos];
        }

        if (only_show) {
            printf("%s", history);
        } else {
            gap_buf_restore(g_cli.gap, history);
            g_cli.history_update = 0;
        }
    }
}

void cli_handle_history(uint8_t up) {
    cli_restore_history(0);
    if (up) {
        if (g_cli.history_move < g_cli.history_size) {
            if (g_cli.history_backup == 0) {
                const char *str = gap_buf_get_all(g_cli.gap);
                strncpy(g_cli.backup, str, MAX_COMMAND_LENGTH - 1);
                g_cli.history_backup = 1;
            }

            g_cli.history_update = 1;
            g_cli.history_move++;
        }
    } else {
        if (g_cli.history_move > 0) {
            g_cli.history_move--;
            g_cli.history_update = 1;
        }
    }
}

void cli_get_same_prefix_command(const char *str) {
    int valid;
    gap_buf_get_len(g_cli.gap, NULL, &valid);
    if (valid == 0 || valid > g_cli.max_command_len) {
        return;
    }

    command *cmd = g_cli.commands;
    int low = 0, high = g_cli.command_count - 1;
    g_cli.same_prefix_count = 0;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = strncmp(cmd[mid].name, str, valid);

        if (cmp < 0) {
            low = mid + 1;
        } else if (cmp > 0) {
            high = mid - 1;
        } else {
            int start = mid;
            while (start >= low && strncmp(cmd[start].name, str, valid) == 0) {
                g_cli.command_index[g_cli.same_prefix_count++] = start;
                start--;
            }

            for (int i = 0; i < g_cli.same_prefix_count / 2; i++) {
                int temp = g_cli.command_index[i];
                g_cli.command_index[i] = g_cli.command_index[g_cli.same_prefix_count - 1 - i];
                g_cli.command_index[g_cli.same_prefix_count - 1 - i] = temp;
            }

            int end = mid + 1;
            while (end <= high && strncmp(cmd[end].name, str, valid) == 0) {
                g_cli.command_index[g_cli.same_prefix_count++] = end;
                end++;
            }
            break;
        }
    }
}


void cli_display_complete(void) {
    command *cmd = g_cli.commands;
    int max_len = 0;
    int cmd_count = 0;
    int cmd_len;
    int cmd_per_line;
    int valid;
    gap_buf_get_len(g_cli.gap, NULL, &valid);

    printf("\n");

    for (uint32_t i = 0; i < g_cli.same_prefix_count; i++) {
        cmd_len = g_cli.command_length[g_cli.command_index[i]];
        if (cmd_len > max_len) {
            max_len = cmd_len;
        }
    }

    cmd_per_line = 80 / max_len;

    for (int i = 0; i < g_cli.same_prefix_count; i++) {
        cmd_len = g_cli.command_length[g_cli.command_index[i]];
        if (cmd_len >= valid) {
            printf("%s", cmd[g_cli.command_index[i]].name);
            printf("%*s", max_len - cmd_len + 3, " ");

            if (++cmd_count == cmd_per_line) {
                printf("\n");
                cmd_count = 0;
            }
        }
    }

    printf("\n");
}

void cli_auto_complete(void) {
    if (g_cli.same_prefix_count == 0) {
        return;
    }

    command *cmd = g_cli.commands;
    int pos;
    gap_buf_get_len(g_cli.gap, NULL, &pos);
    int prefix_len = 0;
    char c;

    if (g_cli.same_prefix_count == 1) {
        prefix_len = g_cli.command_length[g_cli.command_index[0]] - pos;
    } else {
        for (int i = pos; ; i++) {
            c = cmd[g_cli.command_index[0]].name[i];
            for (int j = 1; j < g_cli.same_prefix_count; j++) {
                if (cmd[g_cli.command_index[j]].name[i] == '\0' ||
                    cmd[g_cli.command_index[j]].name[i] != c) {
                    goto EXIT;
                }
            }
            prefix_len++;
        }
    }


EXIT:
    for (int i = pos; i < pos + prefix_len; i++) {
        c = cmd[g_cli.command_index[0]].name[i];
        printf("%c", c);
        gap_buf_insert(g_cli.gap, c);
    }
}

void cli_clear(void) {
    int front;
    int valid;
    gap_buf_get_len(g_cli.gap, &front, &valid);
    if (front > 0) {
        cli_move_cursor(front, 1);
    }
    if (valid > 0) {
        printf("%*s", valid, " ");
        cli_move_cursor(valid, 1);
    }
}

void cli_reset(void) {
    g_cli.tab = 0;
    g_cli.arrow = 0;
    g_cli.history_update = 0;
    g_cli.history_backup = 0;
    g_cli.history_move = 0;
    gap_buf_reset(g_cli.gap);
    cli_prompt();
}

void cli_execute(char *str) {
    cli_parse(str);
    if (g_cli.argc > 0) {
        command *cmd = g_cli.commands;
        int low = 0;
        int high = g_cli.command_count - 1;
        int find = 0;

        while (low <= high) {
            int mid = low + (high - low) / 2;
            int cmp = strncmp(cmd[mid].name, g_cli.argv[0], MAX_COMMAND_LENGTH - 1);

            if (cmp == 0) {
                cmd[mid].func(g_cli.argc - 1, &g_cli.argv[1]);
                find = 1;
                break;
            } else if (cmp < 0) {
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }

        if (!find) {
            printf("Command %s not found\n", g_cli.argv[0]);
        }
    }
}

void cli_handle(uint8_t c) {
    int len;
    int valid;
    int front;
    const char *str;

    if (g_cli.arrow) {
        switch (c) {
            case UP:
            case DOWN:
                cli_handle_history(c == UP);
                if (g_cli.history_update) {
                    cli_clear();
                    cli_restore_history(1);
                }
                break;
            case LEFT:
            case RIGHT:
                if (c == LEFT) {
                    cli_restore_history(0);
                }
                gap_buf_get_len(g_cli.gap, &front, &valid);
                if (gap_buf_can_move(g_cli.gap, c == LEFT)) {
                    cli_move_cursor(1, c == LEFT);
                    gap_buf_move(g_cli.gap, c == LEFT);
                }
                break;

            case DELETE:
                if (gap_buf_can_move(g_cli.gap, 0)) {
                    gap_buf_delete(g_cli.gap);
                    str = gap_buf_get_forward(g_cli.gap, &len);
                    printf("%s ", str);
                    cli_move_cursor(len + 1, 1);
                    g_cli.history_backup = 0;
                    g_cli.tab = 0;
                }
                break;
            default:
                break;
        }
        g_cli.arrow = 0;
        return;
    }

    switch (c) {
        case BACKSPACE:
            cli_restore_history(0);
            if (gap_buf_can_move(g_cli.gap, 1)) {
                printf("\b \b");
                gap_buf_backspace(g_cli.gap);
                str = gap_buf_get_forward(g_cli.gap, &len);
                printf("%s ", str);
                cli_move_cursor(len + 1, 1);
                g_cli.history_backup = 0;
                g_cli.tab = 0;
            }
            break;
        case TAB:
            str = gap_buf_get_all(g_cli.gap);
            if (g_cli.tab) {
                if (g_cli.same_prefix_count > 0) {
                    cli_display_complete();
                    cli_prompt();
                    printf("%s", str);
                }
                g_cli.tab = 0;
            } else {
                cli_restore_history(0);
                cli_get_same_prefix_command(str);
                cli_auto_complete();
                g_cli.tab = 1;
            }
            break;
        case ENTER:
            printf("\n");
            cli_restore_history(0);
            gap_buf_get_len(g_cli.gap, NULL, &valid);
            if (valid > 0) {
                str = gap_buf_get_all(g_cli.gap);
                cli_save_history(str);
                cli_execute((char *) str);
            }
            cli_reset();
            break;
        case ARROW:
            g_cli.arrow = 1;
            break;
        default:
            printf("%c", c);
            cli_restore_history(0);
            g_cli.tab = 0;
            g_cli.history_move = 0;
            g_cli.history_backup = 0;
            gap_buf_insert(g_cli.gap, c);
            if (gap_buf_can_move(g_cli.gap, 0)) {
                str = gap_buf_get_forward(g_cli.gap, &len);
                printf("%s", str);
                cli_move_cursor(len, 1);
            }
            break;
    }
}
