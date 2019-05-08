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

#define MAX(x, y) ((x) >= (y) ? (x) : (y))

void *xrealloc(void * ptr, size_t size) {
    void * buf = realloc(ptr, size);
    if (!buf)
    {
        perror("xrealloc failed");
        exit(1);
    }
    return buf;
}

void *xmalloc(size_t size) {
    void * buf = malloc(size);
    if (!buf)
    {
        perror("xmalloc failed");
        exit(1);
    }
    return buf;
}

void *xcalloc(size_t num, size_t size) {
    void * buf = calloc(num, size);
    if (!buf)
    {
        perror("xcalloc failed");
        exit(1);
    }
    return buf;
}

void fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("FATAL: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    exit(1);
}

void syntax_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("Syntax Error: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

//Stretchy buffers
typedef struct BufHdr {
    size_t len;
    size_t cap;
    char buf[0];
} BufHdr;

#define buf__hdr(b) ((BufHdr *)((char *)(b) - (int)offsetof(BufHdr, buf)))
#define buf__fits(b, n) ((buf_len(b) + (n)) <= buf_cap(b))
#define buf__fit(b, n) (buf__fits(b, n) ? 0 : (b = buf__grow(b, buf_len(b) + (n), sizeof(*(b)))))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_push(b, ...) (buf__fit(b, 1), (b)[buf__hdr(b)->len++] = (__VA_ARGS__))
#define buf_free(b) ((b) ? (free(buf__hdr(b)), (b) = NULL) : 0)
#define buf_end(b) ((b) + buf_len(b))

void *buf__grow(const void *buf, size_t new_len, size_t elem_size) {
    assert(buf_cap(buf) <= (SIZE_MAX - 1)/2);
    size_t new_cap = MAX(1 + 2 * buf_cap(buf), new_len);
    assert(new_len <= new_cap);
    assert(new_cap <= (SIZE_MAX - offsetof(BufHdr, buf))/elem_size);
    size_t new_size = offsetof(BufHdr, buf) + new_cap * elem_size;
    BufHdr *new_hdr;
    if (buf){
        new_hdr = xrealloc(buf__hdr(buf), new_size);
    } else {
        new_hdr = xmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

void buf_test(void)
{
    int *buf = 0;
    assert(buf == 0);

    int n = 1024;
    for (int i = 0; i < n; ++i) {
        buf_push(buf, i);
    }
    assert(buf_len(buf) == n);
    for (int i = 0; i < n; ++i) {
        assert(buf[i] == i);
    }
    assert(buf_len(buf) == n);
    buf_free(buf);
    assert(buf == NULL);
    assert(buf_len(buf) == 0);
}

typedef struct Intern {
    int len;
    const char * str;
} Intern;

static Intern *interns;

const char *str_intern_range(const char *start, const char *end) {
    size_t len = end - start;
    for (Intern *it = interns; it != buf_end(interns); ++it) {
        if (it->len == len && strncmp(it->str, start, len) == 0) {
            return it->str;
        }
    }
    char *str = xmalloc(len+1);
    memcpy(str, start, len);
    str[len] = 0;
    Intern istr = (Intern) {len, str};
    buf_push(interns, istr);
    return str;
}

const char *str_intern(const char *str) {
    return str_intern_range(str, str + strlen(str));
}

void str_intern_test(void) {
    char z [] = "hello1";
    char x [] = "hello";
    char y [] = "hello";
    assert(x != y);
    assert(str_intern(x) == str_intern(y));
    assert(str_intern(x) != str_intern(z));
}

//lexing: translating char stream to token stream
// 1234 (x+y) => '1234' '(' 'x' '+' 'y' ')'
typedef enum TokenKind {
    TOKEN_EOF = 0,
    TOKEN_LAST_CHAR = 127,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_NAME,
    TOKEN_STR,
    TOKEN_LSHIFT,
    TOKEN_RSHIFT,
    TOKEN_EQ,
    TOKEN_NOTEQ,
    TOKEN_LTEQ,
    TOKEN_GTEQ,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_INC,
    TOKEN_DEC,
    TOKEN_COLON_ASSIGN,
    TOKEN_ADD_ASSIGN,
    TOKEN_SUB_ASSIGN,
    TOKEN_OR_ASSIGN,
    TOKEN_AND_ASSIGN,
    TOKEN_XOR_ASSIGN,
    TOKEN_LSHIFT_ASSIGN,
    TOKEN_RSHIFT_ASSIGN,
    TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN,
    TOKEN_MOD_ASSIGN,
} TokenKind;

typedef enum TokenModifier {
    TOKENMOD_NONE,
    TOKENMOD_HEX,
    TOKENMOD_BIN,
    TOKENMOD_OCT,
    TOKENMOD_DEC,
    TOKENMOD_CHAR,
} TokenMod;

size_t copy_token_kind_str(char *dest, size_t dest_size, TokenKind kind) {
    size_t n = 0;
    switch (kind) {
        case TOKEN_EOF:
            n = snprintf(dest, dest_size, "end of file");
            break;
        case TOKEN_INT:
            n = snprintf(dest, dest_size, "integer");
            break;
        case TOKEN_NAME:
            n = snprintf(dest, dest_size, "name");
            break;
        case TOKEN_FLOAT:
            n = snprintf(dest, dest_size, "float");
            break;
        case TOKEN_STR:
            n = snprintf(dest, dest_size, "string");
        default:
            if(kind < 128 && isprint(kind)) {
                n = snprintf(dest, dest_size, "%c", kind);
            }
            else {
                n = snprintf(dest, dest_size, "<ASCII %d>", kind);
            }
            break;
    }
    return n;
}

const char *token_kind_str(TokenKind kind) {
    static char buf[256];
    size_t n = copy_token_kind_str(buf, sizeof(buf), kind);
    assert(n + 1 <= sizeof(buf));
    return buf;
}

typedef struct Token{
    TokenKind kind;
    TokenMod modifier;
    const char *start;
    const char *end;
    union {
        uint64_t intval;
        double floatval;
        char *name;
        char *strval;
    };
} Token;

Token token;
const char *stream;

//const char *keyword_if;
//const char *keyword_for;
//const char *keyword_while;
//
//void init_keywords() {
//    keyword_if = str_intern("if");
//    keyword_for = str_intern("for");
//    keyword_while = str_intern("while");
//}

uint8_t char_to_digit[256] = {
    ['0'] = 0,
    ['1'] = 1,
    ['2'] = 2,
    ['3'] = 3,
    ['4'] = 4,
    ['5'] = 5,
    ['6'] = 6,
    ['7'] = 7,
    ['8'] = 8,
    ['9'] = 9,
    ['a'] = 10, ['A'] = 10,
    ['b'] = 11, ['B'] = 11,
    ['c'] = 12, ['C'] = 12,
    ['d'] = 13, ['D'] = 13,
    ['e'] = 14, ['E'] = 14,
    ['f'] = 15, ['F'] = 15,
};

void scan_int(void) {
    uint64_t base = 10;
    if (*stream == '0') {
        stream++;
        token.modifier = TOKENMOD_DEC;
        if (tolower(*stream) == 'x') { //hex
            base = 16;
            stream++;
            token.modifier = TOKENMOD_HEX;
        } else if (isdigit(*stream)) { // octal
            base = 8;
            token.modifier = TOKENMOD_OCT;
        } else if (*stream == 'b') {
            base = 2;
            stream++;
            token.modifier = TOKENMOD_BIN;
        }
    }
    uint64_t val = 0;
    for (;;) {
        int digit = char_to_digit[*stream];
        if (digit == 0 && *stream != '0') {
            break;
        }
        if (digit >= base) {
            syntax_error("Digit '%c' out of range for base %llu", *stream, base);
        }
        if (val > (UINT64_MAX - digit)/10) {
            syntax_error("Integer literal overflow");
            val = 0;
            while (isdigit(*stream)) {
                ++stream;
            }
            --stream;
        } else {
            val = val*base + digit;
        }
        ++stream;
    }
    token.intval = val;
    token.kind = TOKEN_INT;
}

void scan_float(void) {
    const char * start = stream;
    while (isdigit(*stream)) {
        ++stream;
    }
    if (*stream == '.') {
        ++stream;
    }
    while (isdigit(*stream)) {
        ++stream;
    }
    if (tolower(*stream) == 'e') {
        ++stream;
        if (*stream == '+' || *stream == '-') { ++stream; }
        if (!isdigit(*stream)) {
            syntax_error("Expected digit after float literal exponent, found '%c'", stream);
        }
    }
    while (isdigit(*stream)) {
        ++stream;
    }
    double val = strtod(start, (char **) &stream);
    if (val == HUGE_VAL || val == -HUGE_VAL) {
        syntax_error("Float literal overflow");
    }
    token.floatval = val;
    token.kind = TOKEN_FLOAT;
    token.modifier = TOKENMOD_NONE;
}

char escape_to_char[256] = {
        ['n'] = '\n',
        ['r'] = '\r',
        ['t'] = '\t',
        ['v'] = '\v',
        ['b'] = '\b',
        ['a'] = '\a',
        ['0'] = '\0',
};

void scan_char() {
    assert(*stream == '\'');
    ++stream;

    char val = 0;
    if (*stream == '\'') {
        syntax_error("Char literal cannot be empty");
        ++stream;
    } else if (*stream == '\n') {
        syntax_error("Char literal cannot contain newline");
    } else if (*stream == '\\') {
        ++stream;
        val = escape_to_char[*stream];
        if (val == 0 && *stream != '0') {
            syntax_error("Invalid char literal escape '\\%c'", *stream);
        }
        ++stream;
    } else {
        val = *stream;
        ++stream;
    }

    if (*stream != '\'') {
        syntax_error("Expected closing char quote, got '%c'", *stream);
    } else {
        ++stream;
    }

    token.kind = TOKEN_INT;
    token.intval = val;
    token.modifier = TOKENMOD_CHAR;
}

void scan_str() {
    assert(*stream == '"');
    ++stream;
    token.start = stream;
    char *str = NULL;
    while (*stream && *stream != '"') {
        if (*stream == '\n') {
            syntax_error("String literal cannot contain newline");
        } else if (*stream == '\\') {
            ++stream;
            char val = escape_to_char[*stream];
            if (val == 0 && *stream != '0') {
                syntax_error("Invalid char literal escape '\\%c'", *stream);
            } else {
                buf_push(str, val);
            }
        } else {
            buf_push(str, *stream);
        }
        ++stream;
    }
    if (*stream) {
        assert(*stream == '"');
        ++stream;
    } else {
        syntax_error("Unexpected end of file in string literal");
    }
    buf_push(str, 0);
    token.end = stream;
    token.kind = TOKEN_STR;
    token.strval = str;
}

#define CASE1(c, c1, k1) \
        case c: \
            token.kind = *stream++; \
            if (*stream == c1) { \
                token.kind = k1; \
                ++stream; \
            }\
            break;

#define CASE2(c, c1, k1, c2, k2) \
        case c:\
            token.kind = *stream++; \
            if (*stream == c1) { \
                token.kind = k1; \
                ++stream; \
            } else if (*stream == c2) { \
                token.kind = k2; \
                ++stream; \
            } \
            break;

void next_token(void) {
    bool whitespace = true;
    while (whitespace) {
        switch (*stream) {
            case '\n': case '\r': case '\f': case '\t': case ' ': case '\v':
                ++stream;
            default:
                whitespace = false;
        }
    }

    token.start = stream;
    switch (*stream) {
        case('\"'):
            scan_str();
            break;
        case ('\''):
            scan_char();
            break;
        case('.'):
            scan_float();
            break;
        case('0'): case('1'): case('2'): case('3'): case('4'):
            case('5'): case('6'): case('7'): case('8'): case('9'): {
                while(isdigit(*stream)) {
                    stream++;
                }
                if (*stream == '.' || *stream == 'e' || *stream == 'E') {
                    stream = token.start;
                    scan_float();
                } else {
                    stream = token.start;
                    scan_int();
                }
            }
            break;
        case('a'):case('b'):case('c'):case('d'):case('e'):
        case('f'):case('g'):case('h'):case('i'):case('j'):
        case('k'):case('l'):case('m'):case('n'):case('o'):
        case('p'):case('q'):case('r'):case('s'):case('t'):
        case('u'):case('v'):case('w'):case('x'):case('y'):
        case('z'):case('A'):case('B'):case('C'):case('D'):
        case('E'):case('F'):case('G'):case('H'):case('I'):
        case('J'):case('K'):case('L'):case('M'):case('N'):
        case('O'):case('P'):case('Q'):case('R'):case('S'):
        case('T'):case('U'):case('V'):case('W'):case('X'):
        case('Y'):case('Z'):case('_'): {
            while (isalnum(*stream) || *stream == '_') {
                ++stream;
            }
            token.name = (char *)str_intern_range(token.start, stream);
            token.kind = TOKEN_NAME;
        }
            break;
        case '<':
            token.kind = *stream++;
            if (*stream == '<') {
                token.kind = TOKEN_LSHIFT;
                stream++;
                if (*stream == '=') {
                    token.kind = TOKEN_LSHIFT_ASSIGN;
                    stream++;
                }
            } else if (*stream == '=') {
                token.kind = TOKEN_LTEQ;
                stream++;
            }
            break;
        case '>':
            token.kind = *stream++;
            if (*stream == '>') {
                token.kind = TOKEN_RSHIFT;
                stream++;
                if (*stream == '=') {
                    token.kind = TOKEN_RSHIFT_ASSIGN;
                    stream++;
                }
            } else if (*stream == '=') {
                token.kind = TOKEN_GTEQ;
                stream++;
            }
            break;
        CASE1 (':', '=', TOKEN_COLON_ASSIGN)
        CASE1 ('*', '=', TOKEN_MUL_ASSIGN)
        CASE1 ('/', '=', TOKEN_DIV_ASSIGN)
        CASE1 ('%', '=', TOKEN_MOD_ASSIGN)
        CASE1 ('^', '=', TOKEN_XOR_ASSIGN)
        CASE1 ('=', '=', TOKEN_EQ)
        CASE1 ('!', '=', TOKEN_NOTEQ)
        CASE2('+', '+', TOKEN_INC, '=', TOKEN_ADD_ASSIGN)
        CASE2('-', '-', TOKEN_DEC, '=', TOKEN_SUB_ASSIGN)
        CASE2('&', '&', TOKEN_AND, '=', TOKEN_AND_ASSIGN)
        CASE2('|', '|', TOKEN_OR, '=', TOKEN_OR_ASSIGN)

        default:
            token.kind = *stream++;
            break;
    }
    token.end = stream;
}

void init_stream(const char *str) {
    stream = str;
    next_token();
}

bool is_token(TokenKind kind) {
    return token.kind == kind;
}

bool is_token_name(const char *name) {
    return token.kind == TOKEN_NAME && token.name == name;
}

bool match_token(TokenKind kind) {
    if (is_token(kind)) {
        next_token();
        return true;
    } else {
        return false;
    }
}

bool expect_token(TokenKind kind) {
    if (is_token(kind)) {
        next_token();
        return true;
    } else {
        fatal("expected token %s, got %s", token_kind_str(kind), token_kind_str(token.kind));
        return false;
    }
}

#define assert_token(x) assert(match_token(x))
#define assert_token_name(x) assert(token.name == str_intern(x) && match_token(TOKEN_NAME))
#define assert_token_int(x) assert(token.intval == (x) && match_token(TOKEN_INT))
#define assert_token_float(x) assert(token.floatval == (x) && match_token(TOKEN_FLOAT))
#define assert_token_char(x) assert(token.intval == (x) && match_token(TOKEN_INT) && token.modifier == TOKENMOD_CHAR)
#define assert_token_str(x) assert((strcmp(token.strval, x) == 0) && match_token(TOKEN_STR))
#define assert_token_eof() assert(is_token(0))
#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-assert-side-effect"
void lex_test(void)
{
    // Operator Tests
    init_stream(": := + += ++ - -- -=");
    assert_token(':');
    assert_token(TOKEN_COLON_ASSIGN);
    assert_token('+');
    assert_token(TOKEN_ADD_ASSIGN);
    assert_token(TOKEN_INC);
    assert_token('-');
    assert_token(TOKEN_DEC);
    assert_token(TOKEN_SUB_ASSIGN);
    assert_token_eof();

    init_stream("< <= << <<= > >= >> >>=");
    assert_token('<');
    assert_token(TOKEN_LTEQ);
    assert_token(TOKEN_LSHIFT);
    assert_token(TOKEN_LSHIFT_ASSIGN);
    assert_token('>');
    assert_token(TOKEN_GTEQ);
    assert_token(TOKEN_RSHIFT);
    assert_token(TOKEN_RSHIFT_ASSIGN);
    assert_token_eof();

    init_stream("* *= / /= % %= ^ ^=");
    assert_token('*');
    assert_token(TOKEN_MUL_ASSIGN);
    assert_token('/');
    assert_token(TOKEN_DIV_ASSIGN);
    assert_token('%');
    assert_token(TOKEN_MOD_ASSIGN);
    assert_token('^');
    assert_token(TOKEN_XOR_ASSIGN);
    assert_token_eof();

    init_stream("& && &= | || |= = == ! !=");
    assert_token('&');
    assert_token(TOKEN_AND);
    assert_token(TOKEN_AND_ASSIGN);
    assert_token('|');
    assert_token(TOKEN_OR);
    assert_token(TOKEN_OR_ASSIGN);
    assert_token('=');
    assert_token(TOKEN_EQ);
    assert_token('!');
    assert_token(TOKEN_NOTEQ);
    assert_token_eof();

    // String literals tests
    init_stream("\"\\n\" \"woo9\"");
    assert_token_str("\n");
    assert_token_str("woo9");
    assert_token_eof();

    init_stream("'\\t' 'a'");
    assert_token_char('\t');
    assert_token_char('a');
    assert_token_eof();

    // Integer and float tests
    init_stream("1e3 1.0e3 0xff 011 0b1010 0.23");
    assert_token_float(1e3);
    assert_token_float(1.0e3);
    assert(token.modifier == TOKENMOD_HEX);
    assert_token_int(0xff);
    assert(token.modifier == TOKENMOD_OCT);
    assert_token_int(011);
    assert(token.modifier == TOKENMOD_BIN);
    assert_token_int(0xa);
    assert_token_float(0.23);
    assert_token_eof();

    // Misc tests
    init_stream("a*+987(_wer&tfd*wer");
    assert_token_name("a");
    assert_token('*');
    assert_token('+');
    assert_token_int(987);
    assert_token('(');
    assert_token_name("_wer");
    assert_token('&');
    assert_token_name("tfd");
    assert_token('*');
    assert_token_name("wer");
    assert_token_eof();
}
#pragma clang diagnostic pop
#undef assert_token
#undef assert_token_name
#undef assert_token_int
#undef assert_token_eof

int main(int argc, char **argv) {
    buf_test();
    str_intern_test();
    lex_test();
    return 0;
}