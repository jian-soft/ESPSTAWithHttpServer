#include <string.h>
#include <stdlib.h>


/* parse cmds: "L,1;R,4;FL,1;BL,4;", 紧凑无空格 */
int parse_cmd(char *cmds, char *code, int *beat)
{
    int comma_pos = 0, semicolon_pos = 0;
    int comma_found = 0, semicolon_found = 0;

    //search comma
    while(cmds[comma_pos]) {
        if (cmds[comma_pos] == ',') {
            comma_found = 1;
            break;
        }
        comma_pos++;
    }
    //search semicolon
    while(cmds[semicolon_pos]) {
        if (cmds[semicolon_pos] == ';') {
            semicolon_found = 1;
            break;
        }
        semicolon_pos++;
    }

    if (0 == comma_found || 0 == semicolon_found || 0 == comma_pos || 0 == semicolon_pos) {
        return -1;
    }
    if (comma_pos >= semicolon_pos) {
        return -1;
    }

    memcpy(code, cmds, comma_pos);
    code[comma_pos] = '\0';
    *beat = atoi(cmds + comma_pos + 1);

    return semicolon_pos;
}



