#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>
#include <inttypes.h>

#include "common.c"
#include "lex.c"
#include "ast.c"

int main(int argc, char **argv) {
    common_test();
    lex_test();
    ast_test();
    return 0;
}
