#include <stdio.h>
#include "hint.h"

void redHint(const char* mes) {
    printf("\033[1;31m%s\033[0m", mes);
}
