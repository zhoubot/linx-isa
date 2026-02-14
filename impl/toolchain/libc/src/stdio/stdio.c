/*
 * linx-libc: Standard I/O functions
 */

#include <linxisa_libc.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

struct linx_FILE {
    int fd;
};

static struct linx_FILE linx_stdin_file = { .fd = 0 };
static struct linx_FILE linx_stdout_file = { .fd = 1 };
static struct linx_FILE linx_stderr_file = { .fd = 2 };

FILE *stdin = &linx_stdin_file;
FILE *stdout = &linx_stdout_file;
FILE *stderr = &linx_stderr_file;

int putchar(int c) {
    __linx_putchar(c);
    return c;
}

int puts(const char *s) {
    __linx_puts(s);
    return 0;
}

static size_t linx_strnlen(const char *s, size_t max_len) {
    size_t n = 0;
    while (n < max_len && s[n]) {
        n++;
    }
    return n;
}

struct linx_out {
    char *dst;
    size_t size;
    size_t pos;
    size_t count;
    bool to_uart;
};

static void linx_outc(struct linx_out *out, char c) {
    if (out->to_uart) {
        __linx_putchar((unsigned char)c);
    } else {
        if (out->dst && out->size && out->pos < out->size - 1) {
            out->dst[out->pos] = c;
        }
        out->pos++;
    }
    out->count++;
}

static void linx_out_repeat(struct linx_out *out, char c, size_t n) {
    while (n--) {
        linx_outc(out, c);
    }
}

static int linx_size_to_int(size_t n) {
    if (n > 0x7fffffffU) {
        return 0x7fffffff;
    }
    return (int)n;
}

enum linx_len {
    LINX_LEN_NONE,
    LINX_LEN_HH,
    LINX_LEN_H,
    LINX_LEN_L,
    LINX_LEN_LL,
    LINX_LEN_Z,
    LINX_LEN_T,
    LINX_LEN_J,
};

static unsigned long long linx_get_uarg(va_list *ap, enum linx_len len) {
    switch (len) {
        case LINX_LEN_HH:
        case LINX_LEN_H:
        case LINX_LEN_NONE:
            return (unsigned long long)(unsigned int)va_arg(*ap, unsigned int);
        case LINX_LEN_L:
            return (unsigned long long)va_arg(*ap, unsigned long);
        case LINX_LEN_LL:
            return (unsigned long long)va_arg(*ap, unsigned long long);
        case LINX_LEN_Z:
            return (unsigned long long)va_arg(*ap, size_t);
        case LINX_LEN_T:
            return (unsigned long long)va_arg(*ap, ptrdiff_t);
        case LINX_LEN_J:
            return (unsigned long long)va_arg(*ap, uintmax_t);
    }
    return 0;
}

static long long linx_get_sarg(va_list *ap, enum linx_len len) {
    switch (len) {
        case LINX_LEN_HH:
        case LINX_LEN_H:
        case LINX_LEN_NONE:
            return (long long)(int)va_arg(*ap, int);
        case LINX_LEN_L:
            return (long long)va_arg(*ap, long);
        case LINX_LEN_LL:
            return (long long)va_arg(*ap, long long);
        case LINX_LEN_Z:
            return (long long)va_arg(*ap, ssize_t);
        case LINX_LEN_T:
            return (long long)va_arg(*ap, ptrdiff_t);
        case LINX_LEN_J:
            return (long long)va_arg(*ap, intmax_t);
    }
    return 0;
}

static size_t linx_utoa_rev(unsigned long long v, unsigned base, bool upper,
                            char *buf, size_t buf_size) {
    const char *digits =
        upper ? "0123456789ABCDEF" : "0123456789abcdef";
    size_t n = 0;

    if (buf_size == 0) {
        return 0;
    }

    if (v == 0) {
        buf[n++] = '0';
        return n;
    }

    while (v && n < buf_size) {
        unsigned digit = (unsigned)(v % base);
        buf[n++] = digits[digit];
        v /= base;
    }
    return n;
}

static void linx_out_strn(struct linx_out *out, const char *s, size_t n) {
    for (size_t i = 0; i < n; i++) {
        linx_outc(out, s[i]);
    }
}

static void linx_format_uint(struct linx_out *out, unsigned long long v,
                             unsigned base, bool upper, bool alt, bool left,
                             bool plus, bool space, bool zero_pad,
                             int width, int precision, char sign_ch,
                             const char *prefix_override) {
    char digits[64];
    size_t digit_count = 0;

    if (precision == 0 && v == 0) {
        digit_count = 0;
    } else {
        digit_count = linx_utoa_rev(v, base, upper, digits, sizeof(digits));
    }

    const char *prefix = prefix_override;
    char prefix_buf[2];
    size_t prefix_len = 0;
    if (!prefix) {
        if (alt) {
            if (base == 8) {
                if (v != 0 || precision == 0) {
                    prefix_buf[0] = '0';
                    prefix_len = 1;
                    prefix = prefix_buf;
                }
            } else if (base == 16 && v != 0) {
                prefix_buf[0] = '0';
                prefix_buf[1] = upper ? 'X' : 'x';
                prefix_len = 2;
                prefix = prefix_buf;
            }
        }
    } else {
        prefix_len = strlen(prefix);
    }

    char sign_buf[1];
    size_t sign_len = 0;
    if (sign_ch) {
        sign_buf[0] = sign_ch;
        sign_len = 1;
    } else if (plus) {
        sign_buf[0] = '+';
        sign_len = 1;
    } else if (space) {
        sign_buf[0] = ' ';
        sign_len = 1;
    }

    size_t zeros = 0;
    if (precision >= 0) {
        zero_pad = false;
        if ((size_t)precision > digit_count) {
            zeros = (size_t)precision - digit_count;
        }
    }

    size_t total_len = sign_len + prefix_len + zeros + digit_count;
    size_t pad = 0;
    if (width > 0 && (size_t)width > total_len) {
        pad = (size_t)width - total_len;
    }

    if (!left && !zero_pad) {
        linx_out_repeat(out, ' ', pad);
    }

    if (sign_len) {
        linx_outc(out, sign_buf[0]);
    }
    if (prefix_len) {
        linx_out_strn(out, prefix, prefix_len);
    }

    if (!left && zero_pad) {
        linx_out_repeat(out, '0', pad);
    }

    linx_out_repeat(out, '0', zeros);
    while (digit_count--) {
        linx_outc(out, digits[digit_count]);
    }

    if (left) {
        linx_out_repeat(out, ' ', pad);
    }
}

static void linx_vformat(struct linx_out *out, const char *format, va_list ap) {
    const char *p = format;
    while (*p) {
        if (*p != '%') {
            linx_outc(out, *p++);
            continue;
        }
        p++; /* consume % */
        if (*p == '%') {
            linx_outc(out, *p++);
            continue;
        }

        bool left = false, plus = false, space = false, alt = false, zero = false;
        for (;;) {
            switch (*p) {
                case '-': left = true; p++; continue;
                case '+': plus = true; p++; continue;
                case ' ': space = true; p++; continue;
                case '#': alt = true; p++; continue;
                case '0': zero = true; p++; continue;
                default: break;
            }
            break;
        }

        int width = -1;
        if (*p == '*') {
            width = va_arg(ap, int);
            if (width < 0) {
                left = true;
                width = -width;
            }
            p++;
        } else if (*p >= '0' && *p <= '9') {
            width = 0;
            while (*p >= '0' && *p <= '9') {
                width = width * 10 + (*p - '0');
                p++;
            }
        }

        int precision = -1;
        if (*p == '.') {
            p++;
            precision = 0;
            if (*p == '*') {
                precision = va_arg(ap, int);
                if (precision < 0) {
                    precision = -1;
                }
                p++;
            } else {
                while (*p >= '0' && *p <= '9') {
                    precision = precision * 10 + (*p - '0');
                    p++;
                }
            }
        }

        enum linx_len len = LINX_LEN_NONE;
        if (*p == 'h') {
            p++;
            if (*p == 'h') {
                len = LINX_LEN_HH;
                p++;
            } else {
                len = LINX_LEN_H;
            }
        } else if (*p == 'l') {
            p++;
            if (*p == 'l') {
                len = LINX_LEN_LL;
                p++;
            } else {
                len = LINX_LEN_L;
            }
        } else if (*p == 'z') {
            len = LINX_LEN_Z;
            p++;
        } else if (*p == 't') {
            len = LINX_LEN_T;
            p++;
        } else if (*p == 'j') {
            len = LINX_LEN_J;
            p++;
        }

        const char spec = *p ? *p++ : '\0';
        switch (spec) {
            case 'c': {
                const char ch = (char)va_arg(ap, int);
                size_t pad = 0;
                if (width > 1) {
                    pad = (size_t)(width - 1);
                }
                if (!left) {
                    linx_out_repeat(out, ' ', pad);
                }
                linx_outc(out, ch);
                if (left) {
                    linx_out_repeat(out, ' ', pad);
                }
                break;
            }
            case 's': {
                const char *s = va_arg(ap, const char *);
                if (!s) {
                    s = "(null)";
                }
                size_t max_len = (precision >= 0) ? (size_t)precision : ~(size_t)0;
                size_t len_s = linx_strnlen(s, max_len);
                size_t pad = 0;
                if (width > 0 && (size_t)width > len_s) {
                    pad = (size_t)width - len_s;
                }
                if (!left) {
                    linx_out_repeat(out, ' ', pad);
                }
                linx_out_strn(out, s, len_s);
                if (left) {
                    linx_out_repeat(out, ' ', pad);
                }
                break;
            }
            case 'd':
            case 'i': {
                const long long sval = linx_get_sarg(&ap, len);
                const bool neg = (sval < 0);
                const unsigned long long uval =
                    neg ? (0ULL - (unsigned long long)sval) : (unsigned long long)sval;
                linx_format_uint(out, uval, 10, false, false, left, plus, space,
                                 zero, width, precision, neg ? '-' : 0, NULL);
                break;
            }
            case 'u': {
                const unsigned long long uval = linx_get_uarg(&ap, len);
                linx_format_uint(out, uval, 10, false, false, left, false, false,
                                 zero, width, precision, 0, NULL);
                break;
            }
            case 'o': {
                const unsigned long long uval = linx_get_uarg(&ap, len);
                linx_format_uint(out, uval, 8, false, alt, left, false, false,
                                 zero, width, precision, 0, NULL);
                break;
            }
            case 'x':
            case 'X': {
                const bool upper = (spec == 'X');
                const unsigned long long uval = linx_get_uarg(&ap, len);
                linx_format_uint(out, uval, 16, upper, alt, left, false, false,
                                 zero, width, precision, 0, NULL);
                break;
            }
            case 'p': {
                const void *ptr = va_arg(ap, const void *);
                unsigned long long uval = (unsigned long long)(uintptr_t)ptr;
                const int ptr_width = (int)(sizeof(uintptr_t) * 2);
                const int use_width = (width >= 0) ? width : ptr_width + 2;
                const int use_prec = (precision >= 0) ? precision : ptr_width;
                linx_format_uint(out, uval, 16, false, false, left, false, false,
                                 true, use_width, use_prec, 0, "0x");
                break;
            }
            case 'n': {
                const size_t n = out->count;
                if (len == LINX_LEN_HH) {
                    signed char *dst = va_arg(ap, signed char *);
                    *dst = (signed char)n;
                } else if (len == LINX_LEN_H) {
                    short *dst = va_arg(ap, short *);
                    *dst = (short)n;
                } else if (len == LINX_LEN_L) {
                    long *dst = va_arg(ap, long *);
                    *dst = (long)n;
                } else if (len == LINX_LEN_LL) {
                    long long *dst = va_arg(ap, long long *);
                    *dst = (long long)n;
                } else {
                    int *dst = va_arg(ap, int *);
                    *dst = (int)n;
                }
                break;
            }
            case '\0':
                return;
            default:
                linx_outc(out, '%');
                if (spec) {
                    linx_outc(out, spec);
                }
                break;
        }
    }
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    if (!format) {
        return -1;
    }

    struct linx_out out = {
        .dst = str,
        .size = size,
        .pos = 0,
        .count = 0,
        .to_uart = false,
    };

    va_list ap_copy;
    va_copy(ap_copy, ap);
    linx_vformat(&out, format, ap_copy);
    va_end(ap_copy);

    if (size && str) {
        size_t term = (out.pos < size) ? out.pos : (size - 1);
        str[term] = '\0';
    }

    return linx_size_to_int(out.count);
}

int snprintf(char *str, size_t size, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int r = vsnprintf(str, size, format, ap);
    va_end(ap);
    return r;
}

int vprintf(const char *format, va_list ap) {
    if (!format) {
        return -1;
    }
    struct linx_out out = {
        .dst = NULL,
        .size = 0,
        .pos = 0,
        .count = 0,
        .to_uart = true,
    };
    va_list ap_copy;
    va_copy(ap_copy, ap);
    linx_vformat(&out, format, ap_copy);
    va_end(ap_copy);
    return linx_size_to_int(out.count);
}

int printf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int r = vprintf(format, ap);
    va_end(ap);
    return r;
}

int vfprintf(FILE *stream, const char *format, va_list ap) {
    (void)stream;
    return vprintf(format, ap);
}

int fprintf(FILE *stream, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int r = vfprintf(stream, format, ap);
    va_end(ap);
    return r;
}
