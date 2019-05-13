//
// Created by David Clements on 2019-05-11.
//

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

#define BUF(x) x

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

void common_test(void) {
    buf_test();
    str_intern_test();
}
