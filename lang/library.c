#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#define MAX(x, y) ((x) >= (y) ? (x) : (y))

void *xrealloc(void * ptr, size_t size)
{
    void * buf = realloc(ptr, size);
    if (!buf)
    {
        perror("xrealloc failed");
        exit(1);
    }
    return buf;
}

void *xmalloc(size_t size)
{
    void * buf = malloc(size);
    if (!buf)
    {
        perror("xmalloc failed");
        exit(1);
    }
    return buf;
}

void *xcalloc(size_t num, size_t size)
{
    void * buf = calloc(num, size);
    if (!buf)
    {
        perror("xcalloc failed");
        exit(1);
    }
    return buf;
}

char err_buf [1024*1024];

void fatal(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("FATAL: ");
    vsprintf(err_buf, fmt, args);
    printf("%s \n", err_buf);
    va_end(args);
    exit(1);
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
    const char * s = str_intern(z);
    const char * s2 = str_intern(x);
    assert(str_intern(x) == str_intern(y));
    assert(str_intern(x) != str_intern(z));
}

//lexing: translating char stream to token stream
// 1234 (x+y) => '1234' '(' 'x' '+' 'y' ')'
typedef enum TokenKind{
    TOKEN_INT = 128,
    TOKEN_NAME,
} TokenKind;

size_t copy_token_kind_str(char *dest, size_t dest_size, TokenKind kind) {
    size_t n = 0;
    switch (kind) {
        case 0:
            n = snprintf(dest, dest_size, "end of file");
            break;
        case TOKEN_INT:
            n = snprintf(dest, dest_size, "integer");
            break;
        case TOKEN_NAME:
            n = snprintf(dest, dest_size, "name");
            break;
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
    const char *start;
    const char *end;
    union {
        int intval;
        char *name;
    };
} Token;

Token token;
const char *stream;

const char *keyword_if;
const char *keyword_for;
const char *keyword_while;

void init_keywords() {
    keyword_if = str_intern("if");
    keyword_for = str_intern("for");
    keyword_while = str_intern("while");
}

void next_token(void) {
    token.start = stream;
    switch (*stream) {
        case('0'): case('1'): case('2'): case('3'): case('4'):
            case('5'): case('6'): case('7'): case('8'): case('9'): {
                int val = 0;
                while (isdigit(*stream)) {
                    val *= 10;
                    val += *stream - '0';
                    stream++;
                }
                token.intval = val;
                token.kind = TOKEN_INT;
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
            size_t size = sizeof(size_t);
            token.name = (char *)str_intern_range(token.start, stream);

            token.kind = TOKEN_NAME;
        }
            break;
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

void print_token(Token tok)
{
    Token * toke = &token;
    switch (tok.kind) {
        case TOKEN_INT:
            printf("TOKEN VALUE: %d\n", tok.intval);
            break;
        case TOKEN_NAME: {
            int length = (int) (tok.end - tok.start);
            printf("TOKEN NAME: %.*s ", length, tok.start);
            printf("len: %d \n", length);
        }
            break;
        default:
            printf("TOKEN '%c'\n", tok.kind);
    }
}

void lex_test(void)
{
    buf_free(interns);
    char *source = "a*+987(_wer&tfd*_wer.";
    stream = source;
    next_token();
    while (token.kind)
    {
        //print_token(token);
        next_token();
    }
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


int main(int argc, char **argv) {
    buf_test();
    str_intern_test();
    lex_test();
    return 0;
}